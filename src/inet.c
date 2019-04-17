/*
	inet.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "inet.h"
#include "log.h"
#include "memset.h"
#include "bufprintf.h"
#include "cstring.h"
#include "cstrerror.h"
#include "sys_wait.h"
#include "my_fcntl.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#define MAX_NEWCONNS	5


char *inet_error(int err, char *buf, int maxlen) {
	if (buf == NULL || maxlen < 0)
		return NULL;

	if (err == EAI_SYSTEM)
		return cstrerror(errno, buf, maxlen);

	bufprintf(buf, maxlen, "%s", (char *)gai_strerror(err));
	return buf;
}

/*
	buf should be large enough, MAX_LINE should do
*/
char *inet_printaddr(char *host, char *service, char *buf, int buflen) {
	if (host == NULL || service == NULL || buf == NULL || buflen <= 0)
		return NULL;

	if (cstrchr(host, ':') != NULL)
		bufprintf(buf, buflen, "[%s]:%s", host, service);
	else
		bufprintf(buf, buflen, "%s:%s", host, service);

	return buf;
}

/*
	listen on a service port
	(actually, this function does a lot more than just listen()ing)

	This function is protocol-family independent, so it supports
	both IPv4 and IPv6 (and possibly even more ...)

	Mind that this function may take a long time to complete, as bind()
	may need some time to wait for a port to come available again

	returns -1 on error, listening socket on success
*/
int inet_listen(char *node, char *service) {
int sock, err, optval, retval, retry, success;
struct addrinfo hints, *res, *ai_p;
char host[NI_MAXHOST], serv[NI_MAXSERV], buf[NI_MAXHOST+NI_MAXSERV+MAX_LINE], errbuf[MAX_LINE];

	retval = -1;

#ifndef PF_UNSPEC
#error inet.c: PF_UNSPEC is undefined on this system
#endif
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
/*	hints.ai_protocol = 0;		already 0 */
/*	hints.ai_flags |= AI_PASSIVE; */			/* accept clients on any network */

	if ((err = getaddrinfo(node, service, &hints, &res)) != 0) {
		log_err("inet_listen(%s): %s", service, inet_error(err, errbuf, MAX_LINE));
		return -1;
	}
	for(ai_p = res; ai_p != NULL; ai_p = ai_p->ai_next) {
		if (ai_p->ai_family == PF_LOCAL)		/* skip local sockets */
			continue;

		if ((sock = socket(ai_p->ai_family, ai_p->ai_socktype, ai_p->ai_protocol)) == -1) {
/* be cool about errors about IPv6 ... not many people have it yet */
			if (!(errno == EAFNOSUPPORT && ai_p->ai_family == PF_INET6))
				log_warn("inet_listen(%s): socket(family = %d, socktype = %d, protocol = %d) failed: %s",
					service, ai_p->ai_family, ai_p->ai_socktype, ai_p->ai_protocol, cstrerror(errno, errbuf, MAX_LINE));
			continue;
		}
/*
	This is commented out because you don't really want to restrict
	BBS use to IPv6-only users

#ifdef IPV6_V6ONLY
		optval = 1;
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(int))) == -1)
			log_warn("inet_listen(%s): failed to set IPV6_V6ONLY: %s", service, cstrerror(errno, errbuf, MAX_LINE));
#endif
*/
/*
	SO_REUSEADDR allows us to do quick restarts of the BBS
	I have sometimes seen problems with this (the port going into TIME_WAIT
	and new connections fail, without the socket reporting any error at all)
*/
		optval = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
			log_warn("inet_listen(%s): setsockopt(SO_REUSEADDR) failed: %s", service, cstrerror(errno, errbuf, MAX_LINE));

		optval = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) == -1)
			log_warn("inet_listen(%s): setsockopt(SO_KEEPALIVE) failed: %s", service, cstrerror(errno, errbuf, MAX_LINE));

		optval = 0;
		if (setsockopt(sock, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(int)) == -1)
			log_warn("inet_listen(%s): setsockopt(SO_OOBINLINE) failed: %s", service, cstrerror(errno, errbuf, MAX_LINE));

		if (bind(sock, (struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen) == -1) {
			err = errno;
			success = 0;
			if (getnameinfo((struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen,
				host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0) {
/*
	retry the bind a number of times if the port is in TIME_WAIT state
*/
				if (err == EADDRINUSE) {
					for(retry = 0; retry < BIND_RETRIES; retry++) {
						log_warn("inet_listen(): waiting on bind() on %s", inet_printaddr(host, serv, buf, sizeof(buf)));
						sleep(BIND_WAIT);
						if (bind(sock, (struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen) == -1) {
							err = errno;
							if (err == EADDRINUSE)
								continue;
							else
								break;				/* some other error */
						} else {
							success = 1;
							break;
						}
					}
				}
				if (!success)
					log_warn("inet_listen(): bind() failed on %s: %s", inet_printaddr(host, serv, buf, sizeof(buf)), cstrerror(err, errbuf, sizeof(errbuf)));
			} else
				log_warn("inet_listen(%s): bind failed on an interface, but I don't know which one(!)", service);

			if (!success) {
				close(sock);
				continue;
			}
		}
/*
	I want a blocking listen(), so don't do this

		if (ioctl(sock, FIONBIO, &optval) == -1) {
			log_err("inet_listen(%s): failed to set socket non-blocking: %s", service, cstrerror(errno, errbuf, MAX_LINE));
			close(sock);
			continue;
		}
*/
		if (listen(sock, MAX_NEWCONNS) == -1) {
			if (getnameinfo((struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen,
				host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0)
				log_err("inet_listen(): listen() failed on %s", inet_printaddr(host, serv, buf, sizeof(buf)));
			else
				log_err("inet_listen(%s): listen() failed", service);
			close(sock);
			continue;
		}

/* success, we have a listening socket */

		if (getnameinfo((struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen,
			host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0)
			log_msg("listening on %s", inet_printaddr(host, serv, buf, sizeof(buf)));
		else
			log_msg("listening on port %s", service);

		retval = 0;			/* success */
		break;
	}
	freeaddrinfo(res);
/*
	it's actually possible to get here without a single error message being logged yet
	(which happens if you specify an IPv6 address, but do not have it)
	so just print a message if all is not well
*/
	if (retval) {
		log_err("failed to start network");
		return -1;
	}
	return sock;
}

/*
	accept a new connection

	returns -1 on error, or socket of new connection and ipaddr_buf set to IP address
*/
int inet_accept(int listen_sock, char *ipaddr_buf) {
int s, err, optval;
char errbuf[MAX_LINE];
struct sockaddr_storage client;
socklen_t client_len = sizeof(struct sockaddr_storage);

	if ((s = accept(listen_sock, (struct sockaddr *)&client, &client_len)) < 0) {
		err = errno;

		if (err != EINTR)
			log_err("inet_accept(): failed to accept(): %s", cstrerror(err, errbuf, MAX_LINE));

		if (err == ENOTSOCK || err == EOPNOTSUPP || err == EBADF) {
			log_err("This is a serious error, aborting");
			abort();
		}
		return -1;
	}
	optval = 1;
	if (ioctl(s, FIONBIO, &optval) == -1)		/* set non-blocking */
		log_warn("inet_accept(): failed to set socket nonblocking");

	optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) == -1)
		log_warn("inet_accept(): setsockopt(SO_KEEPALIVE) failed: %s", cstrerror(errno, errbuf, MAX_LINE));

	optval = 0;
	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(int)) == -1)
		log_warn("inet_accept(): setsockopt(SO_OOBINLINE) failed: %s", cstrerror(errno, errbuf, MAX_LINE));

/* put originating IP address in ipaddr_buf */
	if (ipaddr_buf != NULL) {
		if ((err = getnameinfo((struct sockaddr *)&client, client_len, ipaddr_buf, MAX_LONGLINE-1, NULL, 0, NI_NUMERICHOST)) != 0) {
			log_warn("inet_accept(): getnameinfo(): %s", inet_error(err, errbuf, MAX_LINE));
			cstrcpy(ipaddr_buf, "0.0.0.0", 8);
		}
	}
	return s;
}

/* EOB */
