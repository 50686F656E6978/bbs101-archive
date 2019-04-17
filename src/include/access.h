/*
	access.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef ACCESS_H_WJ109
#define ACCESS_H_WJ109	1

#include "User.h"

#define ACCESS_NO_PROMPT	1

int multi_x_access(User *, char *, int);
int multi_ping_access(User *, char *, int);
void check_recipients(User *);

#endif	/* ACCESS_H_WJ109 */

/* EOB */
