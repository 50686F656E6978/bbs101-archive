/*
	access.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "access.h"
#include "cstring.h"
#include "online.h"
#include "util.h"
#include "keys.h"
#include "colorize.h"
#include "backend.h"
#include "edit.h"

#include <stdio.h>
#include <stdlib.h>


void access_wait_key(User *usr) {
int c;

	for(;;) {
		c = Getch(usr);

		if (c == KEY_RETURN || c == KEY_ESC || c == KEY_BS || c == ' '
			|| c == KEY_CTRL('C') || c == KEY_CTRL('D'))
			return;
	}
}

int multi_x_access(User *usr, char *name, int prompt) {
User user;
char *msg;

	if (!strcmp(name, usr->name))
		return 1;

	init_User(&user);

	msg = NULL;

	if (!user_exists(name))
		msg = " <white>--> <red>No such user";
	else
	if (!is_online(name, &user))
		msg = " <white>--> <red>is not online";
	else
	if (user.runtime_flags & RTF_LOCKED)
		msg = " <white>--> <red>is away from the terminal";
	else
	if ((user.flags & USR_X_DISABLED) && !(usr->runtime_flags & RTF_SYSOP)
		&& ((user.flags & USR_BLOCK_FRIENDS) || !cstrfind(user.friends.str, usr->name, ','))
		&& !cstrfind(user.override.str, usr->name, ','))
		msg = " <white>--> <red>has turned off messages";
	else
	if (cstrfind(usr->enemies.str, name, ',') && !(usr->runtime_flags & RTF_SYSOP))
		msg = " <white>--> <red>is on your enemy list";
	else
	if (cstrfind(user.enemies.str, usr->name, ',') && !(usr->runtime_flags & RTF_SYSOP))
		msg = " <white>--> <red>has blocked you";

	deinit_User(&user);

	if (msg != NULL) {
		int l;

		if (prompt == ACCESS_NO_PROMPT)
			return 0;

		Put(usr, msg);

		access_wait_key(usr);

		l = color_strlen(&(usr->display), msg);
		erase(usr, l);
		return 0;
	}
	return 1;
}

int multi_ping_access(User *usr, char *name, int prompt) {
User user;
char *msg;

	if (!strcmp(name, usr->name))
		return 1;

	init_User(&user);

	msg = NULL;

	if (!user_exists(name))
		msg = " <white>--> <red>No such user";
	else
		if (!is_online(name, &user))
			msg = " <white>--> <red>is not online";

	deinit_User(&user);

	if (msg != NULL) {
		int l;

		if (prompt == ACCESS_NO_PROMPT)
			return 0;

		Put(usr, msg);

		access_wait_key(usr);

		l = color_strlen(&(usr->display), msg);
		erase(usr, l);
		return 0;
	}
	return 1;
}

/*
	helper function for check_recipients()
*/
static int check_single_recipient(User *usr, char *name) {
User user;

	if (!strcmp(usr->name, name))
		return 0;

	if (!user_exists(name)) {
		Print(usr, "<red>No such user <yellow>%s\n", name);
		remove_recipient(usr, name);
		return -1;
	}
	init_User(&user);

	if (!is_online(name, &user)) {
		Print(usr, "<red>In the meantime, <yellow>%s<red> has logged off\n", name);
		remove_recipient(usr, name);
		deinit_User(&user);
		return -1;
	}
	if (user.runtime_flags & RTF_LOCKED) {
		if (user.away != NULL && user.away[0])
			Print(usr, "<red>Sorry, but <yellow>%s<red> is away from the terminal; %s\n", name, user.away);
		else
			Print(usr, "<red>Sorry, but <yellow>%s<red> is away from the terminal\n", name);

		remove_recipient(usr, name);
		deinit_User(&user);
		return -1;
	}
	if (!(usr->runtime_flags & RTF_SYSOP)) {
		if (cstrfind(usr->enemies.str, name, ',')) {
			Print(usr, "<yellow>%s<red> is on your enemy list\n", name);

			remove_recipient(usr, name);
			deinit_User(&user);
			return -1;
		}
		if (cstrfind(user.enemies.str, usr->name, ',')) {
			Print(usr, "<red>Sorry, but <yellow>%s<red> does not wish to receive messages from you anymore\n", name);

			remove_recipient(usr, name);
			deinit_User(&user);
			return -1;
		}
		if ((user.flags & USR_X_DISABLED)
			&& ((user.flags & USR_BLOCK_FRIENDS) || !cstrfind(user.friends.str, usr->name, ','))
			&& !cstrfind(user.override.str, name, ',')) {
			Print(usr, "<red>Sorry, but <yellow>%s<red> does not wish to receive any messages right now\n", name);

			remove_recipient(usr, name);
			deinit_User(&user);
			return -1;
		}
	}
	deinit_User(&user);
	return 0;
}

void check_recipients(User *usr) {
char name[MAX_NAME], *p, *endp;

	p = usr->recipients.str;
	while((endp = cstrchr(p, ',')) != NULL) {
		*endp = 0;
		cstrcpy(name, p, MAX_NAME);
		*endp = ',';
		endp++;

		if (check_single_recipient(usr, name))
			continue;

		p = endp;
	}
	cstrcpy(name, p, MAX_NAME);
	check_single_recipient(usr, name);
}

/* EOB */
