/*
	sysop_menu.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef SYSOP_MENU_H_WJ109
#define SYSOP_MENU_H_WJ109	1

#include "User.h"

void init_reboot_mutex(void);

void sysop_menu(User *);
void disconnect_user(User *);
void nuke_user(User *);
void reboot_system(User *);
void shutdown_system(User *);

#endif	/* SYSOP_MENU_H_WJ109 */

/* EOB */
