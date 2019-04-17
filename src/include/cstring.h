/*
	Chatter18	WJ97

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	cstring.h
*/

#ifndef _CSTRING_H_WJ97
#define _CSTRING_H_WJ97 1

#include "config.h"
#include "cstrcpy.h"
#include "memset.h"

#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_STRCHR
#define cstrchr(x,y)		strchr(x,y)
#else
#define cstrchr(x,y)		index((x), (y))
#endif

#ifdef HAVE_STRRCHR
#define cstrrchr(x,y)		strrchr(x,y)
#else
#define cstrrchr(x,y)		rindex((x), (y))
#endif

#define cstricmp(x,y)		strcasecmp((x), (y))
#define cstrnicmp(x,y,z)	strncasecmp((x), (y), (z))

#define ctoupper(x)			((((x) >= 'a') && ((x) <= 'z')) ? ((x) - ' ') : (x))
#define ctolower(x)			((((x) >= 'A') && ((x) <= 'Z')) ? ((x) + ' ') : (x))

char *cstrdup(char *);
void cstrfree(char *);
char *cstrlwr(char *);
char *cstrupr(char *);
char *cstristr(char *, char *);
char *cstrstr(char *, char *);
char *cstrichr(char *, char);

void chop(char *);
void cstrip_line(char *);
void ctrim_line(char *);
void cstrip_spaces(char *);
char **cstrsplit(char *, char, int *);
char *cstrjoin(char **);

int is_numeric(char *);
int is_hexadecimal(char *);
int is_octal(char *);

unsigned long cstrtoul(char *, int);

int cstrmatch_char(char, char);
int cstrmatch(char *, char *);
int cstrfind(char *, char *, char);
int cstrremove(char *, char *, char);
int cstrcount(char *, char);

#endif /* _CSTRING_H_WJ97 */

/* EOB */
