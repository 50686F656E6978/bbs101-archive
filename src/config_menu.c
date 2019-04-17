/*
	config_menu.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "config_menu.h"
#include "keys.h"
#include "bufprintf.h"
#include "edit.h"
#include "Memory.h"
#include "cstring.h"
#include "backend.h"
#include "passwd.h"
#include "util.h"
#include "pager.h"

#include <stdio.h>
#include <stdlib.h>


void config_menu(User *usr) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			Put(usr, "<magenta>\n"
				" <hotkey>Address information          <hotkey>Options\n"
				" Profile <hotkey>info                 <hotkey>Terminal settings\n"
				" <hotkey>Doing                        Customize <hotkey>Who list\n"
				" <hotkey>Reminder\n"
			);
			Put(usr,
				" <hotkey>Password\n"
			);
		}
		print_menu = 1;

		Print(usr, "\n<yellow>Config%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				Put(usr, "\n");
				save_User(usr);
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'a':
			case 'A':
				Put(usr, "Address\n");
				config_address(usr);
				break;

			case 'i':
			case 'I':
				Put(usr, "Profile info\n");
				config_profile(usr);
				break;

			case 'd':
			case 'D':
				Put(usr, "Doing\n");
				config_doing(usr);
				break;

			case 'r':
			case 'R':
				Put(usr, "Reminder\n");
				config_reminder(usr);
				break;

			case 'o':
			case 'O':
				Put(usr, "Options\n");
				config_options(usr);
				break;

			case 't':
			case 'T':
				Put(usr, "Terminal settings\n");
				config_terminal(usr);
				break;

			case 'w':
			case 'W':
				Put(usr, "Who list\n");
				config_who(usr);
				break;

			case 'p':
			case 'P':
				Put(usr, "Password\n");
				config_password(usr);
				break;

			default:
				print_menu = 0;
		}
	}
}

static char *print_address(char *value, char *alt, char *buf, int buflen) {
	if (value != NULL && *value)
		bufprintf(buf, buflen, "<yellow> %s", value);
	else
		bufprintf(buf, buflen, "<white> <unknown%s>", alt);
	return buf;
}

void config_address(User *usr) {
char buf[MAX_LONGLINE];
int c, print_menu;

	load_User_profile(usr, usr->name);

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			Print(usr, "<magenta>\n"
				" <hotkey>Real name :%s<magenta>\n", print_address(usr->real_name, "", buf, MAX_LONGLINE));

			Print(usr,
				" <hotkey>From      :%s",		print_address(usr->city, " city", buf, MAX_LONGLINE));
			Print(usr, "            %s,",	print_address(usr->state, " state", buf, MAX_LONGLINE));
			Print(usr, " %s<magenta>\n",		print_address(usr->country, " country", buf, MAX_LONGLINE));

			Print(usr, "\n"
				" <hotkey>E-mail    :%s<magenta>\n",			print_address(usr->email, " e-mail address", buf, MAX_LONGLINE));
			Print(usr, " <hotkey>WWW       :%s<magenta>\n",	print_address(usr->www, " WWW address", buf, MAX_LONGLINE));

			Print(usr, "\n"
				" <hotkey>Hide address from non-friends     <white>%s<magenta>\n",
				(usr->flags & USR_HIDE_ADDRESS) ? "Yes" : "No"
			);
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Address%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				Put(usr, "Config menu\n");
				save_User(usr);
				unload_User_profile(usr);
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'r':
			case 'R':
				Put(usr, "Real name\n");
				change_config_string(usr, &usr->real_name, "real_name", MAX_LINE, "<green>Enter your real name: <yellow>");
				break;

			case 'f':
			case 'F':
				Put(usr, "From\n");
				change_config_string(usr, &usr->city, "city",		MAX_LINE, "<green>Enter the city you live in    :<yellow> ");
				change_config_string(usr, &usr->state, "state",		MAX_LINE, "<green>Enter the state you are from  :<yellow> ");
				change_config_string(usr, &usr->country, "country",	MAX_LINE, "<green>Enter your country            :<yellow> ");
				break;

			case 'e':
			case 'E':
				Put(usr, "E-mail address\n");
				change_config_string(usr, &usr->email, "email", MAX_LINE, "<green>Enter your e-mail address:<yellow> ");
				break;

			case 'w':
			case 'W':
				Put(usr, "WWW address\n");
				change_config_string(usr, &usr->www, "www", MAX_LONGLINE, "<green>Enter your WWW address:<yellow> ");
				break;

			case 'h':
			case 'H':
				Put(usr, "Hide address information\n");
				usr->flags ^= USR_HIDE_ADDRESS;
				user_dirty(usr, "flags");
				break;

			default:
				print_menu = 0;
		}
	}
}

void config_profile(User *usr) {
int c, print_menu;
GString gstr;

	load_User_profile(usr, usr->name);

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			Print(usr, "<magenta>\n"
				"<hotkey>View current                 <hotkey>Upload\n"
				"<hotkey>Enter new                    <hotkey>Download\n"
			);
			Put(usr, "\n"
				"Vanit<hotkey>y"
			);
			if (usr->vanity != NULL && usr->vanity[0])
				Print(usr, "                       <magenta>*<white> %s <magenta>*\n", usr->vanity);
			else
				Put(usr, "                       <white>None\n");
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Profile%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				Put(usr, "Config menu\n");
				save_User(usr);
				unload_User_profile(usr);
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'v':
			case 'V':
				Put(usr, "View\n");
				if (usr->info == NULL || !usr->info[0]) {
					Put(usr, "<cyan>Your current profile info is empty\n");
					break;
				}
				Put(usr, "<green>\n");
				page_text(usr, usr->info);
				break;

			case 'e':
			case 'E':
				Put(usr, "Edit\n"
					"<green>\n"
					"Enter new profile info, press<yellow> <return><green> twice to save or press<yellow> <Ctrl-C><green> to abort\n"
				);
				init_GString(&gstr);
				if (edit_text(usr, &gstr, NULL, MAX_PROFILE_LINES) == -1)
					Put(usr, "<red>Profile info not changed\n");
				else {
					cstrfree(usr->info);
					usr->info = gstr.str;
					gstr.str = NULL;
					user_dirty(usr, "info");
					save_User(usr);
					Put(usr, "<cyan>Profile info saved\n");
				}
				deinit_GString(&gstr);
				break;

			case 'u':
			case 'U':
				Put(usr, "Upload\n"
					"<green>\n"
					"Upload new profile info, press<yellow> <Ctrl-C><green> to end\n"
				);
				init_GString(&gstr);
				if (upload_text(usr, &gstr, NULL, MAX_PROFILE_LINES) == -1)
					Put(usr, "<red>Profile info not changed\n");
				else {
					cstrfree(usr->info);
					usr->info = gstr.str;
					gstr.str = NULL;
					user_dirty(usr, "info");
					save_User(usr);
					Put(usr, "<cyan>Profile info saved\n");
				}
				deinit_GString(&gstr);
				break;

			case 'd':
			case 'D':
				Put(usr, "Download\n");
				if (usr->info == NULL || !usr->info[0]) {
					Put(usr, "<cyan>Your current profile info is empty\n");
					break;
				}
				download_text(usr, usr->info);
				break;

			case 'y':
			case 'Y':
				Put(usr, "Vanity\n");
				config_vanity(usr);
				break;

			default:
				print_menu = 0;
		}
	}
}

void config_vanity(User *usr) {
	if (usr->vanity != NULL && usr->vanity[0])
		Print(usr, "<green>Your current vanity flag:<cyan> %s\n", usr->vanity);

	change_config_string(usr, &usr->vanity, "vanity", MAX_LINE, "<green>Enter new vanity flag:<yellow> ");
}

void config_doing(User *usr) {
	if (usr->doing != NULL && usr->doing[0])
		Print(usr, "<green>You are currently doing:<cyan> %s\n", usr->doing);

	change_config_string(usr, &usr->doing, "doing", MAX_LINE, "<green>Enter new Doing:<yellow> ");
}

void config_reminder(User *usr) {
	if (usr->reminder != NULL && usr->reminder[0])
		Print(usr, "<green>Current reminder:<cyan> %s\n", usr->reminder);

	change_config_string(usr, &usr->reminder, "reminder", MAX_LINE, "<green>Enter new reminder:<yellow> ");
}

void config_options(User *usr) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			Print(usr, "<magenta>\n"
				"Beep on e<hotkey>Xpress message arrival       <white>%s<magenta>\n"
				"<hotkey>Message reception is ...              <white>%s<magenta>\n"
				"<hotkey>Accept Xes from Friends regardless    <white>%s<magenta>\n",

				(usr->display.flags & DISPLAY_BEEP) ? "Yes" : "No",
				(usr->flags & USR_X_DISABLED) ? "Disabled" : "Enabled",
				(usr->flags & USR_BLOCK_FRIENDS) ? "No" : "Yes"
			);
			Print(usr,
				"Display <hotkey>time as                       <white>%s<magenta>\n"
				"Ask for a <hotkey>reason when locking screen  <white>%s<magenta>\n",

				(usr->flags & USR_12HRCLOCK) ? "12 hour clock (AM/PM)" : "24 hour clock",
				(usr->flags & USR_NO_AWAY_REASON) ? "No" : "Yes"
			);
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Options%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				Put(usr, "Config menu\n");
				save_User(usr);
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'x':
			case 'X':
				Put(usr, "Beep\n");
				usr->display.flags ^= DISPLAY_BEEP;
				user_dirty(usr, "display_flags");
				break;

			case 'm':
			case 'M':
				usr->flags ^= USR_X_DISABLED;
				Print(usr, "%s message reception\n", (usr->flags & USR_X_DISABLED) ? "Disable" : "Enable");
				user_dirty(usr, "flags");
				break;

			case 'a':
			case 'A':
				usr->flags ^= USR_BLOCK_FRIENDS;
				Print(usr, "%s friend messages\n", (usr->flags & USR_BLOCK_FRIENDS) ? "Block" : "Accept");
				user_dirty(usr, "flags");
				break;

			case 't':
			case 'T':
				Put(usr, "Display time\n");
				usr->flags ^= USR_12HRCLOCK;
				user_dirty(usr, "flags");
				break;

			case 'r':
			case 'R':
				usr->flags ^= USR_NO_AWAY_REASON;
				Print(usr, "%s for a reason\n", (usr->flags & USR_NO_AWAY_REASON) ? "Don't ask" : "Ask");
				user_dirty(usr, "flags");
				break;

			default:
				print_menu = 0;
		}
	}
}

void config_terminal(User *usr) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			Print(usr, "<magenta>\n"
				" <hotkey>Terminal emulation                   <white>%s<magenta>\n"
				" Make use of bold/bright <hotkey>attribute    <white>%s<magenta>\n",

				(usr->display.flags & DISPLAY_ANSI) ? "ANSI" : "dumb",
				(usr->display.flags & DISPLAY_BOLD) ? "Yes" : "No"
			);
			Print(usr,
				" Show hotkeys in <hotkey>bold/bright          <white>%s<magenta>\n",

				(usr->display.flags & DISPLAY_BOLD_HOTKEYS) ? "Yes" : "No"
			);
			Print(usr,
				" Always show hotkeys in <hotkey>uppercase     <white>%s<magenta>\n"
				" Show angle brac<hotkey>kets around hotkeys   <white>%s<magenta>\n",

				(usr->display.flags & DISPLAY_UPPERCASE_HOTKEYS) ? "Yes" : "No",
				(usr->display.flags & DISPLAY_HOTKEY_BRACKETS) ? "Yes" : "No"
			);
			Print(usr, "\n"
				" <hotkey>Force screen width and height        <white>%s<magenta>\n"
				" Screen <hotkey>dimensions                    <white>%dx%d<magenta>\n",

				(usr->display.flags & DISPLAY_FORCE_TERM) ? "Yes" : "No",
				usr->display.term_width, usr->display.term_height
			);

			if (usr->display.flags & DISPLAY_ANSI) {
				Print(usr, "\n"
					" Color <hotkey>scheme                         <white>%s<magenta>\n",

					(usr->display.flags & DISPLAY_COLOR_SYMBOLS) ? "Modern" : "Classic"
				);
			}
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Terminal%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				Put(usr, "Config menu\n");
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 't':
			case 'T':
				usr->display.flags ^= DISPLAY_ANSI;

				if (usr->display.flags & DISPLAY_ANSI) {		/* assume bold/non-bold */
					usr->display.flags |= (DISPLAY_BOLD|DISPLAY_BOLD_HOTKEYS);
					usr->display.flags &= ~DISPLAY_HOTKEY_BRACKETS;
				} else {
					usr->display.flags &= ~(DISPLAY_BOLD|DISPLAY_BOLD_HOTKEYS);
					usr->display.flags |= DISPLAY_HOTKEY_BRACKETS;
					reset_colors(usr);
				}
				Put(usr, "Terminal emulation\n");
				user_dirty(usr, "display_flags");
				break;

			case 'a':
			case 'A':
				reset_colors(usr);

				usr->display.flags ^= DISPLAY_BOLD;

				Put(usr, "<white>Attribute bold/bright\n");
				user_dirty(usr, "display_flags");
				break;

			case 'b':
			case 'B':
				Put(usr, "Bold/bright hotkeys\n");
				usr->display.flags ^= DISPLAY_BOLD_HOTKEYS;
				user_dirty(usr, "display_flags");
				break;

			case 'u':
			case 'U':
				Put(usr, "Uppercase hotkeys\n");
				usr->display.flags ^= DISPLAY_UPPERCASE_HOTKEYS;
				user_dirty(usr, "display_flags");
				break;

			case 'k':
			case 'K':
				Put(usr, "Angle brackets around hotkeys\n");
				usr->display.flags ^= DISPLAY_HOTKEY_BRACKETS;
				user_dirty(usr, "display_flags");
				break;

			case 's':
			case 'S':
				if (!(usr->display.flags & DISPLAY_ANSI)) {
					print_menu = 0;
					break;
				}
				Put(usr, "Color scheme\n");
				usr->display.flags ^= DISPLAY_COLOR_SYMBOLS;
				user_dirty(usr, "display_flags");
				break;

			case 'f':
			case 'F':
				usr->display.flags ^= DISPLAY_FORCE_TERM;
				Print(usr, "%s screen width and height\n", (usr->display.flags & DISPLAY_FORCE_TERM) ? "Force" : "Don't force");

				if (!(usr->display.flags & DISPLAY_FORCE_TERM)) {
					usr->display.term_width = usr->telnet.term_width;
					usr->display.term_height = usr->telnet.term_height;
				}
				user_dirty(usr, "display_flags");
				break;

			default:
				print_menu = 0;
		}
	}
}

