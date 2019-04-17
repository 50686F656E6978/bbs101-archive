/*
	Timer.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- all timers are threads that sleep until t = t_action
	- all timers are registered on the timerq list, so that
	  they may be found and cancelled by id
*/

#include "config.h"
#include "Timer.h"
#include "util.h"
#include "cstring.h"
#include "Memory.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static pthread_mutex_t timer_mutex;
static pthread_once_t init_timer_once = PTHREAD_ONCE_INIT;
static Timer timerq;				/* list of all timers currently in the system */
static int eternal_timer_id;


static Timer *new_Timer(void) {
	return (Timer *)Malloc(sizeof(Timer));
}

static void destroy_Timer(Timer *t) {
	Free(t);
}

static void do_init_timerq(void) {
	pthread_mutex_init(&timer_mutex, NULL);

	memset(&timerq, 0, sizeof(Timer));
	init_List(&(timerq.list));

	eternal_timer_id = 1;
}

void init_timerq(void) {
	pthread_once(&init_timer_once, do_init_timerq);
}

/*
	all timers are separate threads
	so in theory, there can be many sleeping threads in the system
	I don't care about this, as they are all sleeping most of the time
*/
static void *timer_thread(void *arg) {
Timer *t;
unsigned int time_left, sleep_time;

	t = (Timer *)arg;
	sleep_time = t->secs;

	for(;;) {
		time_left = sleep(sleep_time);			/* sleep until this timer should run */

		if (t->type == TIMER_CANCELLED)			/* this timer was cancelled by cancel_Timer() */
			break;

		if (time_left > 0) {					/* wrong interruption somehow? continue sleep */
			sleep_time = time_left;
			continue;
		}
		t->action(t->arg);						/* run the action */

		if (t->type == TIMER_ONESHOT)			/* all done */
			break;

		if (t->type != TIMER_RESTART) {
			log_err("timer_thread(): unknown timer type %d, aborting", t->type);
			abort();
		}
		sleep_time = t->secs;					/* restart the timer */
	}
/*
	remove the timer
*/
	pthread_mutex_lock(&timer_mutex);
	unlink_Timer(&timerq, t);
	pthread_mutex_unlock(&timer_mutex);
	destroy_Timer(t);

	pthread_exit(NULL);
}

int start_Timer(TimerType type, unsigned int secs, void (*action)(void *), void *arg) {
int id;
Timer *t;

	if (type != TIMER_ONESHOT && type != TIMER_RESTART)
		return -1;

	if (action == NULL)
		return -1;

	if ((t = new_Timer()) == NULL)
		return -1;

	t->secs = secs;
	t->type = type;
	t->action = action;
	t->arg = arg;

	pthread_mutex_lock(&timer_mutex);

	id = t->id = eternal_timer_id++;		/* hope this never wraps ... */

	if (pthread_create(&(t->tid), NULL, timer_thread, t) != 0) {
		log_err("start_Timer(): error creating thread");
		destroy_Timer(t);
		pthread_mutex_unlock(&timer_mutex);
		return -1;
	}
	append_Timer(&timerq, t);

	pthread_mutex_unlock(&timer_mutex);
	return id;
}

int cancel_Timer(int id) {
Timer *t;
List *l;
int found;

	found = 0;

	pthread_mutex_lock(&timer_mutex);

	foreach_list(l, timerq.list) {
		t = list_item(l, Timer, list);

		if (t->id == id) {
			found = 1;
/*
	do not remove the timer object just now; only mark it as cancelled
	and interrupt the thread's sleep
*/
			t->type = TIMER_CANCELLED;
			pthread_kill(t->tid, SIGUSR1);		/* wakeup */
			break;
		}
	}
	pthread_mutex_unlock(&timer_mutex);

	return (found == 0) ? -1 : 0;
}

/* EOB */
