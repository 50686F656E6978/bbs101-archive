/*
	util.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef UTIL_H_WJ109
#define UTIL_H_WJ109	1

#include "User.h"

#include <stdlib.h>
#include <time.h>

#define RND_STR(x)		((x)[rand() % (sizeof(x)/sizeof(char *))])

#define YESNO_YES		1
#define YESNO_NO		0
#define YESNO_UNDEF		-1

extern char *Days[7];

int spawn(void *(*)(void*), void *);
int yesno(User *, char *, int);

void reset_colors(User *);
void beep(User *);
void erase(User *, int);
void erase_line(User *);
void clear_screen(User *);
void save_cursor(User *);
void restore_cursor(User *);
void home_cursor(User *);
void scroll_up(User *usr, int);
void scroll_down(User *usr, int);

void flags_to_str(int, GString *, char **, int);
int str_to_flags(char *, char **, int);

char *sprint_date(struct tm *, int, char *, int);
char *sprint_time(struct tm *, int, char *, int);
char *sprint_total_time(unsigned long, char *, int);
char *sprint_number(unsigned long, int, char *, int);
char *sprint_number_commas(unsigned long, char *, int);
char *sprint_number_dots(unsigned long, char *, int);
char *sprint_numberth(int, char *);

void hline(User *, char);
void center(User *, char *);

void download_text(User *, char *);
void display_file(User *, char *);

#endif	/* UTIL_H_WJ109 */

/* EOB */
