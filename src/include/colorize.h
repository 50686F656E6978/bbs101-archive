/*
	colorize.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef COLORIZE_H_WJ109
#define COLORIZE_H_WJ109	1

#include "IOBuf.h"
#include "Display.h"

#define WORDWRAP_WIDTH	16

void color_output(IOBuf *, Display *, char *);
int word_len(Display *, char *);
int long_color_code(IOBuf *, Display *, char *);
int skip_long_color_code(Display *, char *);
int color_strlen(Display *, char *);
int color_index(Display *, char *, int);
void print_hotkey(IOBuf *, Display *, char);
int skip_hotkey(Display *, char *);
void auto_color(IOBuf *, Display *);
void restore_color(IOBuf *, Display *);
void restore_color_to(IOBuf *, Display *, ColorIndex);
int raw_color_filter(char *);

#endif	/* COLORIZE_H_WJ109 */

/* EOB */
