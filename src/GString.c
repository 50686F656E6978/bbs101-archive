/*
	GString.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- the GString is a growable string
	  (much like StringIO, only simpler to use)
*/

#include "config.h"
#include "GString.h"
#include "Memory.h"
#include "cstring.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void init_GString(GString *g) {
	memset(g, 0, sizeof(GString));
	g->grow = GSTRING_GROW;
}

void deinit_GString(GString *g) {
	Free(g->str);
	memset(g, 0, sizeof(GString));
}

GString *new_GString(void) {
GString *g;

	if ((g = (GString *)Malloc(sizeof(GString))) == NULL)
		return NULL;

	init_GString(g);
	return g;
}

void destroy_GString(GString *g) {
	if (g == NULL)
		return;

	Free(g->str);
	Free(g);
}

void deepcopy_GString(GString *g, GString *copy) {
	if (g->size > 0) {
		if ((copy->str = (char *)Malloc(g->size)) == NULL)
			return;

		cstrcpy(copy->str, g->str, g->size);
	} else
		copy->str = NULL;

	copy->size = g->size;
	copy->len = g->len;
}

/*
	set size with which a GString stretches
*/
void spurt_GString(GString *g, int spurt) {
	g->grow = spurt;
}

int grow_GString(GString *g, int newsize) {
char *p;

/*
	grow in spurts rather than in exact matches
	I like to keep the code from reallocing all the time
*/
	newsize = (newsize / g->grow + 1) * g->grow;

	if ((p = (char *)Realloc(g->str, newsize)) == NULL)
		return -1;

	g->str = p;
	g->size = newsize;
	return 0;
}

int shrink_GString(GString *g) {
int newsize;
char *p;

	newsize = (g->len / g->grow + 1) * g->grow;

	if (newsize >= g->size)
		return 0;

	if ((p = (char *)Realloc(g->str, newsize)) == NULL)
		return -1;

	g->str = p;
	g->size = newsize;
	return 0;
}

void reset_GString(GString *g) {
char *p;

	if (g->str == NULL)
		return;

	g->str[0] = 0;
	g->len = 0;

	if (g->size > g->grow) {
		if ((p = (char *)Realloc(g->str, g->grow)) == NULL)
			return;

		g->str = p;
		g->size = g->grow;
	}
}

int gstrcpy(GString *g, char *s) {
int l;

	if (s == NULL || !*s)
		return 0;

	l = strlen(s)+1;
	if (g->size < l && grow_GString(g, l) == -1)
		return -1;

	cstrcpy(g->str, s, g->size);
	g->len = l-1;
	return 0;
}

int gstrcat(GString *g, char *s) {
int l;

	if (s == NULL || !*s)
		return 0;

	l = g->len + strlen(s)+1;
	if (g->size < l && grow_GString(g, l) == -1)
		return -1;

	cstrcpy(g->str + g->len, s, g->size - g->len);
	g->len = l-1;
	return 0;
}

int gprintf(GString *g, char *fmt, ...) {
int n;
va_list ap;

	if (g == NULL || fmt == NULL)
		return -1;

	if (g->str == NULL && grow_GString(g, g->grow) == -1)
		return -1;

	va_start(ap, fmt);

	while(1) {
		n = bufvprintf(g->str, g->size, fmt, ap);
		if (n == -1 || n >= g->size-1) {
			if (grow_GString(g, g->size + g->grow-1) == -1) {
				va_end(ap);
				return -1;
			}
			va_end(ap);
			va_start(ap, fmt);
			continue;
		}
		g->len += n;
		break;
	}
	va_end(ap);
	return n;
}

/*
	same as gprintf(), except that it adds the string to the end
*/
int gprint_add(GString *g, char *fmt, ...) {
int n;
va_list ap;

	if (g == NULL || fmt == NULL)
		return -1;

	if (g->str == NULL && grow_GString(g, g->grow) == -1)
		return -1;

	va_start(ap, fmt);

	while(1) {
		if (g->size - g->len <= 0)
			n = -1;
		else
			n = bufvprintf(g->str + g->len, g->size - g->len, fmt, ap);

		if (n == -1 || n >= g->size - g->len - 1) {
			if (grow_GString(g, g->size + g->grow-1) == -1) {
				va_end(ap);
				return -1;
			}
			va_end(ap);
			va_start(ap, fmt);
			continue;
		}
		g->len += n;
		break;
	}
	va_end(ap);
	return n;
}

int gputc(GString *g, int c) {
	if (g->size >= g->len && grow_GString(g, g->size + g->grow-1) == -1)
		return -1;

	g->str[g->len++] = c;
	return 0;
}

/* EOB */
