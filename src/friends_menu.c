/*
	friends_menu.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "friends_menu.h"
#include "backend.h"
#include "keys.h"
#include "cstring.h"
#include "edit.h"
#include "backend.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>


/*
	Explanation for the unusual structure of the program here:
	The Friends/Enemies/Override menu allows you to swap instantly between menus.
	Calling the menu subroutines the normal way here, any user could cause a stack
	overflow easily by simply keeping switching menus, going deeper and deeper into
	subroutines until the stack would crash. To keep this from happening, the menu
	switch enables a different menu, and returns to the toplevel function that
	selects the correct menu subroutine to execute.
*/
void config_friends(User *usr) {
	config_friends_enemies_overrides(usr, FRIENDS_MENU);
}

void config_enemies(User *usr) {
	config_friends_enemies_overrides(usr, ENEMIES_MENU);
}

void config_overrides(User *usr) {
	config_friends_enemies_overrides(usr, OVERRIDES_MENU);
}

void config_friends_enemies_overrides(User *usr, FriendsMenu start_menu) {
FriendsMenu menu;

	menu = start_menu;

	for(;;) {
		switch(menu) {
			case EXIT_MENU:
				Put(usr, "\n");
				save_User(usr);
				return;

			case FRIENDS_MENU:
				friends_menu(usr, &menu);
				break;

			case ENEMIES_MENU:
				enemies_menu(usr, &menu);
				break;

			case OVERRIDES_MENU:
				overrides_menu(usr, &menu);
				break;

			default:
				abort();
		}
	}
}

void friends_menu(User *usr, FriendsMenu *menu) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			if (usr->friends.len <= 0)
				Put(usr, "<cyan>\n Your friend list is empty\n");
			else {
				Put(usr, "\n<green>");
				print_namelist(usr, &(usr->friends));
			}
			Put(usr, "<magenta>\n"
				" <hotkey>Add friend");

			if (usr->friends.len <= 0)
				Put(usr, "\n");
			else
				Put(usr, "                  <hotkey>Remove friend\n");

			Put(usr, " Switch to <hotkey>enemy list");
			if (usr->flags & USR_X_DISABLED)
				Put(usr, "        Switch to <hotkey>override list");

			Put(usr, "\n");
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Friends%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				*menu = EXIT_MENU;
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'a':
			case 'A':
			case '+':
			case '=':
				Put(usr, "Add friend\n");

				if (cstrcount(usr->friends.str, ',') >= MAX_FRIENDS-1)
					Print(usr, "<red>You already have %d friends defined\n", MAX_FRIENDS);
				else
					add_friend(usr);
				break;

			case 'r':
			case 'R':
			case 'd':
			case 'D':
			case '-':
			case '_':
				if (usr->friends.len <= 0) {
					print_menu = 0;
					break;
				}
				Put(usr, "Remove friend\n");
				remove_friend(usr);
				break;

			case '<':
			case 'e':
			case 'E':
				Put(usr, "Enemies\n");
				*menu = ENEMIES_MENU;
				return;

			case 'o':
			case 'O':
				if (!(usr->flags & USR_X_DISABLED)) {
					print_menu = 0;
					break;
				}
				Put(usr, "Overrides\n");
				*menu = OVERRIDES_MENU;
				return;

			default:
				print_menu = 0;
		}
	}
}

void add_friend(User *usr) {
char name[MAX_NAME];

	Put(usr, "<green>Enter your new friend's name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "\n<green>Heh, your best friend is <yellow>YOU\n");
		return;
	}
	if (cstrfind(usr->friends.str, name, ',')) {
		Print(usr, "\n<yellow>%s<red> already is on your friend list\n", name);
		return;
	}
	if (!user_exists(name)) {
		Print(usr, "\n<red>No such user\n");
		return;
	}
/* maybe this person was listed as enemy? */
	if (cstrfind(usr->enemies.str, name, ',')) {
		cstrremove(usr->enemies.str, name, ',');
		usr->enemies.len = strlen(usr->enemies.str);

		shrink_GString(&(usr->enemies));

		user_dirty(usr, "enemies");

		Print(usr, "<yellow>%s<green> moved from your enemy to friend list\n", name);
	}
	if (usr->friends.len > 0)
		gstrcat(&(usr->friends), ",");

	gstrcat(&(usr->friends), name);

	user_dirty(usr, "friends");
}

