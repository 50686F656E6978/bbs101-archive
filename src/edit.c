/*
	edit.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "debug.h"
#include "edit.h"
#include "keys.h"
#include "cstring.h"
#include "Memory.h"
#include "util.h"
#include "online.h"
#include "access.h"
#include "colorize.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


char *Wrap_Charset1 = " ,-+=&^%!?>}])/\\";
char *Wrap_Charset2 = "<{[($`#~|";


/*
	main edit function, this is a line editor supporting cursor keys
*/
void line_editor(User *usr, char *edit_buf, int max, int c, int *cursor, int *end, char echo_char) {
int i;

	switch(c) {
		case KEY_ESC:
		case KEY_CTRL('Y'):			/* delete line */
		case KEY_CTRL('X'):
			Write(usr, "\x1b[%dD\x1b[K", *cursor);
			*cursor = 0;
			*end = 0;
			edit_buf[0] = 0;
			break;

		case KEY_LEFT:
			if (*cursor > 0) {
				(*cursor)--;
				Write(usr, "\x1b[D");
			}
			break;

		case KEY_RIGHT:
			if (*cursor < *end) {
				(*cursor)++;
				Write(usr, "\x1b[C");
			}
			break;

		case KEY_HOME:
			Write(usr, "\x1b[%dD", *cursor);
			*cursor = 0;
			break;

		case KEY_END:
			if (*cursor < *end) {
				Write(usr, "\x1b[%dC", *end - *cursor);
				*cursor = *end;
			}
			break;

		case KEY_BS:
			if (*cursor > 0) {
				(*cursor)--;
				memmove(&edit_buf[*cursor], &edit_buf[*cursor + 1], strlen(edit_buf + *cursor));
				(*end)--;

				if (!echo_char)
					Write(usr, "\b%s \x1b[%dD", edit_buf + *cursor, strlen(edit_buf + *cursor)+1);
				else {
					Putc(usr, '\b');
					for(i = 0; i < strlen(edit_buf + *cursor); i++)
						Putc(usr, echo_char);

					Write(usr, " \x1b[%dD", strlen(edit_buf + *cursor)+1);
				}
			}
			break;

		case KEY_DELETE:
			if (*cursor < *end) {
				memmove(&edit_buf[*cursor], &edit_buf[*cursor + 1], strlen(edit_buf + *cursor));
				(*end)--;

				if (!echo_char)
					Write(usr, "%s \x1b[%dD", edit_buf + *cursor, strlen(edit_buf + *cursor)+1);
				else {
					for(i = 0; i < strlen(edit_buf + *cursor); i++)
						Putc(usr, echo_char);

					Write(usr, " \x1b[%dD", strlen(edit_buf + *cursor)+1);
				}
			}
			break;

		default:
			if (c < ' ' || c > '~')
				break;

			if (*end >= max-1) {
				edit_buf[--(*end)] = 0;
				if (*cursor > *end) {
					*cursor = *end;
					Putc(usr, '\b');
				}
			}
			memmove(&edit_buf[*cursor + 1], &edit_buf[*cursor], strlen(edit_buf + *cursor)+1);
			edit_buf[*cursor] = c;

			if (!echo_char)
				Write(usr, "%s", edit_buf + *cursor);
			else {
				for(i = 0; i < strlen(edit_buf + *cursor); i++)
					Putc(usr, echo_char);
			}
			if (strlen(edit_buf + *cursor) > 1)
				Write(usr, "\x1b[%dD", strlen(edit_buf + *cursor)-1);

			(*cursor)++;
			(*end)++;
	}
}

