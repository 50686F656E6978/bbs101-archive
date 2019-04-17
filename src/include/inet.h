/*
	inet.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef INET_H_WJ109
#define INET_H_WJ109	1

/*
	bind() may retry for as long as 30*5 seconds == 2,5 minutes
*/
#define BIND_RETRIES	30
#define BIND_WAIT		5

char *inet_error(int, char *, int);
char *inet_printaddr(char *, char *, char *, int);
int inet_listen(char *, char *);
int inet_accept(int, char *);

#endif	/* INET_H_WJ109 */

/* EOB */
