/*
	roomlevel.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "roomlevel.h"
#include "online.h"
#include "log.h"
#include "Memory.h"
#include "edit.h"
#include "XMsg.h"
#include "cstring.h"
#include "login.h"
#include "keys.h"
#include "access.h"
#include "util.h"
#include "backend.h"
#include "config_menu.h"
#include "bufprintf.h"
#include "colorize.h"
#include "friends_menu.h"
#include "version.h"
#include "passwd.h"
#include "Stats.h"
#include "msg_system.h"
#include "sysop_menu.h"
#include "pager.h"

#include <stdio.h>
#include <stdlib.h>


void roomlevel(User *usr) {
int c, reprint_prompt = 1;

	for(;;) {
		if (received_new_XMsgs(usr))
			reprint_prompt = 1;
/*
	print room prompt and get input command
*/
		if (reprint_prompt)
			Print(usr, "\n<yellow>Lobby%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		reprint_prompt = 1;

		usr->runtime_flags &= ~RTF_BUSY;
		c = iGetch(usr);

		switch(c) {
			case IO_ERR:
				linkdead(usr);
				return;

			case IO_EINTR:					/* interrupted, no input ready */
				reprint_prompt = 0;
				break;

			case KEY_RETURN:
				break;

			case ' ':
				Put(usr, "Goto");
				break;

			case 'c':
			case 'C':
				Put(usr, "<red>Press<yellow> <Ctrl-C><red> or<yellow> <Ctrl-O><red> to access the Config menu\n");
				break;

			case KEY_CTRL('C'):				/* this don't work for some people (?) */
			case KEY_CTRL('O'):				/* so I added Ctrl-O by special request */
				Put(usr, "<white>Config menu\n");
				config_menu(usr);
				break;

			case 'h':
			case 'H':
			case '?':
				Put(usr, "Help\n\n");
				display_file(usr, HELP_FILE);
				break;

			case 'l':
				Put(usr, "Logout\n");
				logout(usr);
				break;

			case KEY_CTRL('L'):
				Put(usr, "Lock screen\n");
				lock_screen(usr);
				break;

			case 'p':
			case 'P':
				Put(usr, "Profile\n");
				profile(usr);
				break;

			case KEY_CTRL('P'):
				Put(usr, "Ping\n");
				ping(usr);
				break;

			case KEY_CTRL('S'):
				if (usr->runtime_flags & RTF_SYSOP) {
					Put(usr, "Sysop menu\n");
					sysop_menu(usr);
				}
				break;

			case 't':
				Put(usr, "Time\n");
				date(usr);
				break;

			case 'w':
				Put(usr, "Who\n");
				wholist(usr, (usr->flags & USR_SHORT_WHO) ? WHO_SHORT : WHO_LONG);
				break;

			case 'W':
				Put(usr, "Who\n");
				wholist(usr, (usr->flags & USR_SHORT_WHO) ? WHO_LONG : WHO_SHORT);
				break;

			case KEY_CTRL('W'):
				Put(usr, "Customize Who list\n");
				config_who(usr);
				break;

			case KEY_CTRL('F'):
				Put(usr, "Online friends\n");
				online_friends(usr);
				break;

			case KEY_CTRL('T'):
				Put(usr, "Talked to\n");
				online_talked_to(usr);
				break;

			case 'x':
				Put(usr, "eXpress Message\n");
				enter_xmsg(usr);
				break;

			case ':':
			case ';':
				Put(usr, "Emote\n");
				enter_emote(usr);
				break;

			case 'o':
				Put(usr, "Override\n");
				if (!(usr->flags & USR_X_DISABLED)) {
					Put(usr, "<red>Override is non-functional when you are able to receive messages\n");
					break;
				}
				config_overrides(usr);
				break;

			case 'S':
				Put(usr, "Statistics\n");
				print_stats(usr);
				break;

			case 'X':
			case '*':
				Put(usr, "Toggle message reception\n");
				usr->flags ^= USR_X_DISABLED;
				user_dirty(usr, "flags");

				reset_GString(&(usr->override));

				Print(usr, "<magenta>Message reception is now turned <yellow>%s\n",
					(usr->flags & USR_X_DISABLED) ? "off" : "on");

				if (usr->flags & USR_X_DISABLED) {
					if (usr->flags & USR_HELPING_HAND) {
						usr->flags &= ~USR_HELPING_HAND;
						usr->runtime_flags |= RTF_WAS_HH;
						Put(usr, "<magenta>You are no longer available to help others\n");
					}
				} else {
					if (usr->runtime_flags & RTF_WAS_HH) {
						usr->flags |= USR_HELPING_HAND;
						usr->runtime_flags &= ~RTF_WAS_HH;
						Put(usr, "<magenta>You are now available to help others\n");
					}
				}
				break;

			case '$':
				if (!is_sysop(usr->name)) {
					Print(usr, "Time\n");
					date(usr);
					break;
				}
				if (usr->runtime_flags & RTF_SYSOP) {
					usr->runtime_flags &= ~RTF_SYSOP;
					Print(usr, "Exiting Sysop mode\n");
					break;
				} else {
					Put(usr, "Sysop mode\n");
					enter_sysop_mode(usr);
				}
				break;

			case '[':
			case ']':
				Put(usr, "Version information\n");
				print_version_info(usr);
				break;

			case '>':
				Put(usr, "Friends\n");
				config_friends(usr);
				break;

			case '<':
				Put(usr, "Enemies\n");
				config_enemies(usr);
				break;

			default:
				;
		}
	}
}

