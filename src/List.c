/*
	List.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- circular lists
	- the first element (item #0) acts as list head (and thus, this space is wasted)
*/

#include "List.h"

#include <stdio.h>
#include <stdlib.h>


void init_List(List *l) {
	l->prev = l->next = l;
}

void append_List(List *root, List *l) {
	l->prev = root->prev;
	l->next = root;

	root->prev->next = l;
	root->prev = l;
}

void prepend_List(List *root, List *l) {
	l->prev = root;
	l->next = root->next;

	root->next->prev = l;
	root->next = l;
}

void unlink_List(List *root, List *l) {
	if (l == root)
		return;

	l->prev->next = l->next;
	l->next->prev = l->prev;
	l->prev = l->next = NULL;
}

List *pop_List(List *root) {
List *p;

	if (root->prev == root)
		return NULL;

	p = root->prev;
	unlink_List(root, p);
	return p;
}

List *pop0_List(List *root) {
List *p;

	if (root->prev == root)
		return NULL;

	p = root->next;
	unlink_List(root, p);
	return p;
}

/* EOB */
