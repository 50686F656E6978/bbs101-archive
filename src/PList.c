/*
	PList.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- list of pointers
*/

#include "config.h"
#include "PList.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>


void init_PList(PList *pl) {
	pl->p = NULL;
	init_List(&(pl->list));
}

PList *new_PList(void *v) {
PList *pl;

	if ((pl = (PList *)Malloc(sizeof(PList))) == NULL)
		return NULL;

	pl->p = v;
	return pl;
}

void destroy_PList(PList *pl) {
	pl->p = NULL;
	Free(pl);
}

/* EOB */
