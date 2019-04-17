/*
	msg_system.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef MSG_SYSTEM_H_WJ109
#define MSG_SYSTEM_H_WJ109	1

#include "User.h"

int sent_xmsg_stats(User *, XMsg *, char *);
void listdestroy_PList_XMsg(PList *);
int send_xmsg_bcc(User *, XMsg *);
int recv_XMsg(User *, XMsg *);
XMsg *unseen_XMsg(User *);
void enter_xmsg(User *);
void print_XMsg(User *usr, XMsg *);
int received_new_XMsgs(User *);
void enter_emote(User *);

#endif	/* MSG_SYSTEM_H_WJ109 */

/* EOB */