/*
	edit multiple lines into a GString buffer
	used for editing messages
*/
void text_editor(User *usr, GString *gstr, char *prompt, char *edit_buf, int max, int c, int *cursor, int *end, int *line, int max_lines) {
	line_editor(usr, edit_buf, max, c, cursor, end, 0);

	if (*line < max_lines-1 && *cursor >= max-1) {
		char wrap[MAX_HALFLINE];
		int wrap_len, i;

/* do word-wrapping */
		if (c != ' ' && cstrchr(Wrap_Charset2, c) == NULL) {
			wrap_len = 25;

			for(i = *cursor - 1; i > *cursor - wrap_len && i > 0; i--) {
				if (cstrchr(Wrap_Charset1, edit_buf[i]) != NULL) {
					i++;
					break;
				}
				if (cstrchr(Wrap_Charset2, edit_buf[i]) != NULL)
					break;
			}
			if (i > *cursor - wrap_len && i > 0) {
				cstrcpy(wrap, edit_buf+i, MAX_LINE);
				edit_buf[i] = 0;
				erase(usr, *cursor - i);
			}
		}
		Put(usr, "\n");

		gstrcat(gstr, edit_buf);
		gputc(gstr, '\n');
		(*line)++;

		cstrcpy(edit_buf, wrap, MAX_LINE);
		*cursor = *end = strlen(edit_buf);

		if (prompt != NULL)
			Put(usr, prompt);

		Put(usr, edit_buf);
	}
}

int edit_line(User *usr, char *edit_buf, int max) {
int c, cursor, end;

	cursor = end = 0;
	edit_buf[0] = 0;

	for(;;) {
		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				Put(usr, "\n");
				return -1;

			case KEY_RETURN:
				Put(usr, "\n");
				return 0;

			default:
				line_editor(usr, edit_buf, max, c, &cursor, &end, 0);
		}
	}
	return 0;
}

int edit_lines(User *usr, GString *gstr, char *prompt, int max_lines) {
int c, cursor, end, max, line;
char edit_buf[MAX_LINE];

	reset_GString(gstr);

	if (prompt != NULL)
		max = MAX_LINE - color_strlen(&(usr->display), prompt);
	else
		max = MAX_LINE;

	if (usr->display.term_width < max)
		max = usr->display.term_width;

	cursor = end = line = 0;
	edit_buf[0] = 0;

	if (prompt != NULL)
		Put(usr, prompt);

	for(;;) {
		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				Put(usr, "\n");
				reset_GString(gstr);
				return -1;

			case KEY_RETURN:
				Put(usr, "\n");

				if (!edit_buf[0])
					return 0;

				gstrcat(gstr, edit_buf);
				gputc(gstr, '\n');

				line++;
				if (line >= max_lines)
					return 0;

				edit_buf[0] = 0;
				cursor = end = 0;

				if (prompt != NULL)
					Put(usr, prompt);
				break;

			default:
				text_editor(usr, gstr, prompt, edit_buf, max, c, &cursor, &end, &line, max_lines);
		}
	}
	return 0;
}