void date(User *usr) {
time_t now, t;
struct tm *tm;
char buf[3];
int today, today_month, today_year, old_month, green, week, day;

	now = time(NULL);
	tm = localtime(&now);

	sprint_numberth(tm->tm_mday, buf);

	Print(usr, "<magenta>Current time is <yellow>%sday, %s %d%s %04d %02d:%02d:%02d\n",
		Days[tm->tm_wday], Months[tm->tm_mon], tm->tm_mday, buf, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);

/* print calendar */
	Put(usr, "\n<magenta>  S  M Tu  W Th  F  S\n");

	today = tm->tm_mday;
	today_month = tm->tm_mon;
	today_year = tm->tm_year;

	t = now - (14 + tm->tm_wday) * SECS_IN_DAY;
	tm = localtime(&t);
	old_month = tm->tm_mon;

	Put(usr, "<green>");
	green = 1;

	for(week = 0; week < 5; week++) {
		for(day = 0; day < 7; day++) {
			tm = localtime(&t);

			if  (tm->tm_mday == today && tm->tm_mon == today_month && tm->tm_year == today_year)
				Print(usr, "<white> %2d<%s>", tm->tm_mday, (green == 0) ? "yellow" : "green");
			else {
				if (old_month != tm->tm_mon) {
					green ^= 1;
					old_month = tm->tm_mon;

					Put(usr, (green == 1) ? "<green>" : "<yellow>");
				}
				Print(usr, " %2d", tm->tm_mday);
			}
			t += SECS_IN_DAY;
		}
		Put(usr, "\n");
	}
}

int sort_who_asc_byname(void *p1, void *p2) {
WhoListEntry *a, *b;

	a = (WhoListEntry *)p1;
	b = (WhoListEntry *)p2;

	return strcmp(a->name, b->name);
}

int sort_who_desc_byname(void *p1, void *p2) {
WhoListEntry *a, *b;

	a = (WhoListEntry *)p1;
	b = (WhoListEntry *)p2;

	return -strcmp(a->name, b->name);
}

int sort_who_asc_bytime(void *p1, void *p2) {
WhoListEntry *a, *b;

	a = (WhoListEntry *)p1;
	b = (WhoListEntry *)p2;

	if (a->online_time < b->online_time)
		return -1;

	if (a->online_time > b->online_time)
		return 1;

	return 0;
}

