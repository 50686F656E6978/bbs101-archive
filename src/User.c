/*
	User.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "User.h"
#include "Memory.h"
#include "bufprintf.h"
#include "log.h"
#include "cstring.h"
#include "online.h"
#include "colorize.h"
#include "util.h"
#include "keys.h"
#include "backend.h"
#include "Stats.h"
#include "pager.h"
#include "msg_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

/*
	warning: order is important
*/
static char *user_flags[] = {
	"USR_HIDE_ADDRESS",
	"USR_12HRCLOCK",
	"USR_HELPING_HAND",
	"USR_X_DISABLED",
	"USR_BLOCK_FRIENDS",
	"USR_SHORT_WHO",
	"USR_SORT_BYNAME",
	"USR_SORT_DESCENDING",
	"USR_HIDE_ENEMIES"
};


void init_User(User *u) {
	memset(u, 0, sizeof(User));

	pthread_mutex_init(&(u->mutex), NULL);

	init_List(&(u->list));
	init_IOBuf(&(u->iobuf));
	init_Telnet(&(u->telnet));
	init_Display(&(u->display));

	init_GString(&(u->recipients));
	init_GString(&(u->talked_to));
	init_GString(&(u->dirty));
	init_GString(&(u->pager_buf));
	spurt_GString(&(u->pager_buf), PAGER_GROW);

	init_GString(&(u->quick));
	init_GString(&(u->friends));
	init_GString(&(u->enemies));
	init_GString(&(u->override));

	init_PList(&(u->sent_xmsgs));
	init_PList(&(u->recv_xmsgs));

	u->timeout = time(NULL);

	u->seen_xmsgs = u->recv_xmsgs.list.prev;
}

void deinit_User(User *u) {
	close_IOBuf(&(u->iobuf));

	deinit_GString(&(u->recipients));
	deinit_GString(&(u->talked_to));
	deinit_GString(&(u->dirty));
	deinit_GString(&(u->pager_buf));

	deinit_GString(&(u->quick));
	deinit_GString(&(u->friends));
	deinit_GString(&(u->enemies));
	deinit_GString(&(u->override));

	listdestroy_PList_XMsg(&(u->recv_xmsgs));
	listdestroy_PList_XMsg(&(u->sent_xmsgs));

	cstrfree(u->real_name);
	cstrfree(u->city);
	cstrfree(u->state);
	cstrfree(u->country);
	cstrfree(u->email);
	cstrfree(u->www);
	cstrfree(u->doing);
	cstrfree(u->reminder);
	cstrfree(u->vanity);
	cstrfree(u->xmsg_header);
	cstrfree(u->away);
	cstrfree(u->last_from);
	cstrfree(u->info);
}

User *new_User(void) {
User *u;

	if ((u = (User *)Malloc(sizeof(User))) == NULL)
		return NULL;

	init_User(u);

/* the user is practically always in a locked state, except when waiting for input */
	Lock(u);
	return u;
}

void destroy_User(User *u) {
	deinit_User(u);
	Free(u);
}

void Lock(User *u) {
	pthread_mutex_lock(&(u->mutex));
}

void Unlock(User *u) {
	pthread_mutex_unlock(&(u->mutex));
}

/*
	this is actually a not-so-deep copy ...
	it's lazy as it only copies some things that I need
*/
void deepcopy_User(User *u, User *copy) {
	Lock(u);
	Lock(copy);

	cstrcpy(copy->name, u->name, MAX_NAME);

	copy->flags = u->flags;
	copy->runtime_flags = u->runtime_flags;

	copy->away = cstrdup(u->away);

	deepcopy_GString(&(u->recipients), &(copy->recipients));
	deepcopy_GString(&(u->friends), &(copy->friends));
	deepcopy_GString(&(u->enemies), &(copy->enemies));
	deepcopy_GString(&(u->override), &(copy->override));

	Unlock(copy);
	Unlock(u);
}

