/*
	sigs.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "sigs.h"
#include "util.h"
#include "log.h"
#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

static int catch_signals[] = {
	SIGHUP, SIGINT, SIGQUIT, SIGUSR1, SIGUSR2, SIGPIPE, SIGALRM,
	SIGTERM, SIGCHLD, SIGURG, SIGXCPU, SIGVTALRM, SIGPROF,
	SIGWINCH, SIGIO, SIGPWR, SIGSYS,
};

static pthread_once_t init_signals_once = PTHREAD_ONCE_INIT;


/*
	this is only a placeholder so that the default signal handler is not being called
*/
void dummy_signal_handler(int sig) {
	;
}

/*
	install dummy signal handler for signal
	the signals are caught for real by the sigwait() in the signal_thread()
*/
void set_signal_handler(int sig) {
struct sigaction sa;

	sa.sa_handler = dummy_signal_handler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(sig, &sa, NULL);
}

/*
	this is the thread that catches all UNIX signals
*/
void *signals_thread(void *arg) {
sigset_t set;
int sig, i, num_sigs;

/* install dummy signal handlers */

	num_sigs = sizeof(catch_signals)/sizeof(int);
	for(i = 0; i < num_sigs; i++)
		set_signal_handler(catch_signals[i]);

	for(;;) {
		sigfillset(&set);
		sigwait(&set, &sig);	/* catch all signals */

		switch(sig) {
			case SIGINT:
			case SIGTERM:
				log_info("signals_thread(): exiting");
				db_close();
				exit(0);

			case SIGWINCH:		/* resize terminal window (ignore) */
				break;

			default:
				log_debug("signals_thread(): signal %d caught", sig);
		}
	}
	return NULL;
}

static void _init_signals(void) {
sigset_t set;
int i, num_sigs;

/* using the default signal mask, start the signal processing thread */
	spawn(signals_thread, NULL);

/* block signals */

	sigemptyset(&set);

	num_sigs = sizeof(catch_signals)/sizeof(int);
	for(i = 0; i < num_sigs; i++) {
/*
	SIGUSR1 is used for interrupting threads, so
	I want to catch it, but not block it
*/
		if (catch_signals[i] != SIGUSR1)
			sigaddset(&set, catch_signals[i]);
	}
	sigprocmask(SIG_BLOCK, &set, NULL);
}

void init_signals(void) {
	pthread_once(&init_signals_once, _init_signals);
}

void unblock_signal(int sig) {
sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, sig);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}

/* EOB */
