/*
	Stats.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "Stats.h"
#include "User.h"
#include "defines.h"
#include "cstring.h"
#include "log.h"
#include "backend.h"
#include "version.h"
#include "util.h"
#include "pager.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static pthread_mutex_t stats_mutex;
static Stats stats;


void init_stats(void) {
	pthread_mutex_init(&stats_mutex, NULL);

	memset(&stats, 0, sizeof(Stats));
	init_GString(&(stats.dirty));

	stats.boot_time = time(NULL);
}

/*
	add field to the dirty list
	this is the list of fields that need to be synced
*/
static int stats_dirty(char *field) {
	if (stats.dirty.len <= 0)
		return gstrcpy(&(stats.dirty), field);

	if (cstrfind(stats.dirty.str, field, ','))		/* already in it */
		return 0;

	gstrcat(&(stats.dirty), ",");
	gstrcat(&(stats.dirty), field);
	return 0;
}

void update_stats(User *usr) {
time_t now;

	now = time(NULL);

	if (!usr->online_timer)
		usr->online_timer = now;
	if (usr->online_timer < now)
		usr->total_time += (unsigned int)difftime(now, usr->online_timer);
	usr->online_timer = now;

	pthread_mutex_lock(&stats_mutex);

	if (usr->total_time >= stats.oldest_age) {
		stats.oldest_age = usr->total_time;
		stats.oldest_birth = usr->birth;			/* oldest birth is not really used for anything */
		cstrcpy(stats.oldest, usr->name, MAX_NAME);
		stats_dirty("oldest_age");
		stats_dirty("oldest_birth");
		stats_dirty("oldest");
	}
	if (usr->logins >= stats.logins) {
		stats.logins = usr->logins;
		cstrcpy(stats.most_logins, usr->name, MAX_NAME);
		stats_dirty("logins");
		stats_dirty("most_logins");
	}
	if (usr->xsent >= stats.xsent) {
		stats.xsent = usr->xsent;
		cstrcpy(stats.most_xsent, usr->name, MAX_NAME);
		stats_dirty("xsent");
		stats_dirty("most_xsent");
	}
	if (usr->xrecv >= stats.xrecv) {
		stats.xrecv = usr->xrecv;
		cstrcpy(stats.most_xrecv, usr->name, MAX_NAME);
		stats_dirty("xrecv");
		stats_dirty("most_xrecv");
	}
	if (usr->esent >= stats.esent) {
		stats.esent = usr->esent;
		cstrcpy(stats.most_esent, usr->name, MAX_NAME);
		stats_dirty("esent");
		stats_dirty("most_esent");
	}
	if (usr->erecv >= stats.erecv) {
		stats.erecv = usr->erecv;
		cstrcpy(stats.most_erecv, usr->name, MAX_NAME);
		stats_dirty("erecv");
		stats_dirty("most_erecv");
	}
	if (usr->fsent >= stats.fsent) {
		stats.fsent = usr->fsent;
		cstrcpy(stats.most_fsent, usr->name, MAX_NAME);
		stats_dirty("fsent");
		stats_dirty("most_fsent");
	}
	if (usr->frecv >= stats.frecv) {
		stats.frecv = usr->frecv;
		cstrcpy(stats.most_frecv, usr->name, MAX_NAME);
		stats_dirty("frecv");
		stats_dirty("most_frecv");
	}
	if (usr->qsent >= stats.qsent) {
		stats.qsent = usr->qsent;
		cstrcpy(stats.most_qsent, usr->name, MAX_NAME);
		stats_dirty("qsent");
		stats_dirty("most_qsent");
	}
	if (usr->qansw >= stats.qansw) {
		stats.qansw = usr->qansw;
		cstrcpy(stats.most_qansw, usr->name, MAX_NAME);
		stats_dirty("qansw");
		stats_dirty("most_qansw");
	}
	if (usr->msgs_posted >= stats.msgs_posted) {
		stats.msgs_posted = usr->msgs_posted;
		cstrcpy(stats.most_posted, usr->name, MAX_NAME);
		stats_dirty("msgs_posted");
		stats_dirty("most_posted");
	}
	if (usr->msgs_read >= stats.msgs_read) {
		stats.msgs_read = usr->msgs_read;
		cstrcpy(stats.most_read, usr->name, MAX_NAME);
		stats_dirty("msgs_read");
		stats_dirty("most_read");
	}
	pthread_mutex_unlock(&stats_mutex);
}

