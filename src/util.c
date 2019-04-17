/*
	util.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "util.h"
#include "keys.h"
#include "cstring.h"
#include "log.h"
#include "bufprintf.h"
#include "colorize.h"
#include "Memory.h"
#include "pager.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>


char *Days[7] = { "Sun", "Mon", "Tues", "Wednes", "Thurs", "Fri", "Satur" };


int spawn(void *(*start_func)(void *), void *arg) {
int err;
pthread_t t;

	if ((err = pthread_create(&t, NULL, start_func, arg)) != 0) {
		log_err("spawn(): error creating thread");
		return -1;
	}
	return 0;
}

int yesno(User *usr, char *prompt, int default_answer) {
int c;

	for(;;) {
		Put(usr, prompt);

		c = Getch(usr);

		if (c == KEY_RETURN)
			c = default_answer;

		switch(c) {
			case 'y':
			case 'Y':
				Put(usr, "Yes\n");
				return YESNO_YES;

			case 'n':
			case 'N':
				Put(usr, "No\n");
				return YESNO_NO;

			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				Put(usr, "\n");
				return YESNO_UNDEF;

			default:
				Put(usr, "\n");
		}
	}
	return YESNO_UNDEF;
}

void reset_colors(User *usr) {
	write_IOBuf(&(usr->iobuf), "\x1b[0m", 4);
}

void beep(User *usr) {
	if (usr->display.flags & DISPLAY_BEEP)
		putc_IOBuf(&(usr->iobuf), '\a');
}

void erase(User *usr, int pos) {
	if (pos <= 0)
		return;

	if (usr->display.flags & DISPLAY_ANSI) {
		char buf[MAX_LINE];
		int len;

		if (pos <= 0) {
			cstrcpy(buf, "\x1b[K", MAX_LINE);
			len = 3;
		} else
			len = bufprintf(buf, MAX_LINE, "\x1b[%dD\x1b[K", pos);	/* move cursor left and erase line */

		write_IOBuf(&(usr->iobuf), buf, len);
	} else {
		int i;

		for(i = 0; i < pos; i++)
			write_IOBuf(&(usr->iobuf), "\b \b", 3);
	}
	usr->display.cursor -= pos;
	if (usr->display.cursor < 0)
		usr->display.cursor = 0;
}

void erase_line(User *usr) {
	if (usr->display.flags & DISPLAY_ANSI)
		write_IOBuf(&(usr->iobuf), "\r\x1b[K", 4);
	else {
		int i;

		putc_IOBuf(&(usr->iobuf), '\r');

		for(i = 0; i < usr->display.term_width-1; i++)
			putc_IOBuf(&(usr->iobuf), ' ');

		putc_IOBuf(&(usr->iobuf), '\r');
	}
	usr->display.cursor = 0;
}

void clear_screen(User *usr) {
	if (usr->display.flags & DISPLAY_ANSI)
		write_IOBuf(&(usr->iobuf), "\x1b[1;1H\x1b[2J", 10);
	else {
		int i;

		putc_IOBuf(&(usr->iobuf), '\r');

		for(i = 0; i < usr->display.term_height; i++)
			putc_IOBuf(&(usr->iobuf), '\n');
	}
	usr->display.cursor = 0;
}

void save_cursor(User *usr) {
	write_IOBuf(&(usr->iobuf), "\x1b[s", 3);
}

void restore_cursor(User *usr) {
	if (usr->display.flags & DISPLAY_ANSI)
		write_IOBuf(&(usr->iobuf), "\x1b[u", 3);
}

void home_cursor(User *usr) {
	if (usr->display.flags & DISPLAY_ANSI)
		write_IOBuf(&(usr->iobuf), "\x1b[1;1H", 6);
	else
		putc_IOBuf(&(usr->iobuf), '\r');

	usr->display.cursor = 0;
}

void scroll_up(User *usr, int n) {
	if (!(usr->display.flags & DISPLAY_ANSI))
		return;

	if (n == 1)
		write_IOBuf(&(usr->iobuf), "\x1b[S", 3);
	else {
		char buf[8];
		int len;

		len = bufprintf(buf, sizeof(buf), "\x1b[%dS", n);
		write_IOBuf(&(usr->iobuf), buf, len);
	}
}

void scroll_down(User *usr, int n) {
	if (!(usr->display.flags & DISPLAY_ANSI)) {
		putc_IOBuf(&(usr->iobuf), '\r');

		while(n > 0) {
			putc_IOBuf(&(usr->iobuf), '\n');
			n--;
		}
		return;
	}
	if (n == 1)
		write_IOBuf(&(usr->iobuf), "\x1b[T", 3);
	else {
		char buf[8];
		int len;

		len = bufprintf(buf, sizeof(buf), "\x1b[%dT", n);
		write_IOBuf(&(usr->iobuf), buf, len);
	}
}