void config_who(User *usr) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			Print(usr, "\n<magenta>"
				" Default who list <hotkey>format      <white>%s<magenta>\n"
				" Sort <hotkey>by...                   <white>%s<magenta>\n"
				" Sort <hotkey>order                   <white>%s<magenta>\n",
				(usr->flags & USR_SHORT_WHO)		? "Short" : "Long",
				(usr->flags & USR_SORT_BYNAME)		? "Name" : "Online time",
				(usr->flags & USR_SORT_DESCENDING)	? "Descending" : "Ascending"
			);
			Print(usr,
				" Show online <hotkey>enemies          <white>%s<magenta>\n",
				(usr->flags & USR_HIDE_ENEMIES)		? "No" : "Yes"
			);
		}
		print_menu = 1;

		Print(usr, "<yellow>\n[Config] Who%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				Put(usr, "\n");
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'f':
			case 'F':
				Put(usr, "Format\n");
				usr->flags ^= USR_SHORT_WHO;
				user_dirty(usr, "flags");
				break;

			case 'b':
			case 'B':
				Put(usr, "Sort by\n");
				usr->flags ^= USR_SORT_BYNAME;
				user_dirty(usr, "flags");
				break;

			case 'o':
			case 'O':
				Put(usr, "Sort order\n");
				usr->flags ^= USR_SORT_DESCENDING;
				user_dirty(usr, "flags");
				break;

			case 'e':
			case 'E':
				usr->flags ^= USR_HIDE_ENEMIES;
				Print(usr, "%s enemies\n", (usr->flags & USR_HIDE_ENEMIES) ? "Don't show" : "Show");
				user_dirty(usr, "flags");
				break;

			default:
				print_menu = 0;
		}
	}
}

