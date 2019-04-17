/*
	keys.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef KEYS_H_WJ109
#define KEYS_H_WJ109	1

#include "IOBuf.h"

#define KEY_CTRL(x)		((x) - 'A' + 1)

#define KEY_ESC			0x1b
#define KEY_RETURN		'\r'
#define KEY_BS			'\b'
#define KEY_BEEP		7
#define KEY_TAB			'\t'
#define KEY_BACKTAB		'\\'

/*
	just pick a high number to start at
*/
#define KEY_UP			0x8000
#define KEY_DOWN		0x8001
#define KEY_RIGHT		0x8002
#define KEY_LEFT		0x8003
#define KEY_PAGEUP		0x8004
#define KEY_PAGEDOWN	0x8005
#define KEY_HOME		0x8006
#define KEY_END			0x8007
#define KEY_INSERT		0x8008
#define KEY_DELETE		0x8009


int keyboard_cook(IOBuf *, int);

#endif	/* KEYS_H_WJ109 */

/* EOB */