/*
	the name editor keeps names Capitalized
*/
void name_editor(User *usr, char *edit_buf, int c, int *cursor, int *end) {
	switch(c) {
		case ' ':
			if (!*cursor)
				break;

			if (edit_buf[*cursor] == ' ' || edit_buf[*cursor - 1] == ' ')
				break;

			if (*end >= MAX_NAME-1) {
				edit_buf[--(*end)] = 0;
				if (*cursor > *end) {
					*cursor = *end;
					Putc(usr, '\b');
				}
			}
			memmove(&edit_buf[*cursor + 1], &edit_buf[*cursor], strlen(edit_buf + *cursor)+1);
			edit_buf[*cursor] = ' ';

			if (edit_buf[*cursor + 1] >= 'a' && edit_buf[*cursor + 1] <= 'z')
				edit_buf[*cursor + 1] -= ' ';

			Write(usr, "%s", edit_buf + *cursor);
			if (strlen(edit_buf + *cursor) > 1)
				Write(usr, "\x1b[%dD", strlen(edit_buf + *cursor)-1);

			(*cursor)++;
			(*end)++;
			break;

		case KEY_BS:
			if (*cursor > 0) {
				(*cursor)--;
				memmove(&edit_buf[*cursor], &edit_buf[*cursor + 1], strlen(edit_buf + *cursor));
				(*end)--;

				if (!*cursor) {
					if (edit_buf[*cursor] >= 'a' && edit_buf[*cursor] <= 'z')
						edit_buf[*cursor] -= ' ';
				} else {
					if (edit_buf[*cursor - 1] == ' ' && edit_buf[*cursor] >= 'a' && edit_buf[*cursor] <= 'z')
						edit_buf[*cursor] -= ' ';
				}
				Write(usr, "\b%s \x1b[%dD", edit_buf + *cursor, strlen(edit_buf + *cursor)+1);
			}
			break;

		case KEY_DELETE:
			if (*cursor < *end) {
				memmove(&edit_buf[*cursor], &edit_buf[*cursor + 1], strlen(edit_buf + *cursor));
				(*end)--;

				if (!*cursor) {
					if (edit_buf[*cursor] >= 'a' && edit_buf[*cursor] <= 'z')
						edit_buf[*cursor] -= ' ';
				} else {
					if (edit_buf[*cursor - 1] == ' ' && edit_buf[*cursor] >= 'a' && edit_buf[*cursor] <= 'z')
						edit_buf[*cursor] -= ' ';
				}
				Write(usr, "%s \x1b[%dD", edit_buf + *cursor, strlen(edit_buf + *cursor)+1);
			}
			break;

		default:
			if (c >= ' ' && c <= '~') {
				if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
					break;

				if ((!*cursor || edit_buf[*cursor - 1] == ' ')
					&& c >= 'a' && c <= 'z')
					c -= ' ';						/* auto uppercase */
			}
			line_editor(usr, edit_buf, MAX_NAME, c, cursor, end, 0);
	}
}

int edit_name(User *usr, char *edit_buf) {
int c, cursor, end;

	cursor = end = 0;
	edit_buf[0] = 0;

	for(;;) {
		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				erase(usr, cursor);
				Put(usr, "\n");
				return -1;

			case KEY_RETURN:
				if (end > 0 && edit_buf[end-1] == ' ')
					edit_buf[--end] = 0;

				Put(usr, "\n");
				return 0;

			default:
				name_editor(usr, edit_buf, c, &cursor, &end);
		}
	}
	return 0;
}

/*
	edit a single name, and enable tab expansion
*/
int edit_tabname(User *usr, char *edit_buf) {
int c, cursor, end;
char *tabp;
GString tablist;

	cursor = end = 0;
	edit_buf[0] = 0;

	tabp = NULL;
	init_GString(&tablist);

	for(;;) {
		c = Getch(usr);

		if (c != KEY_TAB && c != KEY_BACKTAB) {
			tabp = NULL;
			reset_GString(&tablist);
		}
		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				erase(usr, cursor);
				Put(usr, "\n");
				deinit_GString(&tablist);
				return -1;

			case KEY_RETURN:
				if (end > 0 && edit_buf[end-1] == ' ')
					edit_buf[--end] = 0;

				Put(usr, "\n");
				deinit_GString(&tablist);
				return 0;

			case KEY_TAB:
				if (tablist.len <= 0) {
					get_online_names(&tablist);
					filter_tablist(usr, &tablist, edit_buf, cursor);
					tabp = NULL;
				}
				tab_list(usr, &tablist, &tabp, edit_buf, MAX_NAME, &cursor, &end);
				break;

			case KEY_BACKTAB:
				if (tablist.len <= 0) {
					get_online_names(&tablist);
					filter_tablist(usr, &tablist, edit_buf, cursor);
					tabp = NULL;
				}
				backtab_list(usr, &tablist, &tabp, edit_buf, MAX_NAME, &cursor, &end);
				break;

			default:
				name_editor(usr, edit_buf, c, &cursor, &end);
		}
	}
	return 0;
}

