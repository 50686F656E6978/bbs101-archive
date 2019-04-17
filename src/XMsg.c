/*
	XMsg.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "XMsg.h"
#include "Memory.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>


XMsg *new_XMsg(void) {
XMsg *x;

	if ((x = (XMsg *)Malloc(sizeof(XMsg))) == NULL)
		return NULL;

	return x;
}

void destroy_XMsg(XMsg *x) {
	if (x == NULL)
		return;

	if (x->refcount > 0)
		return;

	Free(x->recipients);
	Free(x->msg);
	Free(x);
}

void set_XMsg(XMsg *x, XMsgType type, char *name, char *recipients, time_t t) {
	x->type = type;
	cstrcpy(x->from, name, MAX_NAME);
	x->recipients = cstrdup(recipients);
	x->mtime = t;
}

int add_XMsg_recipient(XMsg *x, char *name) {
	if (x->recipients == NULL) {
		if ((x->recipients = cstrdup(name)) == NULL)
			return -1;
	} else {
		char *p;
		int l;

		l = strlen(x->recipients) + 1 + strlen(name) + 1;
		if ((p = (char *)Realloc(x->recipients, l)) == NULL)
			return -1;

		x->recipients = p;

		cstrcat(x->recipients, ",", l);
		cstrcat(x->recipients, name, l);
	}
	return 0;
}

int add_XMsg_line(XMsg *x, char *line) {
	if (x->msg == NULL) {
		if ((x->msg = cstrdup(line)) == NULL)
			return -1;
	} else {
		char *p;
		int l;

		l = strlen(x->msg) + 1 + strlen(line) + 1;
		if ((p = (char *)Realloc(x->msg, l)) == NULL)
			return -1;

		x->msg = p;

		cstrcat(x->msg, "\n", l);
		cstrcat(x->msg, line, l);
	}
	return 0;
}

/* EOB */