/*
	write bitflags out as a text string
*/
void flags_to_str(int flags, GString *gstr, char **flagnames, int numflags) {
int i, bit;

	bit = 1;

	for(i = 0; i < numflags; i++) {
		if (flags & bit) {
			if (gstr->len > 0)
				gstrcat(gstr, "|");

			gstrcat(gstr, flagnames[i]);
		}
		bit <<= 1;
	}
}

int str_to_flags(char *str, char **flagnames, int numflags) {
int i, bit, flags;

	flags = 0;
	bit = 1;

	for(i = 0; i < numflags; i++) {
		if (cstrfind(str, flagnames[i], '|'))
			flags |= bit;

		bit <<= 1;
	}
	return flags;
}

/*
	pretty print the date
*/
char *sprint_date(struct tm *t, int twelve_hour_clock, char *date_str, int buflen) {
char add[2];

	if (t->tm_mday >= 10 && t->tm_mday <= 20) {
		add[0] = 't';
		add[1] = 'h';
	} else {
		switch(t->tm_mday % 10) {
			case 1:
				add[0] = 's';
				add[1] = 't';
				break;

			case 2:
				add[0] = 'n';
				add[1] = 'd';
				break;

			case 3:
				add[0] = 'r';
				add[1] = 'd';
				break;

			default:
				add[0] = 't';
				add[1] = 'h';
		}
	}
	if (twelve_hour_clock) {
		char am_pm = 'A';

		if (t->tm_hour >= 12) {
			am_pm = 'P';
			if (t->tm_hour > 12)
				t->tm_hour -= 12;
		}
		bufprintf(date_str, buflen, "%sday, %s %d%c%c %d %02d:%02d:%02d %cM",
			Days[t->tm_wday], Months[t->tm_mon], t->tm_mday, add[0], add[1], t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec, am_pm);
	} else
		bufprintf(date_str, buflen, "%sday, %s %d%c%c %d %02d:%02d:%02d",
			Days[t->tm_wday], Months[t->tm_mon], t->tm_mday, add[0], add[1], t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec);

	return date_str;
}

/*
	print time, maybe with AM/PM
*/
char *sprint_time(struct tm *t, int twelve_hour_clock, char *buf, int max) {
	if (twelve_hour_clock) {
		char am_pm = 'A';

		if (t->tm_hour >= 12) {
			am_pm = 'P';
			if (t->tm_hour > 12)
				t->tm_hour -= 12;
		}
		bufprintf(buf, max, "%02d:%02d %cM", t->tm_hour, t->tm_min, am_pm);
	} else
		bufprintf(buf, max, "%02d:%02d", t->tm_hour, t->tm_min);

	return buf;
}

/*
	I wrote the former function out and turned into this math-like looking
	formula stuff. This one actually produces better output.

	Note: buf must be large enough, MAX_LINE should do
*/
char *sprint_total_time(unsigned long total, char *buf, int buflen) {
int div[5] = { SECS_IN_WEEK, SECS_IN_DAY, SECS_IN_HOUR, SECS_IN_MIN, 1 };
int v[5];
int i, l, elems;
char *one[5] = { "week", "day", "hour", "minute", "second" };
char *more[5] = { "weeks", "days", "hours", "minutes", "seconds" };

	if (buf == NULL || buflen <= 0)
		return NULL;

	elems = 0;
	for(i = 0; i < 5; i++) {
		v[i] = total / div[i];
		total %= div[i];
		if (v[i] > 0)
			elems++;
	}
	if (!elems) {
		cstrcpy(buf, "0 seconds", buflen);
		return buf;
	}
	buf[0] = 0;
	l = 0;
	for(i = 0; i < 5; i++) {
		if (v[i] > 0) {
			l += bufprintf(buf+l, buflen - l, "%d %s", v[i], (v[i] == 1) ? one[i] : more[i]);
			elems--;
			if (!elems)
				break;

			l += bufprintf(buf+l, buflen - l, (elems == 1) ? " and " : ", ");
		}
	}
	return buf;
}