/*
	supports multiple recipients, tab expansion, tabbing through a list, and backtabbing
*/
int edit_recipients(User *usr, int (*access_func)(User *, char *, int)) {
int c, cursor, end, more, l;
char edit_buf[MAX_NAME], *tabp, *p, *endp;
GString tablist;

	cursor = end = 0;
	edit_buf[0] = 0;

	more = 0;

	if (usr->recipients.len <= 0)
		Put(usr, "<green>Enter recipients: <yellow>");
	else
		if (cstrchr(usr->recipients.str, ','))
			Put(usr, "<green>Enter recipients [<many>]: <yellow>");
		else
			Print(usr, "<green>Enter recipients [%s]: <yellow>", usr->recipients.str);

	tabp = NULL;
	init_GString(&tablist);

	for(;;) {
		c = Getch(usr);

		if (c != KEY_TAB && c != KEY_BACKTAB) {
			tabp = NULL;
			reset_GString(&tablist);
		}
		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				erase(usr, cursor);
				Put(usr, "\n");

				reset_GString(&(usr->recipients));
				deinit_GString(&tablist);
				return -1;

			case KEY_RETURN:
				if (!end && !edit_buf[0] && !more) {		/* choose default usr->recipients */
					Put(usr, "\n");
					deinit_GString(&tablist);
					return 0;
				}
				if (end > 0 && edit_buf[end-1] == ' ')
					edit_buf[--end] = 0;

				if (!more)
					reset_GString(&(usr->recipients));

				if (add_recipient(usr, edit_buf) == -1) {
					deinit_GString(&tablist);
					return -1;
				}
				if (usr->recipients.len > 0) {
					l = usr->recipients.len-1;
					if (usr->recipients.str[l] == ',') {
						usr->recipients.str[l] = 0;
						usr->recipients.len--;
					}
				}
				Put(usr, "\n");
				deinit_GString(&tablist);
				return 0;

			case KEY_BS:
				if (cursor > 0) {
					name_editor(usr, edit_buf, KEY_BS, &cursor, &end);
					break;
				}
				if (usr->recipients.len > 0) {
					char *p;

					if ((p = cstrrchr(usr->recipients.str, ',')) == NULL) {
						cstrcpy(edit_buf, usr->recipients.str, MAX_NAME);
						cursor = end = strlen(edit_buf);
						reset_GString(&(usr->recipients));

						erase(usr, cursor + 5);		/* "Enter recipients [name]: " */
						Put(usr, "<green>");
						Print(usr, ": <yellow>%s", edit_buf);
					} else {
						*p = 0;
						usr->recipients.len = strlen(usr->recipients.str);

						p++;
						cstrcpy(edit_buf, p, MAX_NAME);
						cursor = end = strlen(edit_buf);

						if (cstrchr(usr->recipients.str, ',') == NULL) {
							erase(usr, 11);				/* " [<many>]: " */
							Print(usr, "<green> [%s]: <yellow>%s", usr->recipients.str, edit_buf);
						} else
							Print(usr, "%s", edit_buf);
					}
				}
				break;

			case '+':
			case ',':
				if (!end) {
					if (usr->recipients.len <= 0)
						break;

					erase(usr, 2);
					Put(usr, ", ");
					more++;
					break;
				}
				if (end > 0 && edit_buf[end-1] == ' ')
					edit_buf[--end] = 0;

				if (!access_func(usr, edit_buf, 0)) {
					erase(usr, cursor);
					Put(usr, "<yellow>");
					cursor = end = 0;
					edit_buf[0] = 0;
					break;
				}
				if (!more) {		/* new, different recipient */
					if (usr->recipients.len <= 0)
						erase(usr, cursor + 2);
					else {
						if (cstrchr(usr->recipients.str, ',') == NULL)
							erase(usr, cursor + usr->recipients.len + 5);
						else
							erase(usr, cursor + 11);
					}
					Print(usr, "<green> [%s], <yellow>", edit_buf);
					reset_GString(&(usr->recipients));
				} else {
					if (cstrfind(usr->recipients.str, edit_buf, ',')) {		/* already in recipient list */
						erase(usr, cursor);
						cursor = end = 0;
						edit_buf[0] = 0;
						break;
					}
					if (usr->recipients.len > 0) {
						if (cstrchr(usr->recipients.str, ',') == NULL) {
							erase(usr, cursor + usr->recipients.len + 4);
							Put(usr, "<green>[<many>], <yellow>");
						} else {
							erase(usr, cursor + 2);
							Put(usr, ", ");
						}
					} else {
						erase(usr, cursor + 2);
						Print(usr, "<green> [%s], <yellow>", edit_buf);
					}
				}
				if (add_recipient(usr, edit_buf) == -1) {
					deinit_GString(&tablist);
					return -1;
				}
				more++;
				cursor = end = 0;
				edit_buf[0] = 0;
				break;

			case KEY_ESC:
			case KEY_CTRL('R'):
			case KEY_CTRL('Y'):
			case KEY_CTRL('X'):
			case '-':
				if (usr->recipients.len <= 0)
					erase(usr, cursor);
				else {
					if (cstrchr(usr->recipients.str, ',') != NULL) {
						remove_recipient(usr, edit_buf);
						if (cstrchr(usr->recipients.str, ',') == NULL) {
							erase(usr, cursor + 9);
							Print(usr, "<green>%s]: <yellow>", usr->recipients.str);
						} else
							erase(usr, cursor);
					} else {
						int l;

						l = cursor + usr->recipients.len + 5;
						remove_recipient(usr, edit_buf);
						if (usr->recipients.len <= 0) {
							erase(usr, l);
							Put(usr, "<green>");
							Put(usr, ": <yellow>");
						} else
							erase(usr, cursor);
					}
				}
				cursor = end = 0;
				edit_buf[0] = 0;
				break;

			case KEY_CTRL('L'):
				if (end > 0)
					break;

				if (usr->recipients.len > 0
					&& cstrchr(usr->recipients.str, ',') != NULL) {
					Put(usr, usr->recipients.str);
					Put(usr, "\n<green>Enter recipients [<many>]: <yellow>");
				}
				break;

			case KEY_TAB:
				if (tablist.len <= 0) {
					get_online_names(&tablist);
					filter_tablist(usr, &tablist, edit_buf, cursor);
					tabp = NULL;
				}
				tab_list(usr, &tablist, &tabp, edit_buf, MAX_NAME, &cursor, &end);
				break;

			case KEY_BACKTAB:
				if (tablist.len <= 0) {
					get_online_names(&tablist);
					filter_tablist(usr, &tablist, edit_buf, cursor);
					tabp = NULL;
				}
				backtab_list(usr, &tablist, &tabp, edit_buf, MAX_NAME, &cursor, &end);
				break;

			case KEY_CTRL('F'):
				if (usr->friends.len <= 0)
					break;

				if (usr->recipients.len <= 0)
					erase(usr, cursor + 2);
				else
					if (cstrchr(usr->recipients.str, ',') == NULL)
						erase(usr, cursor + 5 + strlen(usr->recipients.str));	/* "Enter recipients [name]: " */
					else
						erase(usr, cursor + 11);			/* " [<many>]: " */

				if (!more)
					reset_GString(&(usr->recipients));

				p = endp = usr->friends.str;
				while((endp = cstrchr(endp, ',')) != NULL) {
					*endp = 0;
					if (is_online(p, NULL) && add_recipient(usr, p) == -1) {
						*endp = ',';
						deinit_GString(&tablist);
						return -1;
					}
					*endp = ',';
					endp++;
					p = endp;
				}
				if (access_func(usr, p, ACCESS_NO_PROMPT) && add_recipient(usr, p) == -1) {
					*endp = ',';
					deinit_GString(&tablist);
					return -1;
				}
				if (usr->recipients.len <= 0) {
					Put(usr, "<green>");
					Print(usr, ": <yellow>%s", edit_buf);
				} else {
					more++;

					if (cstrchr(usr->recipients.str, ',') == NULL)
						Print(usr, "<green> [%s], <yellow>%s", usr->recipients.str, edit_buf);
					else
						Print(usr, "<green> [<many>], <yellow>%s", edit_buf);
				}
				break;

			case KEY_CTRL('T'):
				if (usr->talked_to.len <= 0)
					break;

				if (usr->recipients.len <= 0)
					erase(usr, cursor + 2);
				else
					if (cstrchr(usr->recipients.str, ',') == NULL)
						erase(usr, cursor + 5 + strlen(usr->recipients.str));	/* "Enter recipients [name]: " */
					else
						erase(usr, cursor + 11);			/* " [<many>]: " */

				if (!more)
					reset_GString(&(usr->recipients));

				p = endp = usr->talked_to.str;
				while((endp = cstrchr(endp, ',')) != NULL) {
					*endp = 0;
					if (is_online(p, NULL) && add_recipient(usr, p) == -1) {
						*endp = ',';
						deinit_GString(&tablist);
						return -1;
					}
					*endp = ',';
					endp++;
					p = endp;
				}
				if (access_func(usr, p, ACCESS_NO_PROMPT) && add_recipient(usr, p) == -1) {
					*endp = ',';
					deinit_GString(&tablist);
					return -1;
				}
				if (usr->recipients.len <= 0) {
					Put(usr, "<green>");
					Print(usr, ": <yellow>%s", edit_buf);
				} else {
					more++;

					if (cstrchr(usr->recipients.str, ',') == NULL)
						Print(usr, "<green> [%s], <yellow>%s", usr->recipients.str, edit_buf);
					else
						Print(usr, "<green> [<many>], <yellow>%s", edit_buf);
				}
				break;

			case KEY_CTRL('W'):
				if (!(usr->runtime_flags & RTF_SYSOP))
					break;

				if (usr->recipients.len <= 0)
					erase(usr, cursor + 2);
				else
					if (cstrchr(usr->recipients.str, ',') == NULL)
						erase(usr, cursor + 5 + strlen(usr->recipients.str));	/* "Enter recipients [name]: " */
					else
						erase(usr, cursor + 11);			/* " [<many>]: " */

				get_online_names(&(usr->recipients));

				cstrremove(usr->recipients.str, usr->name, ',');
				usr->recipients.len = strlen(usr->recipients.str);

				if (usr->recipients.len <= 0) {
					Put(usr, "<green>");
					Print(usr, ": <yellow>%s", edit_buf);
				} else {
					more++;

					if (cstrchr(usr->recipients.str, ',') == NULL)
						Print(usr, "<green> [%s], <yellow>%s", usr->recipients.str, edit_buf);
					else
						Print(usr, "<green> [<many>], <yellow>%s", edit_buf);
				}
				break;

			default:
				name_editor(usr, edit_buf, c, &cursor, &end);
		}
	}
	deinit_GString(&tablist);
	return 0;
}

