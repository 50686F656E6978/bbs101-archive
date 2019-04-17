/*
	roomlevel.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef ROOMLEVEL_H_WJ109
#define ROOMLEVEL_H_WJ109	1

#include "User.h"

#define HELP_FILE	"help.txt"

#define WHO_LONG	1
#define WHO_SHORT	2

void roomlevel(User *);
void date(User *);
void wholist(User *, int);
void online_friends(User *);
void online_talked_to(User *);
void ping(User *);
void enter_sysop_mode(User *);
void lock_screen(User *);
void profile(User *);

#endif	/* ROOMLEVEL_H_WJ109 */

/* EOB */
