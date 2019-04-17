/*
	colorize.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "colorize.h"
#include "bufprintf.h"
#include "cstring.h"
#include "defines.h"
#include "edit.h"
#include "colors.h"

#include "debug.h"

#include <stdio.h>
#include <stdlib.h>


/*
	default symbols for auto-coloring
	no '_' because it looks ugly
*/
static char *Default_Symbols = "!@$%^&*-+=()[]{}<>.,:;/'\"`\\|~#?";


void color_output(IOBuf *dev, Display *display, char *str) {
int c, n, pos, is_symbol, auto_color_symbols, wrap_after;

	auto_color_symbols = 1;
	wrap_after = 0;
	pos = 0;

	while(*str) {
		is_symbol = 0;
		pos++;
		c = *str;
/*
	do word wrapping
	charset1 wraps after the character, charset2 wraps before the character is printed
*/
		if (display->term_width - display->cursor <= WORDWRAP_WIDTH) {
			if (cstrchr(Wrap_Charset1, c) != NULL && display->cursor + word_len(display, str+1) >= display->term_width) {
				if (c == ' ') {
					write_IOBuf(dev, "\r\n", 2);
					display->cursor = 0;
					auto_color_symbols = 1;

					str++;
					continue;
				}
				wrap_after = 1;
			} else {
				if (c != '<' && cstrchr(Wrap_Charset2, c) != NULL
					&& display->cursor + word_len(display, str+1) >= display->term_width) {
					write_IOBuf(dev, "\r\n", 2);
					display->cursor = 0;
					auto_color_symbols = 1;
				}
			}
		}
/* modern color scheme: do automatic coloring of symbols */
		if ((display->flags & (DISPLAY_ANSI|DISPLAY_COLOR_SYMBOLS)) == (DISPLAY_ANSI|DISPLAY_COLOR_SYMBOLS)) {
			if (cstrchr(Default_Symbols, c) != NULL) {
				is_symbol = 1;
/*
	auto-coloring a single dot is ugly, but multiple ones is fun
*/
				if (c == '.' && str[1] != '.' && (pos > 0 && str[-1] != '.'))
					auto_color_symbols = 0;
			}
		}
		switch(c) {
			case '\r':
				putc_IOBuf(dev, '\r');
				display->cursor = 0;
				auto_color_symbols = 1;
				break;

			case '\n':
				write_IOBuf(dev, "\r\n", 2);
				display->cursor = 0;
				auto_color_symbols = 1;
				break;

			case '\b':
				putc_IOBuf(dev, '\b');
				if (display->cursor > 0)
					display->cursor--;
				break;

			case '<':
				n = long_color_code(dev, display, str);
				str += n;

				if (n > 0) {
					auto_color_symbols = 0;
					break;
				}
		/* fall through */

			default:
				if (is_symbol && auto_color_symbols)
					auto_color(dev, display);

				putc_IOBuf(dev, c);

				if (is_symbol && auto_color_symbols)
					restore_color(dev, display);

				if (!is_symbol)
					auto_color_symbols = 1;

				display->cursor++;
		}
		if (wrap_after) {					/* do word wrapping */
			wrap_after = 0;
			write_IOBuf(dev, "\r\n", 2);
			display->cursor = 0;
			auto_color_symbols = 1;
		}
		if (*str)
			str++;
	}
}

/*
	crappy word length routine, used for word-wrapping in color_output()
	It scans ahead at most only WORDWRAP_WIDTH characters
*/
int word_len(Display *display, char *str) {
int len;

	len = 0;
	while(*str) {
		switch(*str) {
			case '<':
				str += skip_long_color_code(display, str)-1;
				break;

			case '\r':
				return -10000;		/* fool him */

			case ' ':
			case '\n':
				return len;

			default:
				if (cstrchr(Wrap_Charset1, *str) != NULL) {
					len++;
					return len;
				}
				if (cstrchr(Wrap_Charset2, *str) != NULL)
					return len;

				if (*str >= ' ' && *str <= '~') {
					len++;
					if (len >= WORDWRAP_WIDTH)
						return len;
				}
		}
		if (*str)
			str++;
	}
	return len;
}