int sort_who_desc_bytime(void *p1, void *p2) {
WhoListEntry *a, *b;

	a = (WhoListEntry *)p1;
	b = (WhoListEntry *)p2;

	if (a->online_time < b->online_time)
		return 1;

	if (a->online_time > b->online_time)
		return -1;

	return 0;
}

void wholist_status(User *usr, WhoListEntry *who, char *status, char *color) {
	*status = ' ';

	if (!strcmp(who->name, usr->name))
		cstrcpy(color, "white", MAX_COLORBUF);
	else
		cstrcpy(color, "yellow", MAX_COLORBUF);

	if (cstrfind(usr->enemies.str, who->name, ',')) {
		cstrcpy(color, "red", MAX_COLORBUF);

		if (!(usr->display.flags & DISPLAY_ANSI))
			*status = '-';
	} else {
		if (cstrfind(usr->friends.str, who->name, ',')) {
			cstrcpy(color, "green", MAX_COLORBUF);

			if (!(usr->display.flags & DISPLAY_ANSI))
				*status = '+';
		}
	}
	if (who->flags & USR_HELPING_HAND)
		*status = '%';

	if (who->runtime_flags & RTF_SYSOP)
		*status = '$';

	if (who->runtime_flags & RTF_HOLD)
		*status = 'b';

	if (who->flags & USR_X_DISABLED)
		*status = '*';

	if (who->runtime_flags & RTF_LOCKED)
		*status = '#';
}

void long_who_list(User *usr, WhoListEntry *who, int num) {
char buf[PRINT_BUF], color[MAX_COLORBUF], status;
int i, hrs, mins, l, c, width, max;

/* 36 is the length of the string with the stats that's to be added at the end, plus margin */
	max = sizeof(buf) - 36;

	for(i = 0; i < num; i++) {
		hrs = who[i].online_time / 3600;
		mins = (who[i].online_time % 3600) / 60;

		wholist_status(usr, &who[i], &status, color);

		width = (usr->display.term_width > max) ? max : usr->display.term_width;

		if (who[i].doing == NULL || !who[i].doing[0])
			bufprintf(buf, sizeof(buf), "<%s>%s<cyan>", color, who[i].name);
		else
			bufprintf(buf, sizeof(buf), "<%s>%s <cyan>%s", color, who[i].name, who[i].doing);

		l = color_index(&(usr->display), buf, width - 9);
		buf[l] = 0;

		c = color_strlen(&(usr->display), buf);
		while(c < (width-9) && l < max) {
			buf[l++] = ' ';
			c++;
		}
		buf[l] = 0;

		Print(usr, "%s <white>%c <yellow>%2d:%02d\n", buf, status, hrs, mins);
	}
}

void short_who_list(User *usr, WhoListEntry *who, int num) {
int i, j, rows, columns, n;
char status, color[MAX_COLORBUF];

	columns = 4;
	rows = num / columns;
	if (num % columns)
		rows++;

	Put(usr, "<yellow>");

/*
	print short who-list in sorted columns
*/
	n = 0;
	for(j = 0; j < rows; j++) {
		n = j;
		for(i = 0; i < columns; i++) {
			wholist_status(usr, &who[i], &status, color);

			Print(usr, "<white>%c<%s>%-18s", status, color, who[n].name);

			n += rows;
			if (n >= num)
				break;
		}
		Put(usr, "\n");
	}
}

