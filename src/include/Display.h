/*
	Display.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef DISPLAY_H_WJ109
#define DISPLAY_H_WJ109	1

#include "GString.h"
#include "colors.h"

#define TERM_WIDTH		80
#define TERM_HEIGHT		24

/*
	display flags
*/
#define DISPLAY_BEEP				1
#define DISPLAY_ANSI				2
#define DISPLAY_BOLD				4
#define DISPLAY_BOLD_HOTKEYS		8
#define DISPLAY_HOTKEY_BRACKETS		0x10
#define DISPLAY_UPPERCASE_HOTKEYS	0x20
#define DISPLAY_COLOR_SYMBOLS		0x40
#define DISPLAY_FORCE_TERM			0x80

typedef struct {
	int term_width, term_height, flags, cursor;
	ColorIndex current_color;
} Display;

void init_Display(Display *);
void resize_Display(Display *, int, int);
void save_Display_flags(Display *, GString *);
void load_Display_flags(Display *, char *);

#endif	/* DISPLAY_H_WJ109 */

/* EOB */
