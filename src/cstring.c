/*
	Chatter18	WJ97

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	cstring.c
*/

#include "config.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>


char *cstrdup(char *s) {
char *p;
int len;

	if (s == NULL)
		return NULL;

	len = strlen(s)+1;
	if ((p = (char *)Malloc(len)) == NULL)
		return NULL;

	cstrcpy(p, s, len);
	return p;
}

void cstrfree(char *s) {
	if (s == NULL)
		return;

	Free(s);
}

char *cstrlwr(char *s) {
char *p;

	if (s == NULL)
		return NULL;

	for(p = s; *p; p++)
		*p = ctolower(*p);

	return s;
}

char *cstrupr(char *s) {
char *p;

	if (s == NULL)
		return NULL;

	for(p = s; *p; p++)
		*p = ctoupper(*p);

	return s;
}

char *cstristr(char *s, char *substr) {
int i;

	if (!substr)
		return NULL;

	i = strlen(substr);
	while(s && *s) {
		if (!cstrnicmp(s, substr, i))
			return s;
		s++;
	}
	return NULL;
}

char *cstrstr(char *s, char *substr) {
int i;

	if (!substr)
		return NULL;

	i = strlen(substr);
	while(s && *s) {
		if (!strncmp(s, substr, i))
			return s;
		s++;
	}
	return NULL;
}

char *cstrichr(char *str, char kar) {
char *p;

	if (str == NULL || !*str)
		return NULL;

	if ((p = cstrchr(str, kar)) != NULL)
		return p;

	if (kar >= 'A' && kar <= 'Z')
		kar += ' ';
	else
		if (kar >= 'a' && kar <= 'z')
			kar -= ' ';
		else {
			return NULL;
		}
	p = cstrchr(str, kar);
	return p;
}

void chop(char *msg) {
int i;

	if (msg == NULL)
		return;

	i = strlen(msg)-1;
	while((i >= 0) && (msg[i] == '\r' || msg[i] == '\n'))
		msg[i--] = 0;
}

void cstrip_line(char *msg) {
char *p;
int i;

	if (msg == NULL || !*msg)
		return;

	while((p = cstrchr(msg, 27)) != NULL)		/* filter escape characters */
		memmove(p, p+1, strlen(p)+1);
	while((p = cstrchr(msg, '\t')) != NULL)		/* tab2space */
		*p = ' ';

	i = strlen(msg);
	while(i && *msg == ' ')
		memmove(msg, msg+1, i--);

	i--;
	while(i >= 0 && (msg[i] == ' ' || msg[i] == '\r' || msg[i] == '\n'))
		msg[i--] = 0;
}