void wholist(User *usr, int format) {
WhoListEntry *who;
int num;
time_t now;
struct tm *tm;
char buf[MAX_HALFLINE];
void *sort_func;

	if ((who = get_online_list(&num)) == NULL) {
		Put(usr, "<red>error: failed to see who is online\n");
		return;
	}
	if (usr->flags & USR_SORT_BYNAME) {
		if (usr->flags & USR_SORT_DESCENDING)
			sort_func = (void *)sort_who_desc_byname;
		else
			sort_func = (void *)sort_who_asc_byname;
	} else {
		if (usr->flags & USR_SORT_DESCENDING)
			sort_func = (void *)sort_who_desc_bytime;
		else
			sort_func = (void *)sort_who_asc_bytime;
	}
/*
	filter out enemies
*/
	if ((usr->flags & USR_HIDE_ENEMIES) && usr->enemies.len > 0) {
		int i;

		for(i = 0; i < num;) {
			if (cstrfind(usr->enemies.str, who[i].name, ',')) {
				memmove(&who[i], &who[i+1], (num - i) * sizeof(WhoListEntry));
				num--;
				continue;
			}
			i++;
		}
	}
	qsort(who, num, sizeof(WhoListEntry), sort_func);

	now = time(NULL);
	tm = localtime(&now);
	sprint_time(tm, usr->flags & USR_12HRCLOCK, buf, sizeof(buf));

	if (num == 1)
		Print(usr, "<magenta>\nYou are the only lonely user online at <yellow>%s\n", buf);
	else
		Print(usr, "<magenta>\nThere are <yellow>%d<magenta> users online at <yellow>%s\n", num, buf);

	if (format == WHO_LONG) {
		Put(usr, "<white>");
		hline(usr, '-');
		long_who_list(usr, who, num);
	} else
		short_who_list(usr, who, num);

	Free(who);
}

void online_friends(User *usr) {
WhoListEntry *who;
int num, num_online, i;
time_t now;
struct tm *tm;
char buf[MAX_HALFLINE];

	if (usr->friends.len <= 0) {
		Put(usr, "<cyan>Your friend list is empty\n");
		return;
	}
	if ((who = get_online_list(&num)) == NULL) {
		Put(usr, "<red>error: failed to see who is online\n");
		return;
	}
	qsort(who, num, sizeof(WhoListEntry), (void *)sort_who_asc_byname);
/*
	this is probably hopelessly inefficient for short friendlists and many users
	online ... on the other hand, I don't want to call is_online() in a loop
	because it locks the wholist multiple times
	and what if you have a large friendlist and there are few users online...?
*/
	num_online = 0;
	if (num > 1) {
		for(i = 0; i < num;) {
			if (!cstrfind(usr->friends.str, who[i].name, ',')) {
				memmove(&(who[i]), &(who[i+1]), (num-i) * sizeof(WhoListEntry));
				num--;
			} else {
				num_online++;
				i++;
			}
		}
	}
	if (num_online <= 0) {
		Put(usr, "<cyan>None of your friends are online\n");
		Free(who);
		return;
	}
	now = time(NULL);
	tm = localtime(&now);
	sprint_time(tm, usr->flags & USR_12HRCLOCK, buf, sizeof(buf));

	if (num_online == 1)
		Print(usr, "<magenta>\nThere is one friend online at <yellow>%s\n", buf);
	else
		Print(usr, "<magenta>\nThere are <yellow>%d<magenta> friends online at <yellow>%s\n", num_online, buf);

	short_who_list(usr, who, num);

	Free(who);
}