void inc_stats(IncStat type) {
	pthread_mutex_lock(&stats_mutex);

	switch(type) {
		case STATS_LOGINS:
			stats.logins_boot++;
			break;

		case STATS_XSENT:
			stats.xsent_boot++;
			break;

		case STATS_XRECV:
			stats.xrecv_boot++;
			break;

		case STATS_ESENT:
			stats.esent_boot++;
			break;

		case STATS_ERECV:
			stats.erecv_boot++;
			break;

		case STATS_FSENT:
			stats.fsent_boot++;
			break;

		case STATS_FRECV:
			stats.frecv_boot++;
			break;

		case STATS_QSENT:
			stats.qsent_boot++;
			break;

		case STATS_QANSW:
			stats.qansw_boot++;
			break;

		case STATS_POSTED:
			stats.posted_boot++;
			break;

		case STATS_READ:
			stats.read_boot++;
			break;

		default:
			fprintf(stderr, "inc_stats(): unknown type %d\n", type);
			abort();
	}
	pthread_mutex_unlock(&stats_mutex);
}

int load_stats(void) {
	pthread_mutex_lock(&stats_mutex);

	if (backend_load_stats(&stats)) {
		pthread_mutex_unlock(&stats_mutex);
		log_err("load_stats(): failed");
		return 0;
	}
	pthread_mutex_unlock(&stats_mutex);
	return 0;
}

int save_stats(void) {
	pthread_mutex_lock(&stats_mutex);

	if (backend_save_stats(&stats)) {
		pthread_mutex_unlock(&stats_mutex);
		log_err("save_stats(): failed");
		return 0;
	}
	pthread_mutex_unlock(&stats_mutex);
	return 0;
}