void remove_friend(User *usr) {
char name[MAX_NAME];

	Put(usr, "<green>Enter name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "\n<green>Stopped being friends with yourself? How sad and lonely ...\n");
		return;
	}
	if (!cstrfind(usr->friends.str, name, ',')) {
		Put(usr, "\n<red>There is no such person on your friend list\n");
		return;
	}
	cstrremove(usr->friends.str, name, ',');
	usr->friends.len = strlen(usr->friends.str);

	shrink_GString(&(usr->friends));

	user_dirty(usr, "friends");
}

void enemies_menu(User *usr, FriendsMenu *menu) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			if (usr->enemies.len <= 0)
				Put(usr, "<cyan>\n Your enemy list is empty\n");
			else {
				Put(usr, "\n<green>");
				print_namelist(usr, &(usr->enemies));
			}
			Put(usr, "<magenta>\n"
				" <hotkey>Add enemy");

			if (usr->enemies.len <= 0)
				Put(usr, "\n");
			else
				Put(usr, "                   <hotkey>Remove enemy\n");

			Put(usr, " Switch to <hotkey>friend list");
			if (usr->flags & USR_X_DISABLED)
				Put(usr, "       Switch to <hotkey>override list");

			Put(usr, "\n");
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Enemies%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				*menu = EXIT_MENU;
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'a':
			case 'A':
			case '+':
			case '=':
				Put(usr, "Add enemy\n");

				if (cstrcount(usr->enemies.str, ',') >= MAX_FRIENDS-1)
					Print(usr, "<red>You already have %d enemies defined\n", MAX_FRIENDS);
				else
					add_enemy(usr);
				break;

			case 'r':
			case 'R':
			case 'd':
			case 'D':
			case '-':
			case '_':
				if (usr->enemies.len <= 0) {
					print_menu = 0;
					break;
				}
				Put(usr, "Remove enemy\n");
				remove_enemy(usr);
				break;

			case '>':
			case 'f':
			case 'F':
				Put(usr, "Friends\n");
				*menu = FRIENDS_MENU;
				return;

			case 'o':
			case 'O':
				if (!(usr->flags & USR_X_DISABLED)) {
					print_menu = 0;
					break;
				}
				Put(usr, "Overrides\n");
				*menu = OVERRIDES_MENU;
				return;

			default:
				print_menu = 0;
		}
	}
}

void add_enemy(User *usr) {
char name[MAX_NAME];

	Put(usr, "<green>Enter your new enemy's name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "\n<green>Heh, you are your own worst enemy!\n");
		return;
	}
	if (cstrfind(usr->enemies.str, name, ',')) {
		Print(usr, "\n<yellow>%s<red> already is on your enemy list\n", name);
		return;
	}
	if (!user_exists(name)) {
		Print(usr, "\n<red>No such user\n");
		return;
	}
/* maybe this person was listed as friend? */
	if (cstrremove(usr->friends.str, name, ',')) {
		usr->friends.len = strlen(usr->friends.str);
		shrink_GString(&(usr->friends));

		user_dirty(usr, "friends");

		Print(usr, "<yellow>%s<green> moved from your friend to enemy list\n", name);
	}
/* maybe we talked to this person */
	if (cstrremove(usr->talked_to.str, name, ',')) {
		usr->talked_to.len = strlen(usr->talked_to.str);
		shrink_GString(&(usr->talked_to));
	}
/* maybe this person was also on the override list */
	if (cstrremove(usr->override.str, name, ',')) {
		usr->override.len = strlen(usr->override.str);
		shrink_GString(&(usr->override));
	}
	if (usr->enemies.len > 0)
		gstrcat(&(usr->enemies), ",");

	gstrcat(&(usr->enemies), name);

	user_dirty(usr, "enemies");
}

void remove_enemy(User *usr) {
char name[MAX_NAME];

	Put(usr, "<green>Enter name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "\n<green>It's good to see you've made peace with yourself\n");
		return;
	}
	if (!cstrfind(usr->enemies.str, name, ',')) {
		Put(usr, "\n<red>There is no such person on your enemy list\n");
		return;
	}
	cstrremove(usr->enemies.str, name, ',');
	usr->enemies.len = strlen(usr->enemies.str);

	shrink_GString(&(usr->enemies));

	user_dirty(usr, "enemies");
}