int add_recipient(User *usr, char *name) {
	if (usr->recipients.len <= 0)
		return gstrcpy(&(usr->recipients), name);

	if (cstrfind(usr->recipients.str, name, ','))
		return 0;

	gstrcat(&(usr->recipients), ",");
	gstrcat(&(usr->recipients), name);
	return 0;
}

void remove_recipient(User *usr, char *name) {
	if (usr->recipients.len <= 0)
		return;

	cstrremove(usr->recipients.str, name, ',');
	usr->recipients.len = strlen(usr->recipients.str);
	shrink_GString(&(usr->recipients));
}

/*
	remove all names from the tablist that do not match
	(this function is a lot like cstrremove())
*/
void filter_tablist(User *usr, GString *tablist, char *match, int matchlen) {
char *str, *p, *delim_p;

	str = tablist->str;

	if (str == NULL || !*str)
		return;

	p = str;
	while((delim_p = cstrchr(p, ',')) != NULL) {
		*delim_p = 0;

		if ((usr->enemies.len > 0 && (usr->flags & USR_HIDE_ENEMIES) && cstrfind(usr->enemies.str, p, ','))
			|| strncmp(p, match, matchlen)
			|| !strcmp(p, usr->name)) {
			*delim_p = ',';
			delim_p++;
			memmove(p, delim_p, strlen(delim_p)+1);
			continue;
		}
		*delim_p = ',';
		p = delim_p;
		p++;
	}
	if (strncmp(p, match, matchlen)) {
		int l;

		*p = 0;
		l = strlen(str)-1;
		if (l > 0 && str[l] == ',')
			str[l] = 0;
	}
	tablist->len = strlen(tablist->str);
}