void print_stats(User *usr) {
char buf[MAX_LINE];
time_t now;
struct tm *tm;
int w, l;
unsigned long num;

	update_stats(usr);

	now = time(NULL);

	init_pager(usr);

	Put(usr, "<yellow>This is <white>bbs101<yellow> version <white>" BBS_VERSION "\n");

	pthread_mutex_lock(&stats_mutex);

	tm = localtime((time_t *)&(stats.boot_time));
	Print(usr, "<green>The system was last booted on <cyan>%s\n", sprint_date(tm, (usr->flags & USR_12HRCLOCK), buf, MAX_LINE));
	Print(usr, "<green>Uptime is <yellow>%s\n", sprint_total_time((unsigned long)difftime(now, stats.boot_time), buf, MAX_LINE));
	Print(usr, "<yellow>%s<green> successful login%s made since boot time\n",
		sprint_number(stats.logins_boot, ',', buf, sizeof(buf)), (stats.logins_boot == 1) ? "" : "s");

	Print(usr, "\n<yellow>User statistics\n");

	if (stats.youngest[0]) {
		tm = localtime((time_t *)&(stats.youngest_birth));
		Print(usr, "<green>Youngest user is <white>%s<green>, created on <cyan>%s\n", stats.youngest, sprint_date(tm, usr->flags & USR_12HRCLOCK, buf, MAX_LINE));
	}
	if (stats.oldest[0])
		Print(usr, "<green>Oldest user is <white>%s<green>, online for <yellow>%s\n", stats.oldest, sprint_total_time(stats.oldest_age, buf, MAX_LINE));

/*
	determine width of next block of text
	I like to pretty-print this screen...
*/
	w = strlen(stats.most_logins);
	if ((l = strlen(stats.most_xsent)) > w)
		w = l;
	if ((l = strlen(stats.most_xrecv)) > w)
		w = l;
	if ((l = strlen(stats.most_esent)) > w)
		w = l;
	if ((l = strlen(stats.most_erecv)) > w)
		w = l;
	if ((l = strlen(stats.most_fsent)) > w)
		w = l;
	if ((l = strlen(stats.most_frecv)) > w)
		w = l;
	if ((l = strlen(stats.most_qsent)) > w)
		w = l;
	if ((l = strlen(stats.most_qansw)) > w)
		w = l;
	if ((l = strlen(stats.most_posted)) > w)
		w = l;
	if ((l = strlen(stats.most_read)) > w)
		w = l;

	Print(usr, "\n<green>Most logins are by                     <white>%-*s<green> : <yellow>%s\n", w, stats.most_logins, sprint_number(stats.logins, ',', buf, sizeof(buf)));

	if (stats.xsent > 0 && stats.most_xsent[0])
		Print(usr, "<green>Most eXpress Messages were sent by     <white>%-*s<green> : <yellow>%s\n", w, stats.most_xsent, sprint_number(stats.xsent, ',', buf, sizeof(buf)));

	if (stats.xrecv > 0 && stats.most_xrecv[0])
		Print(usr, "<green>Most eXpress Messages were received by <white>%-*s<green> : <yellow>%s\n", w, stats.most_xrecv, sprint_number(stats.xrecv, ',', buf, sizeof(buf)));

	if (stats.esent > 0 && stats.most_esent[0])
		Print(usr, "<green>Most emotes were sent by               <white>%-*s<green> : <yellow>%s\n", w, stats.most_esent, sprint_number(stats.esent, ',', buf, sizeof(buf)));

	if (stats.erecv > 0 && stats.most_erecv[0])
		Print(usr, "<green>Most emotes were received by           <white>%-*s<green> : <yellow>%s\n", w, stats.most_erecv, sprint_number(stats.erecv, ',', buf, sizeof(buf)));
/*
	if (stats.fsent > 0 && stats.most_fsent[0])
		Print(usr, "<green>Most Feelings were sent by             <white>%-*s<green> : <yellow>%s\n", w, stats.most_fsent, sprint_number(stats.fsent, ',', buf, sizeof(buf)));

	if (stats.frecv > 0 && stats.most_frecv[0])
		Print(usr, "<green>Most Feelings were received by         <white>%-*s<green> : <yellow>%s\n", w, stats.most_frecv, sprint_number(stats.frecv, ',', buf, sizeof(buf)));

	if (stats.qsent > 0 && stats.most_qsent[0])
		Print(usr, "<green>Most Questions were asked by           <white>%-*s<green> : <yellow>%s\n", w, stats.most_qsent, sprint_number(stats.qsent, ',', buf, sizeof(buf)));

	if (stats.qansw > 0 && stats.most_qansw[0])
		Print(usr, "<green>Most Answers were given by             <white>%-*s<green> : <yellow>%s\n", w, stats.most_qansw, sprint_number(stats.qansw, ',', buf, sizeof(buf)));

	if (stats.msgs_posted > 0 && stats.most_posted[0])
		Print(usr, "<green>Most messages were posted by           <white>%-*s<green> : <yellow>%s\n", w, stats.most_posted, sprint_number(stats.msgs_posted, ',', buf, sizeof(buf)));

	if (stats.msgs_read > 0 && stats.most_read[0])
		Print(usr, "<green>Most messages were read by             <white>%-*s<green> : <yellow>%s\n", w, stats.most_read, sprint_number(stats.msgs_read, ',', buf, sizeof(buf)));
*/
	if (usr->runtime_flags & RTF_SYSOP) {
		Put(usr, "\n<yellow>Since boot time\n");

		Print(usr, "<green>eXpress Messages sent: <yellow>%-15s", sprint_number(stats.xsent_boot, ',', buf, sizeof(buf)));
		Print(usr, "<green> received: <yellow>%s\n", sprint_number(stats.xrecv_boot, ',', buf, sizeof(buf)));

		Print(usr, "<green>Emotes sent          : <yellow>%-15s", sprint_number(stats.esent_boot, ',', buf, sizeof(buf)));
		Print(usr, "<green> received: <yellow>%s\n", sprint_number(stats.erecv_boot, ',', buf, sizeof(buf)));
/*
		Print(usr, "<green>Feelings sent        : <yellow>%-15s", sprint_number(stats.fsent_boot, ',', buf, sizeof(buf)));
		Print(usr, "<green> received: <yellow>%s\n", sprint_number(stats.frecv_boot, ',', buf, sizeof(buf)));

		Print(usr, "<green>Questions asked      : <yellow>%-15s", sprint_number(stats.qsent_boot, ',', buf, sizeof(buf)));
		Print(usr, "<green> answered: <yellow>%s\n", sprint_number(stats.qansw_boot, ',', buf, sizeof(buf)));
		Print(usr, "<green>Messages posted      : <yellow>%-15s", sprint_number(stats.posted_boot, ',', buf, sizeof(buf)));
		Print(usr, "<green> read    : <yellow>%s\n", sprint_number(stats.read_boot, ',', buf, sizeof(buf)));
*/
	}
	Put(usr, "\n<yellow>Your statistics\n");

	Print(usr, "<green>eXpress Messages sent: <yellow>%-15s", sprint_number(usr->xsent, ',', buf, sizeof(buf)));
	Print(usr, "<green> received: <yellow>%s\n", sprint_number(usr->xrecv, ',', buf, sizeof(buf)));
	Print(usr, "<green>Emotes sent          : <yellow>%-15s", sprint_number(usr->esent, ',', buf, sizeof(buf)));
	Print(usr, "<green> received: <yellow>%s\n", sprint_number(usr->erecv, ',', buf, sizeof(buf)));
/*
	Print(usr, "<green>Feelings sent        : <yellow>%-15s", sprint_number(usr->fsent, ',', buf, sizeof(buf)));
	Print(usr, "<green> received: <yellow>%s\n", sprint_number(usr->frecv, ',', buf, sizeof(buf)));
	Print(usr, "<green>Questions asked      : <yellow>%-15s", sprint_number(usr->qsent, ',', buf, sizeof(buf)));
	Print(usr, "<green> answered: <yellow>%s\n", sprint_number(usr->qansw, ',', buf, sizeof(buf)));
	Print(usr, "<green>Messages posted      : <yellow>%-15s", sprint_number(usr->msgs_posted, ',', buf, sizeof(buf)));
	Print(usr, "<green> read    : <yellow>%s\n", sprint_number(usr->msgs_read, ',', buf, sizeof(buf)));
*/
	tm = localtime((time_t *)&(usr->birth));
	Print(usr, "\n<green>Account created on <cyan>%s<green>\n", sprint_date(tm, usr->flags & USR_12HRCLOCK, buf, sizeof(buf)));
	Print(usr, "You have logged on <yellow>%s<green> times, ", sprint_number(usr->logins, ',', buf, sizeof(buf)));

	num = (unsigned long)(difftime(now, (time_t)(usr->birth)) / (unsigned long)(30 * SECS_IN_DAY));
	if (num == 0UL)
		num = 1UL;
	num = usr->logins / num;

	Print(usr, "an average of <yellow>%s<green> time%s per month\n", sprint_number(num, ',', buf, sizeof(buf)), (num == 1UL) ? "" : "s");
	Print(usr, "Your total online time is <yellow>%s\n", sprint_total_time(usr->total_time, buf, sizeof(buf)));

	pthread_mutex_unlock(&stats_mutex);

	pager(usr);
}

/* EOB */