void online_talked_to(User *usr) {
WhoListEntry *who;
int num, num_online, i;
time_t now;
struct tm *tm;
char buf[MAX_HALFLINE];

	if (usr->talked_to.len <= 0) {
		Put(usr, "<cyan>You have not talked to anyone yet\n");
		return;
	}
	if ((who = get_online_list(&num)) == NULL) {
		Put(usr, "<red>error: failed to see who is online\n");
		return;
	}
	qsort(who, num, sizeof(WhoListEntry), (void *)sort_who_asc_byname);
/*
	this is probably hopelessly inefficient for short talked_to lists and many users
	online ... on the other hand, I don't want to call is_online() in a loop
	because it locks the wholist multiple times
*/
	num_online = 0;
	for(i = 0; i < num;) {
		if (!cstrfind(usr->talked_to.str, who[i].name, ',')) {
			memmove(&(who[i]), &(who[i+1]), (num-i) * sizeof(WhoListEntry));
			num--;
		} else {
			num_online++;
			i++;
		}
	}
	if (num_online <= 0) {
		Put(usr, "<cyan>Nobody you talked to is still online\n");
		Free(who);
		return;
	}
	now = time(NULL);
	tm = localtime(&now);
	sprint_time(tm, usr->flags & USR_12HRCLOCK, buf, sizeof(buf));

	if (num_online == 1)
		Print(usr, "<magenta>\nThere is one person you talked to online at <yellow>%s\n", buf);
	else
		Print(usr, "<magenta>\nThere are <yellow>%d<magenta> people you talked to online at <yellow>%s\n", num_online, buf);

	short_who_list(usr, who, num);

	Free(who);
}

void ping_name(User *usr, char *name) {
User user;

	if (!user_exists(name)) {
		Print(usr, "<red>No such user <yellow>%s\n", name);
		remove_recipient(usr, name);
		return;
	}
	if (!strcmp(usr->name, name)) {
		Put(usr, "<green>You are keeping yourself busy pinging yourself\n");
		return;
	}
	init_User(&user);
	if (!is_online(name, &user)) {
		deinit_User(&user);
		Print(usr, "<yellow>%s<red> is not online\n", name);
		remove_recipient(usr, name);
		return;
	}
	if (user.runtime_flags & RTF_LOCKED) {
		if (user.away != NULL && user.away[0])
			Print(usr, "<yellow>%s<green> is away from the terminal; %s\n", name, user.away);
		else
			Print(usr, "<yellow>%s<green> is away from the terminal\n", name);
	} else
		Print(usr, "<yellow>%s<green> is %sbusy\n", name, (user.runtime_flags & RTF_BUSY) ? "" : "not ");

	deinit_User(&user);
}

void ping(User *usr) {
char *endp, *p, name[MAX_NAME];

	if (edit_recipients(usr, multi_ping_access) == -1)
		return;

	if (usr->recipients.len <= 0)
		return;

	p = usr->recipients.str;
	while((endp = cstrchr(p, ',')) != NULL) {
		*endp = 0;
		cstrcpy(name, p, MAX_NAME);
		*endp = ',';
		endp++;
		p = endp;

		ping_name(usr, name);
	}
	cstrcpy(name, p, MAX_NAME);

	ping_name(usr, name);
}

void enter_sysop_mode(User *usr) {
char pw[MAX_LINE];

	Put(usr, "<green>Enter password: ");
	if (edit_password(usr, pw, MAX_LINE) == -1)
		return;

	if (check_password(usr->name, pw)) {
		Put(usr, "<red>Wrong password\n");
		return;
	}
	if (!is_sysop(usr->name)) {		/* double check */
		Put(usr, "<red>Sorry, but you are no longer <yellow>Sysop<red> here\n");
		return;
	}
	usr->runtime_flags |= RTF_SYSOP;
	Put(usr, "\n<red>*** <white>NOTE: You are now in <yellow>Sysop<white> mode <red>***\n");
}

void lock_screen(User *usr) {
char buf[MAX_LINE];

	if (!(usr->flags & USR_NO_AWAY_REASON)) {
		Put(usr, "<green>Enter reason: <yellow>");
		edit_line(usr, buf, MAX_LINE);

		cstrip_line(buf);

		if (buf[0]) {
			cstrfree(usr->away);
			usr->away = cstrdup(buf);
		}
	}
	clear_screen(usr);

	Put(usr, "\n<white>bbs101 terminal locked");
	if (usr->away != NULL)
		Print(usr, ";<yellow> %s", usr->away);

	usr->runtime_flags |= RTF_LOCKED;

	notify_friends(usr->name, "is away from the terminal for a while");

	for(;;) {
		Put(usr, "\n<red>Enter password to unlock: ");

		if (edit_password(usr, buf, MAX_LINE) == -1)
			continue;

		if (check_password(usr->name, buf))
			Put(usr, "Wrong password\n");
		else
			break;
	}
	Put(usr, "<yellow>Unlocked\n");

	usr->runtime_flags &= ~RTF_LOCKED;
	cstrfree(usr->away);
	usr->away = NULL;

	notify_friends(usr->name, "has returned to the terminal");
}

