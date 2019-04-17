/*
	login.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef LOGIN_H_WJ109
#define LOGIN_H_WJ109	1

#include "User.h"

#define MAX_LOGIN_ATTEMPTS	3

void login(User *);
int login_password(User *, char *);
void logout(User *);
int new_user(User *, char *);
int ask_new_user(User *, char *);
int enter_new_password(User *, char *);
void go_online(User *);

#endif	/* LOGIN_H_WJ109 */

/* EOB */