void config_password(User *usr) {
char pw[MAX_LINE], again[MAX_LINE], crypted[MAX_CRYPTED];
int attempts, shorty;

	Put(usr, "<green>Enter old password:<yellow> ");

	if (edit_password(usr, pw, MAX_LINE) == -1) {
		Put(usr, "<red>Password not changed\n");
		return;
	}
	if (check_password(usr->name, pw)) {
		Put(usr, "<red>Wrong password\n");
		return;
	}
	attempts = shorty = 0;
	for(;;) {
		Put(usr, "<green>Enter new password: ");

		if (edit_password(usr, pw, MAX_LINE) == -1) {
			Put(usr, "<red>Password not changed\n");
			return;
		}
		if (strlen(pw) < PASSWD_MIN_LEN) {
			shorty++;
			Print(usr, "<red>That password is %stoo short\n\n", (shorty <= 1) ? "" : "also ");
			continue;
		}
		Put(usr, "<green>Enter it again (for verification):<yellow> ");

		if (edit_password(usr, again, MAX_LINE) == -1) {
			Put(usr, "<red>Password NOT changed\n");
			return;
		}
		if (strcmp(pw, again)) {
			Put(usr, "<red>Passwords did not match!\n");

			attempts++;
			if (attempts >= 3) {
				Put(usr, "Maybe you want to try again later ...\n");
				return;
			}
			Put(usr, "\n");
		} else {
			memset(again, 0, MAX_LINE);
			break;
		}
	}
	if (crypt_password(pw, crypted, MAX_CRYPTED) == -1) {
		Put(usr, "<red>Error encrypting this password, password not changed\n");
		return;
	}
	memset(pw, 0, MAX_LINE);

	if (save_password(usr->name, crypted) == -1) {
		Put(usr, "<red>Error encountered, password not changed\n");
		return;
	}
	memset(crypted, 0, MAX_CRYPTED);
	Put(usr, "<green>Password changed\n");
}


/*
	set a configurable string
*/
void change_config_string(User *usr, char **var, char *field_name, int bufsiz, char *prompt) {
char buf[MAX_LONGLINE], *s;
int r, max;

	Put(usr, prompt);

	max = (bufsiz < sizeof(buf)) ? bufsiz : sizeof(buf);

	r = edit_line(usr, buf, max);

	if (r == -1)
		return;

	if (!buf[0]) {
		if (var != NULL && *var != NULL && **var)
			Put(usr, "<red>Not changed\n");

		return;
	}
	cstrip_line(buf);

	if (!buf[0]) {
		Free(*var);
		*var = NULL;
		user_dirty(usr, field_name);
		return;
	}
	if ((s = cstrdup(buf)) == NULL) {
		Put(usr, "<red>Out of memory error\n");
		return;
	}
	Free(*var);
	*var = s;
	user_dirty(usr, field_name);
}

/* EOB */