void tab_list(User *usr, GString *tablist, char **tabp, char *edit_buf, int max, int *cursor, int *end) {
char *p;

	if (tablist->len <= 0)
		return;

	if (*tabp != NULL)
		*tabp = cstrchr(*tabp, ',');

	if (*tabp == NULL)
		*tabp = tablist->str;
	else {
		(*tabp)++;
		if (!**tabp)
			*tabp = tablist->str;
	}

/* now, tabp points to the next comma-separated entry in tablist */

	if ((p = cstrchr(*tabp, ',')) != NULL)
		*p = 0;

	cstrcpy(edit_buf, *tabp, max);

	if (p != NULL)
		*p = ',';

	erase(usr, *cursor);
	Write(usr, "%s", edit_buf);

	*cursor = *end = strlen(edit_buf);
}

void backtab_list(User *usr, GString *tablist, char **tabp, char *edit_buf, int max, int *cursor, int *end) {
char *p, *endp;

	if (tablist->len <= 0)
		return;

	endp = NULL;

	if (*tabp != NULL && *tabp != tablist->str) {
		endp = *tabp;
		endp--;
		assert(*endp == ',');

		*endp = 0;
	}
	if ((p = cstrrchr(tablist->str, ',')) == NULL)
		*tabp = tablist->str;
	else
		*tabp = p+1;

/* now, tabp points to the previous comma-separated entry in tablist */

	cstrcpy(edit_buf, *tabp, max);

	if (endp != NULL)
		*endp = ',';

	erase(usr, *cursor);
	Write(usr, "%s", edit_buf);

	*cursor = *end = strlen(edit_buf);
}

