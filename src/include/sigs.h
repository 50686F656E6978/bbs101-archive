/*
	sigs.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef SIGS_H_WJ109
#define SIGS_H_WJ109	1

void init_signals(void);
void set_signal_handler(int);
void unblock_signal(int);

#endif	/* SIGS_H_WJ109 */

/* EOB */
