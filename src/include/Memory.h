/*
	Memory.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	This looks kind of useless, but allows for plugging in different
	object allocator code later
*/

#ifndef MEMORY_H_WJ109
#define MEMORY_H_WJ109	1

#include <stdlib.h>

#define Malloc(x)			calloc(1, (x))
#define Realloc(x, y)		realloc((x), (y))
#define Free(x)				free(x)

#endif	/* MEMORY_H_WJ109 */

/* EOB */