/*
	just like edit_line(), except that it echoes '*****' asterisks
*/
int edit_password(User *usr, char *edit_buf, int max) {
int c, cursor, end;

	cursor = end = 0;
	memset(edit_buf, 0, max);

	for(;;) {
		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				memset(edit_buf, 0, max);
				Put(usr, "\n");
				return -1;

			case KEY_RETURN:
				Put(usr, "\n");
				if (!end)
					return -1;

				return 0;

			default:
				line_editor(usr, edit_buf, max, c, &cursor, &end, '*');
		}
	}
	return 0;
}

int edit_number(User *usr, char *edit_buf) {
int c, cursor, end;

	cursor = end = 0;
	edit_buf[0] = 0;

	for(;;) {
		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				edit_buf[0] = 0;
				Put(usr, "\n");
				return -1;

			case KEY_RETURN:
				Put(usr, "\n");
				if (!end)
					return -1;

				return 0;

			default:
				if (c >= ' ' && c <= '~' && !(c >= '0' && c <= '9'))
					break;

				if (edit_buf[0] == '0' && c >= '0' && c <= '9')
					break;

				line_editor(usr, edit_buf, MAX_NUMBER, c, &cursor, &end, 0);
		}
	}
	return 0;
}

int abort_save_continue(User *usr) {
int c;

	for(;;) {
		Put(usr, "\n<hotkey>A<green>bort, <hotkey>Save, <hotkey>Continue: <white>");

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case 'a':
			case 'A':
				if (yesno(usr, "Abort -- <cyan>Are you sure? (y/N): <white>", 'N') == YESNO_YES)
					return -1;
				break;

			case 's':
			case 'S':
				Put(usr, "Save\n");
				return 0;

			case 'c':
			case 'C':
				Put(usr, "Continue\n<green>");
				return 1;

			default:
				;
		}
	}
	return 0;
}

