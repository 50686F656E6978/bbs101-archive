/*
	keys.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "keys.h"

/*
	input filter that tries to see whether the user used cursor keys
	by analyzing the input buffer and looking for sequences like ESC[5~ and ESC[A
*/
int keyboard_cook(IOBuf *kb, int c) {
	if (c == KEY_ESC) {
		char buf[4];
		int key;

		if (peek_IOBuf(kb, buf, 3) == 3) {
			if (buf[0] != '[')
				return c;

			if (buf[2] != '~')
				return c;

			switch(buf[1]) {
				case '2':
					key = KEY_INSERT;
					break;

				case '3':
					key = KEY_DELETE;
					break;

				case '5':
					key = KEY_PAGEUP;
					break;

				case '6':
					key = KEY_PAGEDOWN;
					break;

				case '7':
					key = KEY_HOME;
					break;

				case '8':
					key = KEY_END;
					break;

				default:
					return c;
			}
			fetch_IOBuf(kb, buf, 3);
			return key;
		}
		if (peek_IOBuf(kb, buf, 2) == 2) {
			if (buf[0] != '[')
				return c;

			switch(buf[1]) {
				case 'A':
					key = KEY_UP;
					break;

				case 'B':
					key = KEY_DOWN;
					break;

				case 'C':
					key = KEY_RIGHT;
					break;

				case 'D':
					key = KEY_LEFT;
					break;

				default:
					return c;
			}
			fetch_IOBuf(kb, buf, 2);
			return key;
		}
	}
	return c;
}

/* EOB */
