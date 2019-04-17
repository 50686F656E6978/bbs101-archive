/*
	msg_system.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "msg_system.h"
#include "Stats.h"
#include "edit.h"
#include "cstring.h"
#include "util.h"
#include "online.h"
#include "access.h"
#include "log.h"
#include "backend.h"

#include <stdio.h>
#include <stdlib.h>


int sent_xmsg_stats(User *usr, XMsg *x, char *name) {
	switch(x->type) {
		case XMSG_X:
			usr->xsent++;
			user_dirty(usr, "xsent");
			update_stats(usr);
			inc_stats(STATS_XSENT);
			break;

		case XMSG_EMOTE:
			usr->esent++;
			user_dirty(usr, "esent");
			update_stats(usr);
			inc_stats(STATS_ESENT);
			break;

		case XMSG_FEELING:
			usr->fsent++;
			user_dirty(usr, "fsent");
			update_stats(usr);
			inc_stats(STATS_FSENT);
			break;

		case XMSG_QUESTION:
			usr->qsent++;
			user_dirty(usr, "qsent");
			update_stats(usr);
			inc_stats(STATS_QSENT);
			break;

		case XMSG_ANSWER:
			usr->qansw++;
			user_dirty(usr, "qansw");
			update_stats(usr);
			inc_stats(STATS_QANSW);
			break;

		default:
			;
	}
	if (name != NULL) {							/* add to talked_to list */
		if (usr->talked_to.len <= 0)
			gstrcpy(&(usr->talked_to), name);
		else {
			if (!cstrfind(usr->talked_to.str, name, ',')) {
				gstrcat(&(usr->talked_to), ",");
				gstrcat(&(usr->talked_to), name);
			}
		}
	}
	return 0;
}

/*
	destroy PList of referenced XMsgs
*/
void listdestroy_PList_XMsg(PList *xpl) {
PList *pl;
XMsg *x;

	while((pl = pop_PList(xpl)) != NULL) {
		x = (XMsg *)(pl->p);
		x->refcount--;
		if (x->refcount <= 0)
			destroy_XMsg(x);

		destroy_PList(pl);
	}
}

/*
	send yourself a carbon copy; do not update stats
*/
int send_xmsg_bcc(User *usr, XMsg *x) {
PList *pl;

	if ((pl = new_PList(x)) == NULL)
		return -1;

	x->refcount++;

	append_PList(&(usr->sent_xmsgs), pl);
	return 0;
}

/*
	Note: User u must be locked
*/
int recv_XMsg(User *u, XMsg *x) {
PList *pl;

	if ((pl = new_PList(x)) == NULL)
		return -1;

	x->refcount++;

	append_PList(&(u->recv_xmsgs), pl);

	switch(x->type) {
		case XMSG_X:
			u->xrecv++;
			user_dirty(u, "xrecv");
			update_stats(u);
			inc_stats(STATS_XRECV);
			break;

		case XMSG_EMOTE:
			u->erecv++;
			user_dirty(u, "erecv");
			update_stats(u);
			inc_stats(STATS_ERECV);
			break;

		case XMSG_FEELING:
			u->frecv++;
			user_dirty(u, "frecv");
			update_stats(u);
			inc_stats(STATS_FRECV);
			break;

		default:
			;
	}
	wakeup(u);
	return 0;
}

XMsg *unseen_XMsg(User *usr) {
PList *pl;

	if (usr->seen_xmsgs == usr->recv_xmsgs.list.prev)
		return NULL;

	pl = list_item(usr->seen_xmsgs->next, PList, list);
	usr->seen_xmsgs = &(pl->list);
	return (XMsg *)(pl->p);
}

int send_xmsg(User *usr, XMsg *x, char *name) {
User *u;

	if ((u = lock_user(name)) == NULL) {
		Print(usr, "<red>Sorry, but <yellow>%s<red> already left\n", name);
		remove_recipient(usr, name);
		return 0;
	}
	if (recv_XMsg(u, x) == -1) {		/* recipient receives the message */
		Print(usr, "<red>Out of memory, message not received by <yellow>%s\n", u->name);
		return 0;
	}
	if (u->runtime_flags & RTF_BUSY)
		Print(usr, "<yellow>%s<green> is busy and will receive your message when done\n", u->name);
	else
		Print(usr, "<green>Message received by <yellow>%s\n", u->name);

	unlock_user(u);

	sent_xmsg_stats(usr, x, name);		/* update stats */
	return 1;
}