void ctrim_line(char *str) {
int i;

	if (str == NULL)
		return;

	i = strlen(str)-1;
	while(i >= 0 && (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n'))
		str[i--] = 0;
}

void cstrip_spaces(char *msg) {
char *p;

	if (msg == NULL)
		return;

	p = msg;
	while((p = cstrchr(p, ' ')) != NULL) {
		p++;
		while(*p == ' ')
			memmove(p, p+1, strlen(p));		/* strlen(p+1)+1 */
	}
}

/*
	Splits a line into an array
	Just like Perl's split() function
	The substrings are not copied into the array; the string that is
	passed as parameter to split() is chopped in pieces

	The return value is allocated, it must be Free()d
	*n == number of elements
*/
char **cstrsplit(char *line, char token, int *n) {
char **args, *p, *startp;
int num = 2;

	if (n != NULL)
		*n = 0;

	if (!line || !*line)
		return NULL;

	p = line;
	while((p = cstrchr(p, token)) != NULL) {
		num++;
		p++;
	}
	if ((args = (char **)Malloc(num * sizeof(char *))) == NULL)
		return NULL;

	startp = p = line;
	num = 0;
	while((p = cstrchr(p, token)) != NULL) {
		args[num++] = startp;
		*p = 0;
		p++;
		startp = p;
	}
	args[num++] = startp;
	args[num] = NULL;

	if (n != NULL)
		*n = num;

	return args;
}

/*
	Joins splitted string-parts into one string again
	Use Free() to free the allocated string again
*/
char *cstrjoin(char **args) {
char *buf;
int i, l, buflen = 0;

	for(i = 0; args[i] != NULL && *args[i]; i++)
		buflen += strlen(args[i]) + 1;
	buflen++;
	if ((buf = (char *)Malloc(buflen)) == NULL)
		return NULL;

	*buf = 0;
	if (args == NULL)
		return buf;

	l = 0;
	for(i = 0; args[i] != NULL && *args[i]; i++) {
		cstrcat(buf, args[i], buflen);
		l = strlen(buf);
		buf[l++] = ' ';
		buf[l] = 0;
	}
	if (l)
		buf[--l] = 0;
	return buf;
}

/*
	you can also do this with strspn(), but then I'd need to write a #ifdef HAVE_STRSPN
	construction ... and this routine just always works
*/
int is_numeric(char *str) {
	if (str == NULL)
		return 0;

	if (*str == '-' || *str == '+')
		str++;

	while(*str) {
		if (*str >= '0' && *str <= '9')
			str++;
		else
			return 0;
	}
	return 1;
}

int is_hexadecimal(char *str) {
	if (str == NULL)
		return 0;

	if (*str == '-' || *str == '+')
		str++;

	if (*str == '0') {
		str++;
		if (*str == 'x')
			str++;
	}
	while(*str) {
		if ((*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F'))
			str++;
		else
			return 0;
	}
	return 1;
}

int is_octal(char *str) {
	if (str == NULL)
		return 0;

	if (*str == '-' || *str == '+')
		str++;

	while(*str) {
		if (*str >= '0' && *str <= '7')
			str++;
		else
			return 0;
	}
	return 1;
}

unsigned long cstrtoul(char *str, int base) {
long l = -1L;

	if (str == NULL || !*str)
		return 0UL;

	if (base == 10)
		return (unsigned long)atol(str);

	if (base == 16) {
		sscanf(str, "%lx", &l);
		return (unsigned long)l;
	}
	if (base == 8) {
		sscanf(str, "%lo", &l);
		return (unsigned long)l;
	}
	return (unsigned long)-1L;
}

/*
	this is a cstrmatch() helper function
*/
int cstrmatch_char(char buf, char pattern) {
	switch(pattern) {
		case 'A':
			if (buf >= 'A' && buf <= 'Z')
				break;

			return 0;

		case 'a':
			if ((buf >= 'A' && buf <= 'Z') || (buf >= 'a' && buf <= 'z'))
				break;

			return 0;

		case 'd':
			if (buf >= '0' && buf <= '9')
				break;

			return 0;

		case '?':
			break;

		case ' ':
			if (buf == ' ' || buf == '\t')
				break;

			return 0;

		case '\n':
			if (buf == '\n' || buf == '\r')
				break;

			return 0;

		case '$':
			if (buf == ' ' || buf == '\t' || buf == '\r' || buf == '\n'
				|| (buf >= 'A' && buf <= 'Z') || (buf >= 'a' && buf <= 'z')
				|| (buf >= '0' && buf <= '9'))
				return 0;

			break;

		default:
			if (buf == pattern)
				break;

			return 0;
	}
	return 1;
}

/*
	my simple string matcher
	it's not very powerful, but it's OK for doing 'one-on-one' matches
*/
int cstrmatch(char *buf, char *pattern) {
	if (buf == NULL || pattern == NULL || !*pattern)
		return 0;

	while(*pattern) {
		if (!*buf)
			return 0;

		switch(*pattern) {
			case '\n':
				if (*buf == '\r' || *buf == '\n') {
					buf++;
					while(*buf == '\r' || *buf == '\n')
						buf++;
					buf--;
				} else
					return 0;

				break;

			case '*':
				buf++;
				pattern++;
				if (!*pattern)
					return 1;

				if (!*buf)
					return 0;

/* find the point where it matches again */
				while(!cstrmatch_char(*buf, *pattern)) {
					buf++;
					if (!*buf)
						return 0;		/* not found, so it doesn't match */
				}
				break;

			default:
				if (!cstrmatch_char(*buf, *pattern))
					return 0;
		}
		buf++;
		pattern++;
	}
	return 1;
}

/*
	find value in (comma-)seperated list stored in a string
*/
int cstrfind(char *str, char *value, char delim) {
char *p, *delim_p;

	if (str == NULL || !*str || value == NULL || !*value)
		return 0;

	p = str;
	while((delim_p = cstrchr(p, delim)) != NULL) {
		*delim_p = 0;

		if (!strcmp(p, value)) {
			*delim_p = delim;
			return 1;
		}
		*delim_p = delim;
		p = delim_p;
		p++;
	}
	if (!strcmp(p, value))
		return 1;

	return 0;
}

/*
	remove value from (comma-)seperated list stored in a string
*/
int cstrremove(char *str, char *value, char delim) {
char *p, *delim_p;

	if (str == NULL || !*str || value == NULL || !*value)
		return 0;

	p = str;
	while((delim_p = cstrchr(p, delim)) != NULL) {
		*delim_p = 0;

		if (!strcmp(p, value)) {
			*delim_p = delim;
			delim_p++;
			memmove(p, delim_p, strlen(delim_p)+1);
			return 1;
		}
		*delim_p = delim;
		p = delim_p;
		p++;
	}
	if (!strcmp(p, value)) {
		int l;

		*p = 0;
		l = strlen(str)-1;
		if (l > 0 && str[l] == ',')
			str[l] = 0;
		return 1;
	}
	return 0;
}

/*
	count number of occurres of c in s
*/
int cstrcount(char *s, char c) {
int n;

	if (s == NULL)
		return 0;

	n = 0;
	while((s = cstrchr(s, c)) != NULL) {
		n++;
		s++;
	}
	return n;
}

/* EOB */
