/*
	colors.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef COLORS_H_WJ109
#define COLORS_H_WJ109	1

/*
	ANSI color codes
*/
#define ANSI_BLACK		30
#define ANSI_RED		31
#define ANSI_GREEN		32
#define ANSI_YELLOW		33
#define ANSI_BLUE		34
#define ANSI_MAGENTA	35
#define ANSI_CYAN		36
#define ANSI_WHITE		37

/* TODO: change this */
#define HOTKEY_COLOR	33

typedef enum {
	COLOR_BLACK = 0,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_WHITE,

	NUM_COLORS
} ColorIndex;

typedef struct {
	char *name;
	int code;
} ColorTable;

extern ColorTable color_table[NUM_COLORS];

#endif	/* COLORS_H_WJ109 */

/* EOB */
