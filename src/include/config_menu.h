/*
	config_menu.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef CONFIG_MENU_H_WJ109
#define CONFIG_MENU_H_WJ109	1

#include "User.h"

void config_menu(User *);
void config_address(User *);
void config_profile(User *);
void config_vanity(User *);
void config_doing(User *);
void config_reminder(User *);
void config_options(User *);
void config_terminal(User *);
void config_who(User *);
void config_password(User *);

void change_config_string(User *, char **, char *, int, char *);

#endif	/* CONFIG_MENU_H_WJ109 */

/* EOB */