void profile(User *usr) {
char name[MAX_NAME], buf[MAX_LINE];
User *profile_user, user_obj, *u;
int printed;

	Put(usr, "<green>Enter name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(name, "Sysop")) {
		char *sysops;

		Put(usr, "\n");

		if ((sysops = get_sysops()) == NULL) {
			Put(usr, "<red>There are no Sysops on this BBS\n");
			return;
		}
		if (cstrchr(sysops, ',') == NULL)
			Print(usr, "<green>Sysop is: <yellow>%s\n", sysops);
		else
			Print(usr, "<green>Sysops are: <yellow>%s\n", sysops);

		Free(sysops);
		return;
	}
	if (!user_exists(name)) {
		Put(usr, "<red>No such user\n");
		return;
	}
	profile_user = &user_obj;
	init_User(profile_user);

	if (load_User_profile(profile_user, name) == -1) {
		Put(usr, "<red>Sorry, failed to load user profile\n");
		deinit_User(profile_user);
		return;
	}
	Put(usr, "\n");

	init_pager(usr);

	printed = 0;
	Put(usr, "<white>");

	if (profile_user->vanity != NULL && profile_user->vanity[0]) {
		char fmt[16];

		bufprintf(fmt, sizeof(fmt), "%%-%ds", MAX_NAME + 10);
		Print(usr, fmt, profile_user->name);
		Print(usr, "<magenta>* <white>%s <magenta>*", profile_user->vanity);
	} else {
		Put(usr, profile_user->name);
		Put(usr, "\n");
	}
/*
	option: hide address info from non-friends
	even Sysop can't see this
*/
	if (!(profile_user->flags & USR_HIDE_ADDRESS) || cstrfind(profile_user->friends.str, usr->name, ',') || !strcmp(usr->name, name)) {
		char prefix[16];

		if ((profile_user->flags & USR_HIDE_ADDRESS) && !strcmp(usr->name, name))
			cstrcpy(prefix, "<white>hidden> ", sizeof(prefix));
		else
			prefix[0] = 0;

		if (profile_user->real_name != NULL && profile_user->real_name[0]) {
			Print(usr, "%s<yellow>%s\n", prefix, profile_user->real_name);
			printed++;
		}
		if (profile_user->city != NULL && profile_user->city[0]) {
			Print(usr, "%s<yellow>%s\n", prefix, profile_user->city);
			printed++;
		}
		if (profile_user->state != NULL && profile_user->state[0]) {
			if (profile_user->country != NULL && profile_user->country[0])
				Print(usr, "%s<yellow>%s, %s\n", prefix, profile_user->state, profile_user->country);
			else
				Print(usr, "%s<yellow>%s\n", prefix, profile_user->state);
			printed++;
		} else {
			if (profile_user->country != NULL && profile_user->country[0]) {
				Print(usr, "%s<yellow>%s\n", prefix, profile_user->country);
				printed++;
			}
		}
		if (printed) {
			Put(usr, "\n");
			printed = 0;
		}
		if (profile_user->email != NULL && profile_user->email[0]) {
			Print(usr, "%s<green>E-mail: <cyan>%s\n", prefix, profile_user->email);
			printed++;
		}
		if (profile_user->www != NULL && profile_user->www[0]) {
			Print(usr, "%s<green>WWW: <cyan>%s\n", prefix, profile_user->www);
			printed++;
		}
		if (printed) {
			Put(usr, "\n");
			printed = 0;
		}
	}
	if (profile_user->doing != NULL && profile_user->doing[0]) {
		Print(usr, "<white>%s <cyan>%s\n", name, profile_user->doing);
		printed++;
	}
	if (printed) {
		Put(usr, "\n");
		printed = 0;
	}
/*
	display last online time
*/
	if (!strcmp(usr->name, name)) {
		Print(usr, "<green>Online for <yellow>%s\n", sprint_total_time(time(NULL) - usr->login_time, buf, sizeof(buf)));

		if (usr->runtime_flags & RTF_SYSOP)
			Print(usr, "<green>You are connected from <yellow>%s\n", usr->hostname);

		Print(usr, "<green>Total online time: <yellow>%s\n", sprint_total_time(usr->total_time + (time(NULL) - usr->login_time), buf, sizeof(buf)));
	} else {
		if ((u = lock_user(name)) != NULL) {
			Print(usr, "<green>Online for <yellow>%s\n", sprint_total_time(time(NULL) - u->login_time, buf, sizeof(buf)));

			if (usr->runtime_flags & RTF_SYSOP)
				Print(usr, "<yellow>%s <green>is connected from <yellow>%s\n", u->name, u->hostname);

			Print(usr, "<green>Total online time: <yellow>%s\n", sprint_total_time(u->total_time + (time(NULL) - u->login_time), buf, sizeof(buf)));

			unlock_user(u);
		} else {
			struct tm *tm;

			tm = localtime((time_t *)&profile_user->last_logout);
			Print(usr, "<green>Last online on <cyan>%s ", sprint_date(tm, usr->flags & USR_12HRCLOCK, buf, sizeof(buf)));
			Print(usr, "<green>for <yellow>%s\n", sprint_total_time(profile_user->last_online_time, buf, sizeof(buf)));

			Print(usr, "<green>Total online time: <yellow>%s\n", sprint_total_time(profile_user->total_time, buf, sizeof(buf)));

			if (usr->runtime_flags & RTF_SYSOP)
				Print(usr, "<yellow>%s <green>was connected from <yellow>%s\n", profile_user->name, usr->last_from);
		}
	}
	Put(usr, "\n");
	printed = 0;
/*
	print friend state
*/
	if (profile_user->flags & USR_X_DISABLED) {
		printed++;

		if ((!(profile_user->flags & USR_BLOCK_FRIENDS) && cstrfind(profile_user->friends.str, usr->name, ','))
			|| cstrfind(profile_user->override.str, usr->name, ','))
			Print(usr, "<yellow>%s <green>has message reception turned off, but is accepting messages from you\n", profile_user->name);
		else
			Print(usr, "<yellow>%s <red>has message reception turned off\n");
	} else {
		if (cstrfind(profile_user->enemies.str, usr->name, ',')) {
			Print(usr, "<yellow>%s <red>does not wish to receive any messages from you\n", profile_user->name);
			printed++;
		}
	}
	if (printed) {
		Put(usr, "\n");
		printed = 0;
	}
/*
	print profile info
*/
	if (profile_user->info != NULL && profile_user->info[0]) {
		char *p, *text;

		Put(usr, "<green>");
/*
	sigh
	You can't just throw a large block of text in Put(), because its buffers will overflow
	so we break the profile info up in lines and display them line by line
*/
		p = text = profile_user->info;
		while((p = cstrchr(text, '\n')) != NULL) {
			*p = 0;
			Put(usr, text);
			Put(usr, "\n");
			*p = '\n';
			p++;
			text = p;
		}
		if (text[0]) {
			Put(usr, text);
			Put(usr, "\n");
		}
	} else
		Print(usr, "<red>%s has an empty profile\n", name);

	deinit_User(profile_user);

	pager(usr);
}

/* EOB */
