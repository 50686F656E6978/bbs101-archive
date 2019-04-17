/*
	online.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- the online list is a hash of linked User objects
*/

#include "config.h"
#include "online.h"
#include "memset.h"
#include "Memory.h"
#include "cstring.h"
#include "msg_system.h"
#include "bufprintf.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


static User *online_list[ONLINE_SLOTS];
static int num_online;

static pthread_mutex_t online_mutex;


void init_online_list(void) {
	memset(online_list, 0, ONLINE_SLOTS * sizeof(User *));
	num_online = 0;
	pthread_mutex_init(&online_mutex, NULL);
}

int hashaddr_ascii(char *key) {
char *p;
int addr, c;

	p = key;
	addr = *p;
	p++;
	while(*p) {
		c = *p - ' ';

		addr <<= 1;
		addr ^= c;

		p++;
	}
	if (addr < 0)
		addr = -addr;
	return addr;
}

/*
	Note: this code assumes 'usr' is not yet online
*/
void locked_add_online_user(User *usr) {
int addr;

	if (!usr->name[0])
		return;

	addr = hashaddr_ascii(usr->name) % ONLINE_SLOTS;

	if (online_list[addr] == NULL)
		online_list[addr] = usr;
	else
		add_User(online_list[addr], usr);

	num_online++;
}

void add_online_user(User *usr) {
	if (!usr->name[0])
		return;

	pthread_mutex_lock(&online_mutex);

	locked_add_online_user(usr);

	pthread_mutex_unlock(&online_mutex);
}

/*
	Note: this code assumes 'usr' is indeed online
*/
void locked_remove_online_user(User *usr) {
int addr;

	if (!usr->name[0])
		return;

	addr = hashaddr_ascii(usr->name) % ONLINE_SLOTS;

	if (empty_list(&(usr->list))) {
		if (online_list[addr] == usr)
			online_list[addr] = NULL;
	} else
		unlink_User(online_list[addr], usr);

	num_online--;
	if (num_online < 0)
		num_online = 0;
}

void remove_online_user(User *usr) {
	if (!usr->name[0])
		return;

	pthread_mutex_lock(&online_mutex);

	locked_remove_online_user(usr);

	pthread_mutex_unlock(&online_mutex);
}

/*
	returns 0 if user not online
	returns 1 if user is online, and makes a copy of the user object,
	so you can examine its properties

	NB. Take extra care when *copy is not NULL; deepcopy() will Lock() the user
	    so if *name is the current user, this will cause deadlock
*/
int is_online(char *name, User *copy) {
User *u;
List *l;
int addr;

	if (name == NULL || !*name)
		return 0;

	addr = hashaddr_ascii(name) % ONLINE_SLOTS;

	pthread_mutex_lock(&online_mutex);

	if (online_list[addr] == NULL) {
		pthread_mutex_unlock(&online_mutex);
		return 0;
	}
	if (!strcmp(online_list[addr]->name, name)) {
		if (copy != NULL)
			deepcopy_User(online_list[addr], copy);

		pthread_mutex_unlock(&online_mutex);
		return 1;
	}
	foreach_list(l, online_list[addr]->list) {
		u = list_item(l, User, list);

		if (!strcmp(u->name, name)) {
			if (copy != NULL) {
				deepcopy_User(u, copy);
				init_List(&(copy->list));
			}
			pthread_mutex_unlock(&online_mutex);
			return 1;
		}
	}
	pthread_mutex_unlock(&online_mutex);
	return 0;
}

/*
	return value is an array of WhoListEntry's
	return value must be freed using Free()
*/
WhoListEntry *get_online_list(int *count) {
WhoListEntry *w;
int i, n;
User *u;
List *l;
time_t now;

	*count = 0;

	pthread_mutex_lock(&online_mutex);

	if (num_online <= 0) {
		pthread_mutex_unlock(&online_mutex);
		return NULL;
	}
	if ((w = Malloc(num_online * sizeof(WhoListEntry))) == NULL) {
		pthread_mutex_unlock(&online_mutex);
		return NULL;
	}
	*count = num_online;

	now = time(NULL);

	n = 0;
	for(i = 0; i < ONLINE_SLOTS; i++) {
		if (online_list[i] != NULL) {
			u = online_list[i];
			cstrcpy(w[n].name, u->name, MAX_NAME);
			w[n].doing[0] = 0;
			cstrcpy(w[n].doing, u->doing, MAX_LINE);
			w[n].flags = u->flags;
			w[n].runtime_flags = u->runtime_flags;
			w[n].online_time = (long)difftime(now, u->login_time);
			n++;

			foreach_list(l, online_list[i]->list) {
				u = list_item(l, User, list);

				cstrcpy(w[n].name, u->name, MAX_NAME);
				w[n].doing[0] = 0;
				cstrcpy(w[n].doing, u->doing, MAX_LINE);
				w[n].flags = u->flags;
				w[n].runtime_flags = u->runtime_flags;
				w[n].online_time = (long)difftime(now, u->login_time);
				n++;
			}
		}
	}
	pthread_mutex_unlock(&online_mutex);
	return w;
}

