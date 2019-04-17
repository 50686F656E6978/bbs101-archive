/*
	XMsg.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef XMSG_H_WJ109
#define XMSG_H_WJ109	1

#include "config.h"
#include "defines.h"
#include "sys_time.h"

typedef enum {
	XMSG_X = 0,
	XMSG_SYSTEM,
	XMSG_NOTIFY,
	XMSG_EMOTE,
	XMSG_FEELING,
	XMSG_QUESTION,
	XMSG_ANSWER
} XMsgType;

typedef struct {
	int refcount;
	XMsgType type;

	char from[MAX_NAME];
	char *recipients, *msg;

	time_t mtime;
} XMsg;

XMsg *new_XMsg(void);
void destroy_XMsg(XMsg *);

void set_XMsg(XMsg *, XMsgType, char *, char *, time_t);
int add_XMsg_recipient(XMsg *, char *);
int add_XMsg_line(XMsg *, char *);

#endif	/* XMSG_H_WJ109 */

/* EOB */
