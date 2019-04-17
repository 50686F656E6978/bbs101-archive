/*
	log.c	WJ103

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- more 'syslog'-like to the outside world
	- automatic rotation
	- automatic archiving
*/

#include "config.h"
#include "log.h"
#include "cstring.h"
#include "Memory.h"
#include "sys_time.h"
#include "my_fcntl.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>

char *Months[12] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December"
};

static pthread_mutex_t log_mutex;


int init_log(void) {
	pthread_mutex_init(&log_mutex, NULL);
	return 0;
}

void log_entry(FILE *f, char *msg, char level, va_list ap) {
time_t t;
struct tm *tm;
char buf[1024];
int l;

	t = time(NULL);
	tm = localtime(&t);		/* logging goes in localtime */

	l = bufprintf(buf, sizeof(buf), "%c%c%c %2d %02d:%02d:%02d %c ", Months[tm->tm_mon][0], Months[tm->tm_mon][1], Months[tm->tm_mon][2],
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, level);
	bufvprintf(buf+l, sizeof(buf) - l, msg, ap);

	pthread_mutex_lock(&log_mutex);

	fprintf(f, "%s\n", buf);

	pthread_mutex_unlock(&log_mutex);
}

void log_msg(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, ' ', ap);
	va_end(ap);
}

void log_info(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'I', ap);
	va_end(ap);
}

void log_err(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'E', ap);
	va_end(ap);
}

void log_warn(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'W', ap);
	va_end(ap);
}

void log_debug(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'D', ap);
	va_end(ap);
}

void log_auth(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stderr, msg, 'A', ap);
	va_end(ap);
}

/* EOB */