void connect_User(User *usr, int sock) {
struct sockaddr_storage addr;
socklen_t addrlen = sizeof(struct sockaddr_storage);

	connect_IOBuf(&(usr->iobuf), sock);
/*
	get the hostname of the client
*/
	if (!getsockname(sock, (struct sockaddr *)&addr, &addrlen)) {
		if (getnameinfo((struct sockaddr *)&addr, addrlen, usr->hostname, NI_MAXHOST, NULL, 0, 0) == -1) {
			if (getnameinfo((struct sockaddr *)&addr, addrlen, usr->hostname, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == -1)
				cstrcpy(usr->hostname, "(unknown IP address)", NI_MAXHOST);
		}
	}
	log_info("new connection from %s", usr->hostname);
}

void close_connection(User *usr, char *reason) {
	if (usr->name[0]) {
		if (reason != NULL)
			log_auth("CLOSE %s (%s): %s", usr->name, usr->hostname, reason);

		remove_online_user(usr);

		usr->last_logout = time(NULL);
		usr->last_online_time = (unsigned int)difftime(usr->last_logout, usr->login_time);
		usr->total_time += (unsigned int)difftime(usr->last_logout, usr->online_timer);
		usr->online_timer = usr->last_logout;

		Free(usr->last_from);
		usr->last_from = NULL;

		if (usr->hostname[0])
			usr->last_from = cstrdup(usr->hostname);

		user_dirty(usr, "total_time");
		user_dirty(usr, "last_logout");
		user_dirty(usr, "last_online_time");
		user_dirty(usr, "last_from");
		save_User(usr);
	} else
		if (reason != NULL)
			log_auth("CLOSE (%s): %s", usr->hostname, reason);

	reset_colors(usr);

	update_stats(usr);
	save_stats();

	destroy_User(usr);

	pthread_exit(NULL);
}

void linkdead(User *usr) {
	notify_friends(usr->name, "freezes up and crumbles to dust");
	close_connection(usr, "connection lost");
}

void Flush(User *usr) {
	flush_IOBuf(&(usr->iobuf));
}

void Put(User *usr, char *str) {
	color_output(&(usr->iobuf), &(usr->display), str);
}

void Putc(User *usr, int c) {
	putc_IOBuf(&(usr->iobuf), c);
}

void Print(User *usr, char *fmt, ...) {
va_list args;
char buf[PRINT_BUF];

	if (usr == NULL || fmt == NULL || !*fmt)
		return;

	va_start(args, fmt);
	bufvprintf(buf, PRINT_BUF, fmt, args);
	va_end(args);

	Put(usr, buf);
}

/*
	do a 'raw' write; no color code processing, etc.
*/
void Write(User *usr, char *fmt, ...) {
va_list args;
char buf[PRINT_BUF];
int len;

	if (usr == NULL || fmt == NULL || !*fmt)
		return;

	va_start(args, fmt);
	len = bufvprintf(buf, PRINT_BUF, fmt, args);
	va_end(args);

	write_IOBuf(&(usr->iobuf), buf, len);
}

int Getch(User *usr) {
int c;

	usr->runtime_flags |= RTF_BUSY;				/* user is busy */

	c = -1;
	while(c == -1) {
		timeout(usr, ADD_TIMER);

		Unlock(usr);
		c = igetch_IOBuf(&(usr->iobuf), timeout_value(usr));
		Lock(usr);

		if (usr->runtime_flags & RTF_DEADCONN)
			close_connection(usr, NULL);

		if (c == IO_ERR)
			linkdead(usr);

		if (c == IO_EINTR) {				/* the getch() was interrupted */
			c = -1;							/* I'm busy, so ignore it for now */
			continue;
		}
		if ((c = telnet_negotiations(&(usr->telnet), c, &(usr->iobuf), &(usr->display))) == -1)
			continue;

		c = keyboard_cook(&(usr->iobuf), c);
	}
	timeout(usr, RESET_TIMER);
	return c;
}

/*
	iGetch() is an interruptible Getch() that returns on error or interrupt
	Note: for iGetch(), you need to set the BUSY flag 'manually'
*/
int iGetch(User *usr) {
int c;

/*	usr->runtime_flags &= ~RTF_BUSY;	*/		/* user is not busy */

	c = -1;
	while(c == -1) {
		timeout(usr, ADD_TIMER);

		Unlock(usr);
		c = igetch_IOBuf(&(usr->iobuf), timeout_value(usr));
		Lock(usr);

		if (usr->runtime_flags & RTF_DEADCONN)
			close_connection(usr, NULL);

		if (c == IO_ERR)
			return c;

		if (c == IO_EINTR)						/* the getch() was interrupted */
			return c;							/* propagate error code */

		if ((c = telnet_negotiations(&(usr->telnet), c, &(usr->iobuf), &(usr->display))) == -1)
			continue;

		c = keyboard_cook(&(usr->iobuf), c);
	}
	timeout(usr, RESET_TIMER);
	return c;
}

void timeout(User *usr, int c) {
time_t t;
ColorIndex saved_color;

	if (c == RESET_TIMER) {
		usr->timeout = time(NULL);
		usr->runtime_flags &= ~(RTF_TIMEOUT_WARNING|RTF_TIMEOUT_WARNING2);
		return;
	}
	t = time(NULL);
/*
	user is on the login prompt
*/
	if (!usr->name[0]) {
		if (difftime(t, usr->timeout) >= LOGIN_TIMEOUT) {
			Put(usr, "\n\nConnection timed out\n\n");
			close_connection(usr, "idle connection");
		}
		return;
	}
/*
	logged in users
*/
	if (!(usr->runtime_flags & RTF_TIMEOUT_WARNING)) {
		if (difftime(t, usr->timeout) >= USER_TIMEOUT) {
			usr->runtime_flags |= RTF_TIMEOUT_WARNING;
			saved_color = usr->display.current_color;
			Put(usr, "\n\n<red>Hello? Is anybody out there?? You will be logged off unless you start looking more alive!\n\n");
			user_color(usr, saved_color);
			beep(usr);
			usr->timeout = time(NULL);
		}
		return;
	}
	if (!(usr->runtime_flags & RTF_TIMEOUT_WARNING2)) {
		if (difftime(t, usr->timeout) >= USER_TIMEOUT2) {
			usr->runtime_flags |= RTF_TIMEOUT_WARNING2;
			saved_color = usr->display.current_color;
			Put(usr, "\n<red>WAKE UP! You will be logged off NOW unless you show you're alive!\n\n");
			user_color(usr, saved_color);
			beep(usr);
			usr->timeout = time(NULL);
		}
		return;
	}
	if (difftime(t, usr->timeout) >= USER_TIMEOUT3) {
		Put(usr, "\n<red>You have been logged off due to inactivity\n\n");

		notify_friends(usr->name, "has been logged out due to inactivity");
		close_connection(usr, "idle connection");
	}
}

/*
	set the ideal timeout value for igetch(), so it does not wake up too often
*/
int timeout_value(User *usr) {
	if (!usr->name[0])					/* user is on the login prompt */
		return LOGIN_TIMEOUT;

	if (!(usr->runtime_flags & RTF_TIMEOUT_WARNING)) {
		if (usr->runtime_flags & RTF_LOCKED)
			return LOCKED_TIMEOUT;

		return USER_TIMEOUT;
	}
	if (!(usr->runtime_flags & RTF_TIMEOUT_WARNING2))
		return USER_TIMEOUT2;

	return USER_TIMEOUT3;
}

void wakeup(User *u) {
	pthread_kill(u->tid, SIGUSR1);			/* wake up user, interrupt select() */
}

/*
	add field to the dirty list
	this is the list of fields that need to be synced
*/
int user_dirty(User *u, char *field) {
	if (u->dirty.len <= 0)
		return gstrcpy(&(u->dirty), field);

	if (cstrfind(u->dirty.str, field, ','))		/* already in it */
		return 0;

	gstrcat(&(u->dirty), ",");
	gstrcat(&(u->dirty), field);
	return 0;
}

void save_User_flags(User *u, GString *gstr) {
	flags_to_str(u->flags, gstr, user_flags, sizeof(user_flags) / sizeof(char *));
}

void load_User_flags(User *u, char *str) {
	u->flags = str_to_flags(str, user_flags, sizeof(user_flags) / sizeof(char *));
}

void user_color(User *u, ColorIndex color) {
	restore_color_to(&(u->iobuf), &(u->display), color);
}

void raw_user_color(User *u, int color) {
	if (color >= ANSI_BLACK && color <= ANSI_WHITE)
		restore_color_to(&(u->iobuf), &(u->display), color - ANSI_BLACK);
}

/* EOB */
