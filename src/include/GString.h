/*
	GString.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef GSTRING_H_WJ109
#define GSTRING_H_WJ109	1

#define GSTRING_GROW	128

typedef struct {
	int size, len, grow;
	char *str;
} GString;

void init_GString(GString *);
void deinit_GString(GString *);
GString *new_GString(void);
void destroy_GString(GString *);
void deepcopy_GString(GString *, GString *);
void spurt_GString(GString *, int);
int grow_GString(GString *, int);
int shrink_GString(GString *);
void reset_GString(GString *);
int gstrcpy(GString *, char *);
int gstrcat(GString *, char *);
int gprintf(GString *, char *, ...);
int gprint_add(GString *, char *, ...);
int gputc(GString *, int);

#endif	/* GSTRING_H_WJ109 */

/* EOB */
