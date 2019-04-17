/*
	defines.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef DEFINES_H_WJ109
#define DEFINES_H_WJ109		1

#define MAX_NAME			19			/* 18 chars+1 */
#define MAX_LINE			80
#define MAX_LONGLINE		256
#define MAX_HALFLINE		40
#define MAX_COLORBUF		20
#define MAX_CRYPTED			255
#define MAX_NUMBER			32
#define MAX_X_LINES			5
#define MAX_PROFILE_LINES	256

#define MAX_PATHLEN			1024		/* MAXPATH variable */
#define PRINT_BUF			512

#define LOGIN_TIMEOUT		20
#define USER_TIMEOUT		60
#define USER_TIMEOUT2		10
#define USER_TIMEOUT3		10
#define LOCKED_TIMEOUT		3600

#define ONE_SECOND			1
#define SECS_IN_MIN			(60 * ONE_SECOND)
#define SECS_IN_HOUR		(60 * SECS_IN_MIN)
#define SECS_IN_DAY			(24 * SECS_IN_HOUR)
#define SECS_IN_WEEK		(7 * SECS_IN_DAY)

#define MAX_FRIENDS			25
#define PASSWD_MIN_LEN		5

#endif	/* DEFINES_H_WJ109 */

/* EOB */