void enter_xmsg(User *usr) {
char name[MAX_NAME], *p, *endp;
XMsg *x;
GString gstr;
int sent;

	if ((x = new_XMsg()) == NULL) {
		Put(usr, "<red>Due to an error, you can not send messages now\n");
		return;
	}
	if (edit_recipients(usr, multi_x_access) == -1) {
		destroy_XMsg(x);
		return;
	}
	if (usr->recipients.len <= 0) {
		destroy_XMsg(x);
		return;
	}
	check_recipients(usr);

	if (usr->recipients.len <= 0) {
		destroy_XMsg(x);
		return;
	}

/* enter the message text */

	Put(usr, "<green>");

	init_GString(&gstr);

	if (edit_lines(usr, &gstr, ">", MAX_X_LINES) == -1) {
		Put(usr, "<red>Message not sent\n");
		destroy_XMsg(x);
		deinit_GString(&gstr);
		return;
	}
	if (gstr.len <= 0) {
		Put(usr, "<red>Message not sent\n");
		destroy_XMsg(x);
		deinit_GString(&gstr);
		return;
	}
	ctrim_line(gstr.str);
	if (add_XMsg_line(x, gstr.str) == -1) {
		Put(usr, "<red>Due to an error, you can not send messages now\n");
		destroy_XMsg(x);
		deinit_GString(&gstr);
		return;
	}
	deinit_GString(&gstr);

	check_recipients(usr);

	if (usr->recipients.len <= 0) {
		destroy_XMsg(x);
		return;
	}
	set_XMsg(x, XMSG_X, usr->name, usr->recipients.str, time(NULL));

	sent = 0;

	p = usr->recipients.str;
	while((endp = cstrchr(p, ',')) != NULL) {
		*endp = 0;
		cstrcpy(name, p, MAX_NAME);
		*endp = ',';
		endp++;
		p = endp;

		sent += send_xmsg(usr, x, name);
	}
	cstrcpy(name, p, MAX_NAME);
	sent += send_xmsg(usr, x, name);

	if (sent)
		send_xmsg_bcc(usr, x);		/* remember message as sent in 'this' user */
	else
		destroy_XMsg(x);
}

void print_XMsg(User *usr, XMsg *x) {
struct tm *tm;

	tm = localtime(&x->mtime);		/* this should be the local time as configured for the user */

	if ((usr->flags & USR_12HRCLOCK) && tm->tm_hour > 12)
		tm->tm_hour -= 12;

	switch(x->type) {
		case XMSG_X:
			Print(usr, "\n\n<magenta>***<cyan> eXpress Message received from <yellow>%s<cyan> at <white>%02d:%02d <magenta>***<yellow>\n", x->from, tm->tm_hour, tm->tm_min);
			Put(usr, x->msg);
			Put(usr, "\n");

			if (usr->recipients.len <= 0)
				add_recipient(usr, x->from);
			break;

		case XMSG_SYSTEM:
			Print(usr, "\n\n<white>*** <yellow>System message received at %d:%02d <white>***<red>\n", tm->tm_hour, tm->tm_min);
			Put(usr, x->msg);
			Put(usr, "\n");
			break;

		case XMSG_NOTIFY:
			Put(usr, x->msg);
			Put(usr, "\n");
			break;

		case XMSG_EMOTE:
			Print(usr, "\n<yellow>\n(%02d:%02d) <white>%s<yellow> %s\n", tm->tm_hour, tm->tm_min, x->from, x->msg);
			break;

		default:
			log_err("print_XMsg(): unknown XMsg type %d", x->type);
			abort();
	}
	beep(usr);
}

int received_new_XMsgs(User *usr) {
int has_new;
XMsg *x;

	has_new = 0;

	while((x = unseen_XMsg(usr)) != NULL) {
		print_XMsg(usr, x);
		has_new = 1;
	}
	return has_new;
}

void enter_emote(User *usr) {
char name[MAX_NAME], line[MAX_LINE], *p, *endp;
XMsg *x;
int sent;

	if ((x = new_XMsg()) == NULL) {
		Put(usr, "<red>Due to an error, you can not send messages now\n");
		return;
	}
	if (edit_recipients(usr, multi_x_access) == -1) {
		destroy_XMsg(x);
		return;
	}
	if (usr->recipients.len <= 0) {
		destroy_XMsg(x);
		return;
	}
	check_recipients(usr);

	if (usr->recipients.len <= 0) {
		destroy_XMsg(x);
		return;
	}

/* enter the message text */

	Print(usr, "<cyan>* <white>%s <yellow>", usr->name);

	if (edit_line(usr, line, MAX_LINE) == -1 || !line[0]) {
		Put(usr, "<red>Message not sent\n");
		destroy_XMsg(x);
		return;
	}
	ctrim_line(line);

	if (!line[0]) {
		Put(usr, "<red>Message not sent\n");
		destroy_XMsg(x);
		return;
	}
	if (add_XMsg_line(x, line) == -1) {
		Put(usr, "<red>Due to an error, you can not send messages now\n");
		destroy_XMsg(x);
		return;
	}
	check_recipients(usr);

	if (usr->recipients.len <= 0) {
		destroy_XMsg(x);
		return;
	}
	set_XMsg(x, XMSG_EMOTE, usr->name, usr->recipients.str, time(NULL));

	sent = 0;

	p = usr->recipients.str;
	while((endp = cstrchr(p, ',')) != NULL) {
		*endp = 0;
		cstrcpy(name, p, MAX_NAME);
		*endp = ',';
		endp++;
		p = endp;

		sent += send_xmsg(usr, x, name);
	}
	cstrcpy(name, p, MAX_NAME);
	sent += send_xmsg(usr, x, name);

	if (sent)
		send_xmsg_bcc(usr, x);		/* remember message as sent in 'this' user */
	else
		destroy_XMsg(x);
}

/* EOB */
