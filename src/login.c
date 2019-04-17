/*
	login.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "login.h"
#include "edit.h"
#include "cstring.h"
#include "online.h"
#include "util.h"
#include "bufprintf.h"
#include "log.h"
#include "backend.h"
#include "passwd.h"
#include "util.h"
#include "Memory.h"
#include "Stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


static char *really_logout[] = {
	"Really logout",
	"Are you sure",
	"Are you sure you are sure",
	"Are you sure you want to logout",
	"Do you really wish to logout",
	"Really logout from the BBS"
};


void login(User *usr) {
char name[MAX_NAME];
User *u;
int i;

	Put(usr, "\nWelcome!\n");

	for(i = 0; i < MAX_LOGIN_ATTEMPTS; i++) {
		if (i > 0)
			Put(usr, "\nPress Ctrl-D to exit\n");

		Put(usr, "\nEnter your name: ");

		if (edit_name(usr, name) == -1) {
			Put(usr, "\nBye, and have a nice life!\n\n");
			close_connection(usr, "user hit break on the login prompt");
			return;
		}
		if (!name[0])
			continue;

		if (!strcmp(name, "New")) {
			if (new_user(usr, name) == -1)
				continue;
		} else
			if (!user_exists(name)) {
				if (ask_new_user(usr, name) == -1)
					continue;
			} else
				if (login_password(usr, name) == -1)
					continue;
/*
	user was already logged in
*/
		if ((u = lock_user(name)) != NULL) {
			Put(u, "\n\n<red>Connection killed by another login\n\n");
			beep(u);
			reset_colors(u);

			log_auth("CLOSE %s (%s): connection killed by another login from %s", u->name, u->hostname, usr->hostname);
			locked_remove_online_user(u);
			save_User(u);
			u->name[0] = 0;
			u->runtime_flags |= RTF_DEADCONN;	/* signal dead connection */
			wakeup(u);							/* wake up (it's time to go) */
			unlock_user(u);
			u = NULL;

			load_User(usr, name);
		} else
			load_User(usr, name);

		usr->login_time = time(NULL);
		usr->logins++;
		save_User_int(usr, "logins", usr->logins);

		add_online_user(usr);

		if (usr->logins > 1) {
			char add[3];

			sprint_numberth(usr->logins, add);
			Print(usr, "\n<green>Welcome back, %s! This is your <yellow>%u%s<green> login\n", usr->name, usr->logins, add);

			go_online(usr);
		} else {
			save_User_int(usr, "birth", usr->birth);
			Print(usr, "\n<green>Hello, %s!\n", usr->name);
		}
		inc_stats(STATS_LOGINS);
		update_stats(usr);
		return;
	}
	Put(usr, "\n\nBye! Come back when you've figured it out..!\n\n");
	close_connection(usr, "too many attempts");
}

int login_password(User *usr, char *name) {
char buf[MAX_LINE];

	Put(usr, "Enter password: ");
	if (edit_password(usr, buf, MAX_LINE) == -1) {
		Put(usr, "\n");
		return -1;
	}
	if (check_password(name, buf)) {
		Put(usr, "Wrong password\n");
		return -1;
	}
	return 0;
}

void logout(User *usr) {
char buf[MAX_LINE];

	bufprintf(buf, sizeof(buf), "<cyan>%s? (y/N): ", RND_STR(really_logout));

	if (yesno(usr, buf, 'N') == YESNO_YES) {
		Put(usr, "\nBye!\n\n");

		notify_friends(usr->name, "explodes into <yellow>golden<magenta> stardust");
		close_connection(usr, "logout");
	}
}

int new_user(User *usr, char *name) {
	Put(usr, "\nHello there, new user! You may choose a name that suits you well.\n"
		"This name will be your alias for the rest of your BBS life.\n"
		"Enter your login name: ");

	while(1) {
		if (edit_name(usr, name) == -1 || !*name)
			return -1;

		if (!name[1]) {
			Put(usr, "\nThat name is too short\n"
				"Enter your login name: ");
			continue;
		}
		if (!strcmp(name, "New") || !strcmp(name, "Sysop") || !strcmp(name, "Guest")) {
			Print(usr, "\nYou can not use '%s' as login name, choose an other login name\n"
				"Enter your login name: ", name);
			continue;
		}
		if (user_exists(name)) {
			Put(usr, "\nThat name already is in use, please choose an other login name\n"
				"Enter your login name: ");
			continue;
		}
		break;
	}
	usr->birth = time(NULL);

	Put(usr, "\nNow to choose a password. Passwords can be 79 characters long and can contain\n"
		"spaces and punctuation characters. Be sure not to use a password that can be\n"
		"guessed easily by anyone. Also be sure not to forget your own password..!\n");

	if (enter_new_password(usr, name) == -1)
		return -1;

	return 0;
}

int ask_new_user(User *usr, char *name) {
	Put(usr, "No such user. ");

	if (yesno(usr, "Do you wish to create a new user? (y/N): ", 'N') != YESNO_YES)
		return -1;

	usr->birth = time(NULL);

	if (enter_new_password(usr, name) == -1)
		return -1;

	return 0;
}

