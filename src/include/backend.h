/*
	backend.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef BACKEND_H_WJ109
#define BACKEND_H_WJ109	1

#include "User.h"
#include "Stats.h"

int user_exists(char *);
int get_crypted_passwd(char *, char *, int);
int get_user_field(char *, char *, char *, int);
int create_user(char *, char *);
int load_User(User *, char *);
int load_User_profile(User *, char *);
void unload_User_profile(User *);
int user_int_field(User *, char *, unsigned int *);
int user_safe_string_field(User *, char *, char **);
int user_quoted_string_field(User *, char *, char **);
int user_gstring_field(User *, char *, GString *);
int save_User(User *);
int save_User_int(User *, char *, unsigned int);
int save_password(char *, char *);
int is_sysop(char *);
char *get_sysops(void);
int backend_load_stats(Stats *);
int backend_save_stats(Stats *);
int delete_user(User *usr, char *);

#endif	/* BACKEND_H_WJ109 */

/* EOB */