int get_online_names(GString *gstr) {
int i, n;
User *u;
List *l;

	reset_GString(gstr);
 
	pthread_mutex_lock(&online_mutex);

	if (num_online <= 0) {
		pthread_mutex_unlock(&online_mutex);
		return 0;
	}
	n = 0;
	for(i = 0; i < ONLINE_SLOTS; i++) {
		if (online_list[i] != NULL) {
			u = online_list[i];

			gstrcat(gstr, u->name);
			gstrcat(gstr, ",");
			n++;

			foreach_list(l, online_list[i]->list) {
				u = list_item(l, User, list);

				gstrcat(gstr, u->name);
				gstrcat(gstr, ",");
				n++;
			}
		}
	}
	pthread_mutex_unlock(&online_mutex);

	if (gstr->len > 0)					/* remove the trailing comma */
		gstr->str[--(gstr->len)] = 0;

	return n;
}

/*
	find user, and keep it locked down
	returns the live User object
*/
User *lock_user(char *name) {
User *u;
List *l;
int addr;

	if (name == NULL || !*name)
		return 0;

	addr = hashaddr_ascii(name) % ONLINE_SLOTS;

	pthread_mutex_lock(&online_mutex);

	if (online_list[addr] == NULL) {
		pthread_mutex_unlock(&online_mutex);
		return NULL;
	}
	if (!strcmp(online_list[addr]->name, name))
		return online_list[addr];

	foreach_list(l, online_list[addr]->list) {
		u = list_item(l, User, list);

		if (!strcmp(u->name, name)) {
			Lock(u);
			return u;
		}
	}
	pthread_mutex_unlock(&online_mutex);
	return NULL;
}

void unlock_user(User *u) {
	Unlock(u);
	pthread_mutex_unlock(&online_mutex);
}

int notify_friends(char *username, char *msg) {
char buf[MAX_LONGLINE];
XMsg *x;
int i;
User *u;
List *l;

	if (username == NULL || !username[0])
		return -1;

	if ((x = new_XMsg()) == NULL)
		return -1;

	bufprintf(buf, sizeof(buf), "<yellow>%s<magenta> %s", username, msg);

	if (add_XMsg_line(x, buf) == -1) {
		destroy_XMsg(x);
		return -1;
	}
	set_XMsg(x, XMSG_NOTIFY, NULL, NULL, time(NULL));

/* send notification to all users that have username listed as friend */

	pthread_mutex_lock(&online_mutex);

	if (num_online <= 0) {
		pthread_mutex_unlock(&online_mutex);
		destroy_XMsg(x);
		return -1;
	}
	for(i = 0; i < ONLINE_SLOTS; i++) {
		if (online_list[i] != NULL) {
			u = online_list[i];

			if (cstrfind(u->friends.str, username, ',')) {
				Lock(u);
				recv_XMsg(u, x);
				Unlock(u);
			}
			foreach_list(l, online_list[i]->list) {
				u = list_item(l, User, list);

				if (cstrfind(u->friends.str, username, ',')) {
					Lock(u);
					recv_XMsg(u, x);
					Unlock(u);
				}
			}
		}
	}
	pthread_mutex_unlock(&online_mutex);

	destroy_XMsg(x);
	return 0;
}

int broadcast(User *usr, char *msg) {
XMsg *x;
int i;
User *u;
List *l;

	if ((x = new_XMsg()) == NULL)
		return -1;

	if (add_XMsg_line(x, msg) == -1) {
		destroy_XMsg(x);
		return -1;
	}
	set_XMsg(x, XMSG_SYSTEM, NULL, NULL, time(NULL));

/* send notification to all online users */

	pthread_mutex_lock(&online_mutex);

	if (num_online <= 0) {
		pthread_mutex_unlock(&online_mutex);
		destroy_XMsg(x);
		return -1;
	}
	for(i = 0; i < ONLINE_SLOTS; i++) {
		if (online_list[i] != NULL) {
			u = online_list[i];

			if (u != usr)
				Lock(u);

			recv_XMsg(u, x);

			if (u != usr)
				Unlock(u);

			foreach_list(l, online_list[i]->list) {
				u = list_item(l, User, list);

				if (u != usr)
					Lock(u);

				recv_XMsg(u, x);

				if (u != usr)
					Unlock(u);
			}
		}
	}
	pthread_mutex_unlock(&online_mutex);

	destroy_XMsg(x);
	return 0;
}

/*
	run callback function on all online users
*/
void broadcast_callback(void (*callback)(User *)) {
int i;
User *u;
List *l;

	pthread_mutex_lock(&online_mutex);

	if (num_online <= 0) {
		pthread_mutex_unlock(&online_mutex);
		return;
	}
	for(i = 0; i < ONLINE_SLOTS; i++) {
		if (online_list[i] != NULL) {
			u = online_list[i];

			Lock(u);
			callback(u);
			Unlock(u);

			foreach_list(l, online_list[i]->list) {
				u = list_item(l, User, list);

				Lock(u);
				callback(u);
				Unlock(u);
			}
		}
	}
	pthread_mutex_unlock(&online_mutex);
}

int count_online(void) {
int n;

	pthread_mutex_lock(&online_mutex);
	n = num_online;
	pthread_mutex_unlock(&online_mutex);
	return n;
}

/* EOB */
