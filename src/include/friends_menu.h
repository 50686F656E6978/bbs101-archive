/*
	friends_menu.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef FRIENDS_MENU_H_WJ109
#define FRIENDS_MENU_H_WJ109	1

#include "User.h"

typedef enum {
	EXIT_MENU = 0,
	FRIENDS_MENU,
	ENEMIES_MENU,
	OVERRIDES_MENU
} FriendsMenu;

void config_friends(User *);
void config_enemies(User *);
void config_overrides(User *);
void config_friends_enemies_overrides(User *, FriendsMenu);

void friends_menu(User *, FriendsMenu *);
void add_friend(User *);
void remove_friend(User *);

void enemies_menu(User *, FriendsMenu *);
void add_enemy(User *);
void remove_enemy(User *);

void overrides_menu(User *, FriendsMenu *);
void add_override(User *);
void remove_override(User *);

void print_namelist(User *, GString *);

#endif	/* FRIENDS_MENU_H_WJ109 */

/* EOB */
