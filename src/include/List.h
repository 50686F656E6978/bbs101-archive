/*
	List.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef LIST_H_WJ109
#define LIST_H_WJ109	1

#define list_item(ptr, type, member)		\
	((type *)((char *)(ptr) - (unsigned long)(&((type *)0UL)->member)))

#define first_item(root, type, member)		list_item((&(root))->next, type, member)

#define unlink_List_type(root, ptr, type, member)	\
	unlink_List(&((root)->member), &((ptr)->member))

#define pop_List_type(ptr, type, member)	\
	(&((ptr)->member) == ((&((ptr)->member))->prev)) ? NULL : \
	((type *)((char *)(pop_List(&((ptr)->member))) - (unsigned long)(&((type *)0UL)->member)))

#define pop0_List_type(ptr, type, member)	\
	(&((ptr)->member) == ((&((ptr)->member))->prev)) ? NULL : \
	((type *)((char *)(pop0_List(&((ptr)->member))) - (unsigned long)(&((type *)0UL)->member)))

#define foreach_list(x, root)				\
	for((x) = (&(root))->next; (x) != &(root); (x) = (x)->next)

#define foreach_list_safe(x, root, y)		\
	for((x) = (&(root))->next, (y) = (x)->next; (x) != &(root); (x) = (y), (y) = (y)->next)

#define empty_list(x)	((x)->prev == (x))

/*
	Note: does NOT destroy the root node
*/
#define listdestroy_List(root, p, type)			\
	while(((p) = pop_##type((root))) != NULL)	\
		destroy_##type(p)

typedef struct List_tag List;

struct List_tag {
	List *prev, *next;
};

void init_List(List *);
void append_List(List *, List *);
void prepend_List(List *, List *);
void unlink_List(List *, List *);
List *pop_List(List *);
List *pop0_List(List *);

#endif	/* LIST_H_WJ109 */

/* EOB */
