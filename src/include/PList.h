/*
	PList.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef PLIST_H_WJ109
#define PLIST_H_WJ109	1

#include "List.h"

#define append_PList(x, y)		append_List(&((x)->list), &((y)->list))
#define add_PList(x, y)			append_PList((x), (y))
#define prepend_PList(x, y)		prepend_List(&((x)->list), &((y)->list))
#define unlink_PList(x, y)		unlink_List_type((x), (y), PList, list)
#define pop_PList(x)			pop_List_type((x), PList, list)
#define pop0_PList(x)			pop0_List_type((x), PList, list)

typedef struct {
	void *p;
	List list;
} PList;

void init_PList(PList *);
PList *new_PList(void *);
void destroy_PList(PList *);

#endif	/* PLIST_H_WJ109 */

/* EOB */
