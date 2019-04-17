/*
	pager.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef PAGER_H_WJ109
#define PAGER_H_WJ109	1

#include "User.h"

/*
	this is the size with which pager GString grows
	because the standard GString.grow is too small
*/
#define PAGER_GROW		1024

void init_pager(User *);
void pager(User *);
void page_text(User *, char *);

#endif	/* PAGER_H_WJ109 */

/* EOB */
