/*
	pager.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "pager.h"
#include "keys.h"
#include "cstring.h"
#include "util.h"
#include "Memory.h"
#include "colorize.h"

#include <stdio.h>
#include <stdlib.h>


void init_pager(User *usr) {
	reset_GString(&(usr->pager_buf));
	redirect_IOBuf(&(usr->iobuf), &(usr->pager_buf));
}

static void more_prompt(User *usr, int total, int pos, int *color_lines) {
	Put(usr, "<yellow>");
	Print(usr, "--More--<cyan> (line %d/%d %d%%)", pos, total, 100 * pos / total);

	if (color_lines != NULL)
		raw_user_color(usr, color_lines[pos]);
}

void pager(User *usr) {
char **lines;
int num_lines, i, n, c, top, end, break_out, display_page, *color_lines;

	redirect_IOBuf(&(usr->iobuf), NULL);

	if ((lines = cstrsplit(usr->pager_buf.str, '\n', &num_lines)) == NULL) {
		Put(usr, "<red>Due to an error, we're unable to display the paged text\n");
		return;
	}
	num_lines--;			/* last entry is only a NULL pointer */
	if (num_lines < 1)
		num_lines = 1;

/*
	put the color for each line in a color array
	this may seem stupid at first, but we are going to scroll text and otherwise it becomes a mess of colorbugs
*/
	if (usr->display.flags & DISPLAY_ANSI) {
		if ((color_lines = (int *)Malloc((num_lines+1) * sizeof(int))) != NULL) {
			c = ANSI_GREEN;
			for(i = 0; i < num_lines; i++) {
				color_lines[i] = c;

				if ((n = raw_color_filter(lines[i])) != 0)
					c = n;
			}
		}
	} else
		color_lines = NULL;

	top = 0;
	break_out = 0;

	usr->runtime_flags |= RTF_BUSY;

	for(;;) {
/*
	display page
*/
		end = top + usr->display.term_height-1;

		if (color_lines != NULL)
			raw_user_color(usr, color_lines[top]);

		for(i = top; i < end; i++) {
			if (i >= num_lines) {
				break_out = 1;
				break;
			}
/*
	these lines have already been processed/rendered, so now it's OK to
	do a raw write to the output buffer
*/
			write_IOBuf(&(usr->iobuf), lines[i], strlen(lines[i]));
			write_IOBuf(&(usr->iobuf), "\r\n", 2);
		}
		if (break_out)
			break;

		more_prompt(usr, num_lines, end, color_lines);

		display_page = 0;
		while(!display_page) {
/*
	NB. iGetch() is needed here instead of Getch() so that we can Free(lines) on error
*/
			c = iGetch(usr);

			switch(c) {
				case IO_ERR:
					Free(lines);
					Free(color_lines);
					linkdead(usr);
					return;

				case IO_EINTR:				/* interrupted, no input ready */
					break;

				case KEY_CTRL('C'):
				case KEY_CTRL('D'):
				case 'q':
				case 'Q':
				case KEY_ESC:
					erase_line(usr);
					display_page = 0;

					break_out = 1;
					break;

				case ' ':
				case KEY_PAGEDOWN:
					erase_line(usr);

					n = num_lines - end;
					if (n <= 0) {
						break_out = 1;
						break;
					}
					if (n > usr->display.term_height-1) {
						display_page = 1;
						top = end;
						break;
					}
/*
	near the end, only print the necessary lines
*/
					display_page = 0;

					if (color_lines != NULL)
						raw_user_color(usr, color_lines[end]);

					for(i = end; i < end + n; i++) {
						write_IOBuf(&(usr->iobuf), lines[i], strlen(lines[i]));
						write_IOBuf(&(usr->iobuf), "\r\n", 2);
					}
					top += n;
					end += n;

					more_prompt(usr, num_lines, end, color_lines);
					break;

				case 'b':
				case KEY_PAGEUP:
					if (top <= 0)
						break;

					if (top >= usr->display.term_height-1 || !(usr->display.flags & DISPLAY_ANSI)) {
						erase_line(usr);
						display_page = 1;
						top -= (usr->display.term_height-1);
						if (top < 0)
							top = 0;
						break;
					}
					display_page = 0;

					n = top;

					top = 0;
					end = usr->display.term_height-1;

					save_cursor(usr);
					scroll_down(usr, n);
					home_cursor(usr);

					if (color_lines != NULL)
						raw_user_color(usr, color_lines[0]);

					for(i = 0; i < n; i++) {
						write_IOBuf(&(usr->iobuf), lines[i], strlen(lines[i]));
						write_IOBuf(&(usr->iobuf), "\r\n", 2);
					}
					restore_cursor(usr);
					erase_line(usr);

					more_prompt(usr, num_lines, end, color_lines);
					break;

				case KEY_RETURN:
				case '+':
				case KEY_DOWN:
					erase_line(usr);
/*
	print just one new line, do not (re)display a full page
*/
					display_page = 0;

					if (end >= num_lines) {
						break_out = 1;
						break;
					}
					if (color_lines != NULL)
						raw_user_color(usr, color_lines[end]);

					write_IOBuf(&(usr->iobuf), lines[end], strlen(lines[end]));
					write_IOBuf(&(usr->iobuf), "\r\n", 2);

					top++;
					end++;
					if (end > num_lines) {
						break_out = 1;
						break;
					}
					more_prompt(usr, num_lines, end, color_lines);
					break;

				case KEY_BS:
				case '-':
				case KEY_UP:
					if (top <= 0)
						break;

					if (!(usr->display.flags & DISPLAY_ANSI)) {
						erase_line(usr);
						top--;
						display_page = 1;
						break;
					}
/*
	scroll the screen and print just one new line, do not (re)display a full page
*/
					display_page = 0;

					top--;
					end--;

					save_cursor(usr);
					scroll_down(usr, 1);
					home_cursor(usr);

					if (color_lines != NULL)
						raw_user_color(usr, color_lines[top]);

					write_IOBuf(&(usr->iobuf), lines[top], strlen(lines[top]));
					write_IOBuf(&(usr->iobuf), "\r\n", 2);

					restore_cursor(usr);
					erase_line(usr);

					more_prompt(usr, num_lines, end, color_lines);
					break;

				case KEY_CTRL('L'):			/* reprint */
					erase_line(usr);
					display_page = 1;
					break;

				case 'g':
				case KEY_HOME:
					if (top <= 0)
						break;

					if (top >= usr->display.term_height-1 || !(usr->display.flags & DISPLAY_ANSI)) {
						erase_line(usr);
						display_page = 1;
						top = 0;
						break;
					}
					display_page = 0;

					n = top;

					top = 0;
					end = usr->display.term_height-1;

					save_cursor(usr);
					scroll_down(usr, n);
					home_cursor(usr);

					if (color_lines != NULL)
						raw_user_color(usr, color_lines[0]);

					for(i = 0; i < n; i++) {
						write_IOBuf(&(usr->iobuf), lines[i], strlen(lines[i]));
						write_IOBuf(&(usr->iobuf), "\r\n", 2);
					}
					restore_cursor(usr);
					erase_line(usr);

					more_prompt(usr, num_lines, end, color_lines);
					break;

				case 'G':
				case KEY_END:
					n = num_lines - (usr->display.term_height-1) - top;
					if (n <= 0)
						break;

					if (n >= usr->display.term_height-1) {
						erase_line(usr);
						display_page = 1;
						top = num_lines - (usr->display.term_height-1);
						break;
					}
/*
	near the end, only print the lines that are needed
*/
					display_page = 0;

					erase_line(usr);

					if (color_lines != NULL)
						raw_user_color(usr, color_lines[end]);

					for(i = end; i < end + n; i++) {
						write_IOBuf(&(usr->iobuf), lines[i], strlen(lines[i]));
						write_IOBuf(&(usr->iobuf), "\r\n", 2);
					}
					top += n;
					end += n;

					more_prompt(usr, num_lines, end, color_lines);
					break;

				default:
					display_page = 0;
			}
			if (break_out)
				break;
		}
		if (break_out)
			break;
	}
	Free(lines);
	Free(color_lines);
	reset_GString(&(usr->pager_buf));
}

/*
	display a large piece of text using a --More-- prompt
*/
void page_text(User *usr, char *text) {
char *p;

	init_pager(usr);

	p = text;
	while((p = cstrchr(text, '\n')) != NULL) {
		*p = 0;

		Put(usr, text);
		Put(usr, "\n");

		*p = '\n';
		text = p + 1;
	}
	if (text != NULL && *text)
		Put(usr, text);

	pager(usr);
}

/* EOB */