int long_color_code(IOBuf *dev, Display *display, char *code) {
int i, l;
char colorbuf[MAX_COLORBUF], buf[MAX_COLORBUF];

	for(i = 0; i < NUM_COLORS; i++) {
		bufprintf(colorbuf, sizeof(colorbuf), "<%s>", color_table[i].name);

		if (!cstrnicmp(code, colorbuf, strlen(colorbuf))) {
			if (display->flags & DISPLAY_ANSI) {
/*
	if it's already the right color, don't reprint the ANSI escape sequence
	NB. if we're still seeing colorbugs, it may be that we're not tracking this value correctly
*/
				if (display->current_color == i)
					return strlen(colorbuf)-1;

				if (display->flags & DISPLAY_BOLD)
					l = bufprintf(buf, sizeof(buf), "\x1b[1;%dm", color_table[i].code);
				else
					l = bufprintf(buf, sizeof(buf), "\x1b[%dm", color_table[i].code);

				display->current_color = i;

				write_IOBuf(dev, buf, l);
			}
			return strlen(colorbuf)-1;
		}
	}
	if (!cstrnicmp(code, "<hotkey>", 8)) {
		print_hotkey(dev, display, code[8]);
		return 8;
	}
	return 0;
}

int skip_long_color_code(Display *display, char *code) {
int i;
char colorbuf[MAX_COLORBUF];

	if (code == NULL || !*code || *code != '<')
		return 0;

	for(i = 0; i < NUM_COLORS; i++) {
		bufprintf(colorbuf, sizeof(colorbuf), "<%s>", color_table[i].name);
		if (!cstrnicmp(code, colorbuf, strlen(colorbuf)))
			return strlen(colorbuf);
	}
	if (!cstrnicmp(code, "<hotkey>", 8))
		return 8+skip_hotkey(display, code);

	return 1;
}

int color_strlen(Display *display, char *str) {
int len = 0, i;

	while(*str) {
		if (*str == '<') {
			i = skip_long_color_code(display, str);
			if (i == 1)
				len++;
			str += i;
		} else {
			if (*str >= ' ' && *str <= '~')
				len++;
			str++;
		}
	}
	return len;
}

/*
	return the 'wide' index of a 'screen' position within a color coded string
	e.g.
		buf[color_index(buf, 79)] = 0;
*/
int color_index(Display *display, char *str, int pos) {
int cpos, i;

	if (str == NULL || pos < 0)
		return 0;

	cpos = 0;
	while(*str && pos > 0) {
		if (*str == '<') {
			i = skip_long_color_code(display, str);
			if (i == 1)
				pos--;

			str += i;
			cpos += i;
		} else {
			if (*str >= ' ' && *str <= '~')
				pos--;

			str++;
			cpos++;
		}
	}
	return cpos;
}

