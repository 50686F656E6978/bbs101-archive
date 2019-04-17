/*
	online.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef ONLINE_H_WJ109
#define ONLINE_H_WJ109	1

#include "User.h"

/*
	they're just a bunch of pointers ...
	so it's no problem to make this number a bit large
*/
#define ONLINE_SLOTS	64

typedef struct {
	char name[MAX_NAME], doing[MAX_LINE];
	int flags, runtime_flags;
	long online_time;
} WhoListEntry;

void init_online_list(void);
void locked_add_online_user(User *);
void add_online_user(User *);
void locked_remove_online_user(User *);
void remove_online_user(User *);
int is_online(char *, User *);
WhoListEntry *get_online_list(int *);
int get_online_names(GString *);

User *lock_user(char *);
void unlock_user(User *);

int notify_friends(char *, char *);
int broadcast(User *, char *);

void broadcast_callback(void (*)(User *));
int count_online(void);

#endif	/* ONLINE_H_WJ109 */

/* EOB */
