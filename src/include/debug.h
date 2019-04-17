/*
	debug.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef DEBUG_H_WJ109
#define DEBUG_H_WJ109	1

#ifdef EBUG
#define DEBUG	1
#endif

#ifdef DEBUG

void debug_breakpoint(void);

#endif	/* DEBUG */

#endif	/* DEBUG_H_WJ109 */

/* EOB */