void print_hotkey(IOBuf *dev, Display *display, char hotkey) {
char buf[MAX_COLORBUF];
int n;

	if (display->flags & DISPLAY_UPPERCASE_HOTKEYS)
		hotkey = ctoupper(hotkey);

	n = 0;

	if (display->flags & DISPLAY_ANSI) {
		if (display->flags & DISPLAY_BOLD_HOTKEYS) {
			if (!(display->flags & DISPLAY_BOLD))
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[1;%dm", HOTKEY_COLOR);
			else
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[%dm", HOTKEY_COLOR);
		} else {
			if (display->flags & DISPLAY_BOLD)
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[0;%dm", HOTKEY_COLOR);
			else
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[%dm", HOTKEY_COLOR);
		}
		if (display->flags & DISPLAY_HOTKEY_BRACKETS)
			n += bufprintf(buf+n, MAX_COLORBUF-n, "<%c>", hotkey);
		else
			buf[n++] = hotkey;

		if (display->flags & DISPLAY_BOLD_HOTKEYS) {
			if (!(display->flags & DISPLAY_BOLD))
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[0;%dm", color_table[display->current_color].code);
			else
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[%dm", color_table[display->current_color].code);
		} else {
			if (display->flags & DISPLAY_BOLD)
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[1;%dm", color_table[display->current_color].code);
			else
				n += bufprintf(buf+n, MAX_COLORBUF-n, "\x1b[%dm", color_table[display->current_color].code);
		}
	} else {
		if (display->flags & DISPLAY_BOLD_HOTKEYS) {
			if (!(display->flags & DISPLAY_BOLD)) {
				cstrcpy(buf+n, "\x1b[1m", MAX_COLORBUF);
				n += 4;
			}
		} else {
			if (display->flags & DISPLAY_BOLD) {
				cstrcpy(buf+n, "\x1b[0m", MAX_COLORBUF);
				n += 4;
			}
		}
		if (display->flags & DISPLAY_HOTKEY_BRACKETS)
			n += bufprintf(buf+n, MAX_COLORBUF-n, "<%c>", hotkey);
		else
			buf[n++] = hotkey;

		if (display->flags & DISPLAY_BOLD_HOTKEYS) {
			if (!(display->flags & DISPLAY_BOLD)) {
				cstrcpy(buf+n, "\x1b[0m", MAX_COLORBUF);
				n += 4;
			}
		} else {
			if (display->flags & DISPLAY_BOLD) {
				cstrcpy(buf+n, "\x1b[1m", MAX_COLORBUF);
				n += 4;
			}
		}
	}
	write_IOBuf(dev, buf, n);
}

int skip_hotkey(Display *display, char *code) {
	if (display->flags & DISPLAY_HOTKEY_BRACKETS)
		return 3;

	return 1;
}

/*
	do automatic coloring of symbols
*/
void auto_color(IOBuf *dev, Display *display) {
char buf[MAX_COLORBUF];
int len;
ColorIndex color;

	switch(display->current_color) {
		case COLOR_WHITE:
			color = COLOR_CYAN;
			break;

		case COLOR_RED:
		case COLOR_GREEN:
		case COLOR_BLUE:
		case COLOR_MAGENTA:
			color = COLOR_YELLOW;
			break;

		case COLOR_YELLOW:
		case COLOR_BLACK:
		case COLOR_CYAN:
		default:
			color = COLOR_WHITE;
	}
	if (display->flags & DISPLAY_BOLD)
		len = bufprintf(buf, MAX_COLORBUF, "\x1b[1;%dm", color_table[color].code);
	else
		len = bufprintf(buf, MAX_COLORBUF, "\x1b[%dm", color_table[color].code);

	write_IOBuf(dev, buf, len);
}

void restore_color(IOBuf *dev, Display *display) {
char buf[MAX_COLORBUF];
int len;

	if (display->flags & DISPLAY_BOLD)
		len = bufprintf(buf, MAX_COLORBUF, "\x1b[1;%dm", color_table[display->current_color].code);
	else
		len = bufprintf(buf, MAX_COLORBUF, "\x1b[%dm", color_table[display->current_color].code);

	write_IOBuf(dev, buf, len);
}

void restore_color_to(IOBuf *dev, Display *display, ColorIndex color) {
/*
	if it's already the right color, don't reprint the ANSI escape sequence
	NB. if we're still seeing colorbugs, it may be that we're not tracking this value correctly
*/
	if (display->current_color == color)
		return;

	display->current_color = color;
	restore_color(dev, display);
}

/*
	already processed text may contain ANSI escape sequences
	when printed (like through write_IOBuf(), as the pager does),
	the current_color should be updated
	this func gets the raw ANSI code from the text string
*/
int raw_color_filter(char *text) {
char *p;
int num;

	if ((p = cstrrchr(text, '\x1b')) == NULL)		/* find the last escape character */
		return 0;

	p++;
	if (*p != '[')
		return 0;

	p++;
	if (*p < '0' || *p > '9')
		return 0;

	if (p[1] == ';')
		p += 2;

	if (*p < 0 || *p > '9')
		return 0;

	if (p[1] == 'm')
		return 0;
	else
		if (p[2] == 'm')
			num = (*p - '0') * 10 + (p[1] - '0');
		else
			return 0;

	if (num >= ANSI_BLACK && num <= ANSI_WHITE)
		return num;

	return 0;
}

/* EOB */