void overrides_menu(User *usr, FriendsMenu *menu) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			if (usr->override.len <= 0)
				Put(usr, "<cyan>\n Your override list is empty\n");
			else {
				Put(usr, "\n<green>");
				print_namelist(usr, &(usr->override));
			}
			Put(usr, "<magenta>\n"
				" <hotkey>Add override");

			if (usr->override.len <= 0)
				Put(usr, "\n");
			else
				Put(usr, "                <hotkey>Remove override\n");

			if (usr->override.len > 0)
				Put(usr, " <hotkey>Clean out\n");

			Put(usr, "\n"
				" Switch to <hotkey>friend list       Switch to <hotkey>enemy list\n");
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Overrides%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				*menu = EXIT_MENU;
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'a':
			case 'A':
			case '+':
			case '=':
				Put(usr, "Add override\n");

				if (cstrcount(usr->override.str, ',') >= MAX_FRIENDS-1)
					Print(usr, "<red>You already have %d overrides defined\n", MAX_FRIENDS);
				else
					add_override(usr);
				break;

			case 'r':
			case 'R':
			case 'd':
			case 'D':
			case '-':
			case '_':
				if (usr->override.len <= 0) {
					print_menu = 0;
					break;
				}
				Put(usr, "Remove override\n");
				remove_override(usr);
				break;

			case 'c':
			case 'C':
				if (usr->override.len <= 0) {
					print_menu = 0;
					break;
				}
				Put(usr, "Clean out\n");
				reset_GString(&(usr->override));
				break;

			case '>':
			case 'f':
			case 'F':
				Put(usr, "Friends\n");
				*menu = FRIENDS_MENU;
				return;

			case '<':
			case 'e':
			case 'E':
				Put(usr, "Enemies\n");
				*menu = ENEMIES_MENU;
				return;

			default:
				print_menu = 0;
		}
	}
}

void add_override(User *usr) {
char name[MAX_NAME];

	Put(usr, "<green>Enter name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "\n<green>You may always send yourself messages\n");
		return;
	}
	if (cstrfind(usr->override.str, name, ',')) {
		Print(usr, "\n<yellow>%s<red> already is on your override list\n", name);
		return;
	}
	if (cstrfind(usr->enemies.str, name, ',')) {
		Print(usr, "\n<red>But <yellow>%s<red> is on your enemy list!\n", name);
		return;
	}
	if (!user_exists(name)) {
		Print(usr, "\n<red>No such user\n");
		return;
	}
	if (usr->override.len > 0)
		gstrcat(&(usr->override), ",");

	gstrcat(&(usr->override), name);
}

void remove_override(User *usr) {
char name[MAX_NAME];

	Put(usr, "<green>Enter name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "\n<green>You may always send yourself messages\n");
		return;
	}
	if (!cstrfind(usr->override.str, name, ',')) {
		Put(usr, "\n<red>There is no such person on your override list\n");
		return;
	}
	cstrremove(usr->override.str, name, ',');
	usr->override.len = strlen(usr->override.str);

	shrink_GString(&(usr->override));
}

void print_namelist(User *usr, GString *g) {
char *str, **arr;
int count;

	if (g->len <= 0)
		return;

	if ((str = cstrdup(g->str)) == NULL) {
		Put(usr, "<red>Out of memory\n");
		return;
	}
	if ((arr = cstrsplit(str, ',', &count)) == NULL) {
		Put(usr, "<red>Out of memory\n");
		Free(str);
		return;
	}
	if (count >= 1) {
		int i, j, rows, columns, n;

		qsort(arr, count, sizeof(char *), (int (*)(const void *, const void *))strcmp);
/*
	print columns
*/
		columns = 4;
		rows = count / columns;
		if (count % columns)
			rows++;

		n = 0;
		for(j = 0; j < rows; j++) {
			n = j;
			for(i = 0; i < columns; i++) {
				Print(usr, " %-18s", arr[n]);

				n += rows;
				if (n >= count)
					break;
			}
			Put(usr, "\n");
		}
	}
	Free(str);
	Free(arr);
}

/* EOB */