int enter_new_password(User *usr, char *name) {
char pass1[MAX_LINE], pass2[MAX_LINE];
int shorty;

	shorty = 0;
	while(1) {
		Put(usr, "Enter new password: ");

		if (edit_password(usr, pass1, MAX_LINE) == -1)
			return -1;

		if (strlen(pass1) < PASSWD_MIN_LEN) {
			shorty++;
			Print(usr, "That password is %stoo short\n\n", (shorty <= 1) ? "" : "also ");
			continue;
		}
		if (!cstricmp(name, pass1)) {
			Put(usr, "Sorry, but that password is not good enough\n\n");
			continue;
		}
		Put(usr, "Enter it again (for verification): ");

		if (edit_password(usr, pass2, MAX_LINE) == -1)
			continue;

		if (strcmp(pass1, pass2)) {
			Put(usr, "\nPasswords didn't match! Please try again\n\n");
			continue;
		}
		memset(pass2, 0, MAX_LINE);
		break;
	}
/*
	Note: it looks like there is a race condition here, but that is not true;
	create_user() will fail if the name is already taken (a username is UNIQUE)
	It's just that I like seeing an appropriate error message being displayed
	to the end user, and this increases the chance of doing so
*/
	if (user_exists(name)) {
		Print(usr, "\nSorry, but in the meantime, someone has taken the username '%s'\n\n", name);
		return -1;
	}
/*
	set some defaults
*/
	usr->flags |= USR_SORT_DESCENDING;		/* long wholist, sort by time, descending */

	if (create_user(name, pass1) == -1) {
		Put(usr, "\nFailed to create user\n\n");
		return -1;
	}
	memset(pass1, 0, MAX_LINE);
	return 0;
}

void go_online(User *usr) {
char buf[MAX_LINE];
int num_online;
WhoListEntry *who;

	if (!usr->birth) {					/* should never happen, really */
		usr->birth = time(NULL);
		user_dirty(usr, "birth");
	}
	if (usr->last_logout > 0) {
		struct tm *tm;

		tm = localtime((time_t *)&(usr->last_logout));
		Print(usr, "\n<green>Last login was on <cyan>%s", sprint_date(tm, usr->flags & USR_12HRCLOCK, buf, sizeof(buf)));

		if (usr->last_online_time > 0)
			Print(usr, " <green>for <cyan>%s", sprint_total_time(usr->last_online_time, buf, sizeof(buf)));

		if (usr->last_from != NULL)
			Print(usr, " <green>from <cyan>%s", usr->last_from);

		Put(usr, "\n");
	}
	if ((who = get_online_list(&num_online)) != NULL) {
		if (num_online == 1)
			Put(usr, "<green>You are the one and only user online right now ...\n");
		else {
			int i, num, num_friends, num_others;
/*
	count # of online friends, and optionally hide enemies
*/
			num_friends = 0;
			num = num_online;
			for(i = 0; i < num; i++) {
				if (cstrfind(usr->friends.str, who[i].name, ','))
					num_friends++;
				else
					if ((usr->flags & USR_HIDE_ENEMIES) && cstrfind(usr->enemies.str, who[i].name, ','))
						num_online--;
			}
			Free(who);

			num_online--;		/* skip self */
			num_others = num_online - num_friends;

			if (num_online <= 0)
				Put(usr, "<green>You are the one and only user online right now ...\n");
			else {
				if (num_friends <= 0) {
					if (num_others <= 1)
						Put(usr, "<green>There is one other user online\n");
					else
						Print(usr, "<green>There are <yellow>%d<green> other users online\n", num_others);
				} else {
					if (num_friends == 1)
						Put(usr, "<green>There is one friend ");
					else
						Print(usr, "<green>There are <yellow>%d<green> friends ", num_friends);

					if (num_others <= 0)
						Put(usr, "online\n");
					else {
						if (num_others == 1)
							Put(usr, "and one other user online\n");
						else
							Print(usr, "and <yellow>%d<green> other users online\n", num_others);
					}
				}
			}
		}
	}
	if (usr->flags & USR_HELPING_HAND) {
/*
		if (!is_sysop(usr->name) && usr->total_time / SECS_IN_DAY < PARAM_HELPER_AGE)
			usr->flags &= ~USR_HELPING_HAND;
		else {
*/
			if (usr->flags & USR_X_DISABLED) {
				usr->flags &= ~USR_HELPING_HAND;
				user_dirty(usr, "flags");
			} else
				Put(usr, "<magenta>You are available to help others\n");
/*		}	*/
	}
	if (usr->flags & USR_X_DISABLED)
		Print(usr, "<magenta>Message reception is turned off%s\n", (usr->flags & USR_BLOCK_FRIENDS) ? ", and you are blocking Friends" : "");

	if (usr->reminder != NULL && usr->reminder[0])
		Print(usr, "\n<magenta>Reminder: <yellow>%s\n", usr->reminder);

	notify_friends(usr->name, "is formed from some <yellow>golden<magenta> stardust");
}

/* EOB */