int abort_save(User *usr) {
int c;

	for(;;) {
		Put(usr, "\n<hotkey>A<green>bort, <hotkey>Save: <white>");

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case 'a':
			case 'A':
				if (yesno(usr, "Abort -- <cyan>Are you sure? (y/N): <white>", 'N') == YESNO_YES)
					return -1;
				break;

			case 's':
			case 'S':
				Put(usr, "Save\n");
				return 0;

			default:
				;
		}
	}
	return 0;
}

int edit_text(User *usr, GString *gstr, char *prompt, int max_lines) {
int c, cursor, end, max, line;
char edit_buf[MAX_LINE];

	reset_GString(gstr);

	if (prompt != NULL)
		max = MAX_LINE - color_strlen(&(usr->display), prompt);
	else
		max = MAX_LINE;

	if (usr->display.term_width < max)
		max = usr->display.term_width;

	cursor = end = line = 0;
	edit_buf[0] = 0;

	if (prompt != NULL)
		Put(usr, prompt);

	for(;;) {
		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				switch(abort_save_continue(usr)) {
					case -1:				/* abort */
						return -1;

					case 0:					/* save */
						return 0;

					case 1:					/* continue */
						break;

					default:
						;
				}
				break;

			case KEY_RETURN:
				Put(usr, "\n");

				if (!edit_buf[0]) {
					switch(abort_save_continue(usr)) {
						case -1:			/* abort */
							return -1;

						case 0:				/* save */
							return 0;

						case 1:				/* continue */
							break;

						default:
							;
					}
				}
				gstrcat(gstr, edit_buf);
				gputc(gstr, '\n');

				line++;
				if (line >= max_lines)
					return abort_save(usr);

				edit_buf[0] = 0;
				cursor = end = 0;

				if (prompt != NULL)
					Put(usr, prompt);
				break;

			default:
				text_editor(usr, gstr, prompt, edit_buf, max, c, &cursor, &end, &line, max_lines);
		}
	}
	return 0;
}

int upload_text(User *usr, GString *gstr, char *prompt, int max_lines) {
int c, cursor, end, max, line;
char edit_buf[MAX_LINE];

	reset_GString(gstr);

	if (prompt != NULL)
		max = MAX_LINE - color_strlen(&(usr->display), prompt);
	else
		max = MAX_LINE;

	if (usr->display.term_width < max)
		max = usr->display.term_width;

	cursor = end = line = 0;
	edit_buf[0] = 0;

	if (prompt != NULL)
		Put(usr, prompt);

	for(;;) {
		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				switch(abort_save_continue(usr)) {
					case -1:				/* abort */
						return -1;

					case 0:					/* save */
						return 0;

					case 1:					/* continue */
						break;

					default:
						;
				}
				break;

			case KEY_RETURN:
				if (line >= max_lines) {
					Put(usr, "<red>Maximum number of lines exceeded, press<yellow> <Ctrl-C> <red>to end upload\n");
					break;
				}
				line++;

				Put(usr, "\n");

				if (edit_buf[0])
					gstrcat(gstr, edit_buf);
				gputc(gstr, '\n');

				edit_buf[0] = 0;
				cursor = end = 0;

				if (prompt != NULL)
					Put(usr, prompt);
				break;

			default:
				text_editor(usr, gstr, prompt, edit_buf, max, c, &cursor, &end, &line, max_lines);
		}
	}
	return 0;
}

/* EOB */
