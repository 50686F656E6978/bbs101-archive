/*
	Stats.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef STATS_H_WJ109
#define STATS_H_WJ109	1

#include "GString.h"
#include "User.h"

#include <time.h>

typedef enum {
	STATS_LOGINS = 1,
	STATS_XSENT,
	STATS_XRECV,
	STATS_ESENT,
	STATS_ERECV,
	STATS_FSENT,
	STATS_FRECV,
	STATS_QSENT,
	STATS_QANSW,
	STATS_POSTED,
	STATS_READ
} IncStat;

typedef struct {
	GString dirty;

	char oldest[MAX_NAME], youngest[MAX_NAME], most_logins[MAX_NAME];
	char most_xsent[MAX_NAME], most_xrecv[MAX_NAME];
	char most_esent[MAX_NAME], most_erecv[MAX_NAME];
	char most_fsent[MAX_NAME], most_frecv[MAX_NAME];
	char most_qsent[MAX_NAME], most_qansw[MAX_NAME];
	char most_posted[MAX_NAME], most_read[MAX_NAME];

	time_t oldest_birth, youngest_birth;
	unsigned int oldest_age;
	unsigned int logins, xsent, xrecv, esent, erecv, fsent, frecv, qsent, qansw;
	unsigned int msgs_posted, msgs_read;

	unsigned int logins_boot;
	unsigned int xsent_boot, xrecv_boot, esent_boot, erecv_boot, fsent_boot, frecv_boot;
	unsigned int qsent_boot, qansw_boot, posted_boot, read_boot;
	time_t boot_time;
} Stats;

void init_stats(void);
void update_stats(User *);
void inc_stats(IncStat);
int load_stats(void);
int save_stats(void);
void print_stats(User *);

#endif	/* STATS_H_WJ109 */

/* EOB */
