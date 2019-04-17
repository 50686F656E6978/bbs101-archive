/*
	bufprintf.c	WJ105

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	place-holder functions for snprintf() and vsnprintf(), which may not be
	present everywhere yet

	Do not call these function directly
	Use the defines bufprintf() and bufvprintf() instead
*/

#include "config.h"
#include "bufprintf.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef HAVE_VPRINTF
#error This package uses vprintf(), which you do not have
#endif


int bufprintf(char *buf, int buflen, char *fmt, ...) {
va_list args;

	va_start(args, fmt);
	return bufvprintf(buf, buflen, fmt, args);
}

int bufvprintf(char *buf, int buflen, char *fmt, va_list args) {
int ret;

	if (buflen <= 0)
		return 0;

	if (args == NULL) {						/* some v*printf() implementations may bomb on args == NULL */
		strncpy(buf, fmt, buflen);
		buf[buflen-1] = 0;
		return strlen(buf);
	}
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, buflen, fmt, args);
	ret = strlen(buf);						/* <-- important! */
#else
	ret = vsprintf(buf, fmt, args);
	assert(ret < buflen);
#endif
	va_end(args);
	return ret;
}

/* EOB */
