/*
	Display.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "Display.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>


/*
	warning: order is important
*/
static char *display_flags[] = {
	"DISPLAY_BEEP",
	"DISPLAY_ANSI",
	"DISPLAY_BOLD",
	"DISPLAY_BOLD_HOTKEYS",
	"DISPLAY_HOTKEY_BRACKETS",
	"DISPLAY_UPPERCASE_HOTKEYS",
	"DISPLAY_COLOR_SYMBOLS",
	"DISPLAY_FORCE_TERM"
};


void init_Display(Display *d) {
	d->term_width = TERM_WIDTH;
	d->term_height = TERM_HEIGHT;
	d->flags = DISPLAY_HOTKEY_BRACKETS;
	d->current_color = COLOR_WHITE;
}

void resize_Display(Display *d, int w, int h) {
	d->term_width = w;
	d->term_height = h;
}

void save_Display_flags(Display *d, GString *gstr) {
	flags_to_str(d->flags, gstr, display_flags, sizeof(display_flags) / sizeof(char *));
}

void load_Display_flags(Display *d, char *str) {
	d->flags = str_to_flags(str, display_flags, sizeof(display_flags) / sizeof(char *));
}

/* EOB */