/*
	Print a large number with dots or comma's
	We use a silly trick to do this ; walk the string in a reverse order and
	insert comma's into the string representation

	Note: buf must be large enough, MAX_NUMBER will do
*/
char *sprint_number(unsigned long ul, int sep, char *buf, int buflen) {
char buf2[MAX_NUMBER];
int i, j = 0, n = 0;

	if (buf == NULL || buflen <= 0)
		return NULL;

	buf[0] = 0;
	bufprintf(buf2, sizeof(buf2), "%lu", ul);
	i = strlen(buf2)-1;
	if (i < 0)
		return buf;

	while(i >= 0 && j < buflen-1) {
		buf[j++] = buf2[i--];
		if (j >= buflen-1)
			break;

		n++;
		if (i >= 0 && n >= 3) {
			n = 0;
			buf[j++] = sep;			/* usually dot or comma */
		}
	}
	buf[j] = 0;

	cstrcpy(buf2, buf, MAX_NUMBER);
	i = strlen(buf2)-1;
	j = 0;
	while(i >= 0 && j <= buflen-1)
		buf[j++] = buf2[i--];
	buf[j] = 0;
	return buf;
}

/*
	display large numbers with comma's
	Note: buf must be large enough, MAX_NUMBER should do
*/
char *sprint_number_commas(unsigned long ul, char *buf, int buflen) {
	return sprint_number(ul, ',', buf, buflen);
}

/*
	display large numbers with dots
	Note: buf must be large enough, MAX_NUMBER should do
*/
char *sprint_number_dots(unsigned long ul, char *buf, int buflen) {
	return sprint_number(ul, '.', buf, buflen);
}

/*
	print number "1st", "2nd", "3rd"
	only the last two chars into buf
*/
char *sprint_numberth(int num, char *add) {
	add[2] = 0;

	if (((num % 100) >= 10) && ((num % 100) <= 20)) {
		add[0] = 't';
		add[1] = 'h';
	} else {
		switch(num % 10UL) {
			case 1:
				add[0] = 's';
				add[1] = 't';
				break;

			case 2:
				add[0] = 'n';
				add[1] = 'd';
				break;

			case 3:
				add[0] = 'r';
				add[1] = 'd';
				break;

			default:
				add[0] = 't';
				add[1] = 'h';
		}
	}
	return add;
}

void hline(User *usr, char c) {
char buf[PRINT_BUF];
int n, len;

	n = usr->display.term_width+1;
	if (n > PRINT_BUF-1)
		n = PRINT_BUF-1;

	len = n;
	buf[n--] = 0;

	if (n >= 0)
		buf[n--] = '\n';
	if (n >= 0)
		buf[n--] = '\r';

	while(n >= 0)
		buf[n--] = c;

	write_IOBuf(&(usr->iobuf), buf, len);
}

void center(User *usr, char *msg) {
int n;

	n = ((usr->display.term_width-1) >> 1) - (color_strlen(&(usr->display), msg) >> 1) - usr->display.cursor;
	while(n > 0) {
		putc_IOBuf(&(usr->iobuf), ' ');
		n--;
	}
	Put(usr, msg);
	Put(usr, "\n");
}

void download_text(User *usr, char *text) {
int c, i;
char **lines, *copy;

	Put(usr, "<cyan>Start screen capture and press<white> <return> <cyan>to start download<green>\n");

	c = 0;
	while(c != KEY_RETURN) {
		c = Getch(usr);

		switch(c) {
			case KEY_ESC:
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
				Put(usr, "<red>Download cancelled\n");
				return;

			case KEY_RETURN:
				break;

			default:
				c = 0;
		}
	}
/*
	now, we could just dump the text to the user's terminal, but it would look like crap
	due to the newlines. So we split the text up and write "\r\n" 'by hand'
*/
	if ((copy = cstrdup(text)) == NULL) {
		Put(usr, "<red>Due to an error, we're unable to download the text\n");
		return;
	}
	if ((lines = cstrsplit(copy, '\n', NULL)) == NULL) {
		Put(usr, "<red>Due to an error, we're unable to download the text\n");
		cstrfree(copy);
		return;
	}
/*
	raw write the text with newline conversion
*/
	for(i = 0; lines[i] != NULL; i++) {
		write_IOBuf(&(usr->iobuf), lines[i], strlen(lines[i]));
		write_IOBuf(&(usr->iobuf), "\r\n", 2);
	}
	Free(lines);
	cstrfree(copy);

	Put(usr, "<white>\n-- end of text --\n"
		"<cyan>Stop screen capture and press any key to continue\n");
	Getch(usr);
}

void display_file(User *usr, char *filename) {
FILE *f;
char line[MAX_LONGLINE];

	if ((f = fopen(filename, "r")) == NULL) {
		Print(usr, "<red>Sorry, but there was an error when opening the file '%s'\n", filename);
		return;
	}
	init_pager(usr);

	while(fgets(line, sizeof(line), f) != NULL) {
		ctrim_line(line);

		Print(usr, "%s\n", line);
	}
	fclose(f);

	pager(usr);
}

/* EOB */
