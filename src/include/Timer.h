/*
	Timer.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef TIMER_H_WJ109
#define TIMER_H_WJ109	1

#include "List.h"

#include <pthread.h>

#define TIMER_SLEEP		20

#define append_Timer(x, y)		append_List(&((x)->list), &((y)->list))
#define add_Timer(x, y)			append_Timer((x), (y))
#define prepend_Timer(x, y)		prepend_List(&((x)->list), &((y)->list))
#define unlink_Timer(x, y)		unlink_List_type((x), (y), Timer, list)
#define pop_Timer(x)			pop_List_type((x), Timer, list)
#define pop0_Timer(x)			pop0_List_type((x), Timer, list)

typedef enum {
	TIMER_ONESHOT = 0,
	TIMER_RESTART,
	TIMER_CANCELLED
} TimerType;

typedef struct {
	int id;
	pthread_t tid;

	unsigned int secs;
	TimerType type;
	void (*action)(void *);
	void *arg;

	List list;
} Timer;

void init_timerq(void);
int start_Timer(TimerType, unsigned int, void (*)(void *), void *);
int cancel_Timer(int);

#endif	/* TIMER_H_WJ109 */

/* EOB */
