/*
	Telnet.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "Telnet.h"
#include "keys.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/telnet.h>

#ifndef TELOPT_NAWS
#define TELOPT_NAWS			31		/* negotiate about window size */
#endif

#ifndef TELOPT_NEW_ENVIRON
#define TELOPT_NEW_ENVIRON	39		/* set new environment variable */
#endif

#define MAX_TELNETBUF		20


void init_Telnet(Telnet *t) {
	t->state = TS_DATA;
	t->term_height = TERM_HEIGHT;		/* hard-coded defaults; may be set by TELOPT_NAWS */
	t->term_width = TERM_WIDTH;
}

/*
	handle the telnet protocol -- using a state machine

	the interface of this function is a bit weird with the IOBuf and Display arguments,
	but I don't have an idea of how to attach this kind of hooks otherwise
*/
int telnet_negotiations(Telnet *t, unsigned char c, IOBuf *iobuf, Display *display) {
char buf[MAX_TELNETBUF];

	switch(t->state) {
		case TS_DATA:
			switch(c) {
				case 0:
				case '\n':
					return -1;

				case 0x7f:			/* DEL/BS conversion */
					c = KEY_BS;
					break;

				case IAC:
					t->state = TS_IAC;
					return -1;

				default:
					if (c > 0x7f) {
						return -1;
					}
			}
			return c;

		case TS_IAC:
			switch(c) {
				case IAC:				/* IAC IAC received */
					t->state = TS_DATA;
					return 0;			/* return 0, although the BBS probably won't like it much ... */

				case AYT:
					write_IOBuf(iobuf, "\r\n[YeS]\r\n", 9);
					t->state = TS_DATA;
					return -1;

				case NOP:
					t->state = TS_DATA;
					return -1;

				case SB:
					t->in_sub++;
					return -1;

				case SE:
					t->in_sub--;
					if (t->in_sub < 0)
						t->in_sub = 0;
					t->state = TS_DATA;
					return -1;

				case WILL:
					t->state = TS_WILL;
					return -1;

				case DO:
					t->state = TS_DO;
					return -1;

/* after a SB we can have... */
				case TELOPT_NAWS:
					t->state = TS_NAWS;
					return -1;

				case TELOPT_NEW_ENVIRON:
					t->state = TS_NEW_ENVIRON;
					return -1;
			}
			t->state = TS_ARG;
			return -1;


		case TS_WILL:
			switch(c) {
				case TELOPT_NAWS:
					break;

				case TELOPT_NEW_ENVIRON:		/* NEW-ENVIRON SEND */
					bufprintf(buf, sizeof(buf), "%c%c%c%c%c%c", IAC, SB, TELOPT_NEW_ENVIRON, 1, IAC, SE);
					write_IOBuf(iobuf, buf, 6);
					break;

				default:
					bufprintf(buf, sizeof(buf), "%c%c%c", IAC, DONT, c);
					write_IOBuf(iobuf, buf, 3);
			}
			t->state = TS_DATA;
			return -1;

		case TS_DO:
			switch(c) {
				case TELOPT_SGA:
					break;

				case TELOPT_ECHO:
					break;

				default:
					bufprintf(buf, sizeof(buf), "%c%c%c", IAC, WONT, c);
					write_IOBuf(iobuf, buf, 3);
			}
			t->state = TS_DATA;
			return -1;


		case TS_NAWS:
			if (t->in_sub <= 4)						/* expect next NAWS byte */
				t->in_sub_buf[t->in_sub++] = c;
			else {
				int width, height;

				width = (unsigned int)t->in_sub_buf[1] & 0xff;
				width <<= 8;
				width |= ((unsigned int)t->in_sub_buf[2] & 0xff);

				height = (unsigned int)t->in_sub_buf[3] & 0xff;
				height <<= 8;
				height |= ((unsigned int)t->in_sub_buf[4] & 0xff);

				if (width <= 1)
					width = TERM_WIDTH;
				if (width > MAX_TERM)
					width = MAX_TERM;

				if (height <= 2)			/* --More-- prompt stops working properly on <= 2 */
					height = TERM_HEIGHT;
				if (height > MAX_TERM)
					height = MAX_TERM;

				t->in_sub = 0;
				t->state = TS_IAC;					/* expect SE */

				t->term_width = width;
				t->term_height = height;

				if (display != NULL)
					resize_Display(display, width, height);
			}
			return -1;

/*
	Environment variables
*/
		case TS_NEW_ENVIRON:
			if (c == 0)								/* IS */
				t->state = TS_NEW_ENVIRON_IS;

			t->in_sub = 1;
			t->in_sub_buf[t->in_sub] = 0;
			return -1;

		case TS_NEW_ENVIRON_IS:
			switch(c) {
				case 0:								/* expect variable */
				case 2:
				case 3:
					t->in_sub = 1;
					t->in_sub_buf[t->in_sub] = 0;
					t->state = TS_NEW_ENVIRON_VAR;
					break;

				case IAC:
					t->state = TS_IAC;				/* expect SE */
					break;

				default:
					t->state = TS_DATA;				/* must be wrong */
			}
			return -1;

		case TS_NEW_ENVIRON_VAR:
			if (c == 1) {
				t->in_sub_buf[t->in_sub++] = 0;
				t->state = TS_NEW_ENVIRON_VAL;		/* expect value */
				return -1;
			}
			if (c == IAC) {
				t->state = TS_IAC;					/* expect SE */
				return -1;
			}
			if (t->in_sub < MAX_SUB_BUF - 2) {
				t->in_sub_buf[t->in_sub++] = c;
				t->in_sub_buf[t->in_sub] = 0;
			}
			return -1;

		case TS_NEW_ENVIRON_VAL:
			if (c <= 3 || c == IAC) {				/* next variable or end of list */
				t->in_sub_buf[t->in_sub] = 0;

/* variable has been processed ; get next one or end on IAC */

				if (c == IAC)
					t->state = TS_IAC;
				else
					t->state = TS_NEW_ENVIRON_VAR;

				if (!strcmp(t->in_sub_buf+1, "USER")) {		/* entered a user name */
					t->in_sub = 1;
					t->in_sub_buf[t->in_sub] = 0;
					return KEY_RETURN;
				}
				t->in_sub = 1;
				t->in_sub_buf[t->in_sub] = 0;
			} else {
/* setting username, let through */
				if (!strcmp(t->in_sub_buf+1, "USER")) {
					return c;
				}
			}
			return -1;

		case TS_ARG:
			t->state = t->in_sub ? TS_IAC : TS_DATA;
			return -1;
	}
	return -1;
}

/* EOB */
