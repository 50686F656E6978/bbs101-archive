/*
	Telnet.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef TELNET_H_WJ109
#define TELNET_H_WJ109	1

#include "IOBuf.h"
#include "Display.h"

/*
	the value of 8 is ridiculously small, but OK for the BBS
	it only cares for NAWS bytes and the "USER" environment variable
*/
#define MAX_SUB_BUF			8

#define MAX_TERM			500

/* telnet states */
#define TS_DATA				0
#define TS_IAC				1
#define TS_ARG				2
#define TS_WILL				3
#define TS_DO				4
#define TS_NAWS				5
#define TS_NEW_ENVIRON		6
#define TS_NEW_ENVIRON_IS	7
#define TS_NEW_ENVIRON_VAR	8
#define TS_NEW_ENVIRON_VAL	9

typedef struct {
	int state, in_sub;
	int term_width, term_height;
	char in_sub_buf[MAX_SUB_BUF];
} Telnet;

void init_Telnet(Telnet *);
int telnet_negotiations(Telnet *, unsigned char, IOBuf *, Display *);

#endif	/* TELNET_H_WJ109 */

/* EOB */
