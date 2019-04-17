/*
	IOBuf.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "IOBuf.h"
#include "Memory.h"
#include "memset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <assert.h>


void init_IOBuf(IOBuf *iob) {
	memset(iob, 0, sizeof(IOBuf));
	iob->fd = -1;
}

IOBuf *new_IOBuf(int fd) {
IOBuf *iob;

	if ((iob = (IOBuf *)Malloc(sizeof(IOBuf))) == NULL)
		return NULL;

	iob->fd = fd;
	return iob;
}

void destroy_IOBuf(IOBuf *iob) {
	iob->redirect = NULL;
	Free(iob);
}

void connect_IOBuf(IOBuf *iob, int fd) {
	init_IOBuf(iob);
	iob->fd = fd;
}

/*
	see if input data is ready or not
*/
int input_ready_IOBuf(IOBuf *ib) {
int n;

	assert(ib->in_idx <= 0);

	for(;;) {
		if ((n = read(ib->fd, ib->inbuf, INPUTBUF_SIZE)) <= 0) {
			if (n == -1) {
				if (errno == EINTR)
					continue;

				if (errno == EAGAIN || errno == EWOULDBLOCK)
					return 0;

				return -1;			/* some error, probably EBADF */
			}
			if (!n)					/* end of file */
				return -1;
		}
		break;
	}
	ib->in_idx = n;
	return ib->in_idx;
}

/*
	wait for file descriptor to become ready
*/
int wait_fd_read(int fd, struct timeval *tv) {
fd_set fds;
int err;

	for(;;) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		err = select(fd+1, &fds, NULL, NULL, tv);
		if (err < 0) {
/*
	SIGUSR1 may interrupt us here, when a message is being sent to the user
*/
			if (errno == EINTR)		/* signal received */
				return 0;

			return -1;				/* some error, probably EBADF */
		}
		if (!err) {					/* no fd's ready */
			if (tv == NULL)
				continue;
			else
				break;
		}
		break;			/* select() succeeded */
	}
	return err;
}

/*
	wait for file descriptor to become writable
*/
int wait_fd_write(int fd, struct timeval *tv) {
fd_set fds;
int err;

	for(;;) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		err = select(fd+1, NULL, &fds, NULL, tv);
		if (err < 0) {
			if (errno == EINTR)		/* signal received (handle it later) */
				continue;			/* retry select() */

			return -1;				/* some error, probably EBADF */
		}
		if (!err) {					/* no fd's ready */
			if (tv == NULL)
				continue;
			else
				break;
		}
		break;			/* select() succeeded */
	}
	return err;
}

/*
	put input data into the read buffer, and block if needed
*/
void feed_input_IOBuf(IOBuf *ib) {
int n;

	assert(ib->in_idx <= 0);

	for(;;) {
		if ((n = read(ib->fd, ib->inbuf, INPUTBUF_SIZE)) <= 0) {
			if (n == -1) {
				if (errno == EINTR)
					continue;
/*
	the read() would block; no data ready, but we can wait for it
*/
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					if (wait_fd_read(ib->fd, NULL) == -1) {
						ib->in_idx = -1;
						return;
					}
					continue;			/* retry read() */
				}
				ib->in_idx = -1;
				return;					/* error, probably EBADF */
			}
			if (!n) {					/* end of file */
				ib->in_idx = -1;
				return;
			}
		}
		break;
	}
	ib->in_idx = n;
}

/*
	forever wait for input (user is busy)
*/
int getch_IOBuf(IOBuf *ib) {
int c;

	flush_IOBuf(ib);			/* implicitly flush out any pending prompts */

	if (ib->in_idx <= 0) {
		feed_input_IOBuf(ib);

		if (ib->in_idx <= 0)
			return IO_ERR;
	}
	ib->in_idx--;
	c = ib->inbuf[0];
	memmove(ib->inbuf, &(ib->inbuf[1]), ib->in_idx);
	return (c & 0xff);
}

int peek_IOBuf(IOBuf *ib, char *dst, int len) {
	if (ib->in_idx < len)
		return -1;

	memcpy(dst, (char *)(ib->inbuf), len);
/*	dst[len] = 0;	*/
	return len;
}

int fetch_IOBuf(IOBuf *ib, char *dst, int len) {
	if (ib->in_idx < len)
		return -1;

	memcpy(dst, (char *)(ib->inbuf), len);
/*	dst[len] = 0;	*/

	ib->in_idx -= len;
	memmove(ib->inbuf, &(ib->inbuf[len]), ib->in_idx);
	return len;
}

/*
	interruptible getch(), when the user is not busy
*/
int igetch_IOBuf(IOBuf *ib, int timeout) {
int n;
struct timeval tv;

	flush_IOBuf(ib);			/* implicitly flush out any pending prompts */

	if (ib->in_idx <= 0) {
		switch(input_ready_IOBuf(ib)) {
			case -1:			/* error */
				return -1;

			case 0:				/* no data ready */
				tv.tv_sec = timeout;
				tv.tv_usec = 0;

				if ((n = wait_fd_read(ib->fd, &tv)) == -1)
					return IO_ERR;

				if (!n)			/* interrupted; a message arrived */
					return IO_EINTR;

				break;

			default:			/* data ready, do normal getch */
				;
		}
	}
	return getch_IOBuf(ib);
}

int flush_IOBuf(IOBuf *ob) {
int n;

	if (ob->redirect != NULL)
		return 0;

	if (ob->out_idx <= 0)
		return 0;

	while(ob->out_idx > 0) {
		n = write(ob->fd, ob->outbuf, ob->out_idx);
		if (n == -1) {
			if (errno == EINTR)
				continue;
/*
	the write() would block; we can wait for it
*/
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (wait_fd_write(ob->fd, NULL) == -1) {
					ob->out_idx = -1;
					return -1;
				}
				continue;			/* retry write() */
			}
			return -1;		/* some kind of error, probably EBADF ? */
		}
		ob->out_idx -= n;
	}
	ob->out_idx = 0;
	return 0;
}

int write_IOBuf(IOBuf *ob, void *data, int len) {
int n;

	if (ob->redirect != NULL) {
/*
	NB. output redirection may not work correctly when data contains a 0-byte
	In practice, this never happens because it works with strings and strlen() all the way
*/
		return gstrcat(ob->redirect, (char *)data);
	}
	if (len <= 0)
		return 0;

	if (len > OUTPUTBUF_SIZE) {			/* too much data to hold in IOBuf structure */
		if (flush_IOBuf(ob) == -1)
			return -1;

/* too much data, simply write it out directly without buffering in between */
		while(len > 0) {
			n = write(ob->fd, data, len);
			if (n == -1) {
				if (errno == EINTR)
					continue;
/*
	the write() would block; we can wait for it
*/
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					if (wait_fd_write(ob->fd, NULL) == -1) {
						ob->out_idx = -1;
						return -1;
					}
					continue;			/* retry write() */
				}
				return -1;		/* some kind of error, probably EBADF ? */
			}
			len -= n;
		}
		return 0;
	}
	if (ob->out_idx + len > OUTPUTBUF_SIZE) {
		if (flush_IOBuf(ob) == -1)
			return -1;
	}
	memcpy(ob->outbuf + ob->out_idx, data, len);		/* buffer data */
	ob->out_idx += len;
	return 0;
}

int putc_IOBuf(IOBuf *ob, int c) {
	if (ob->redirect != NULL)
		return gputc(ob->redirect, c);

	if (ob->out_idx >= OUTPUTBUF_SIZE)
		if (flush_IOBuf(ob) == -1)
			return -1;

	ob->outbuf[ob->out_idx++] = c;
	return 0;
}

void close_IOBuf(IOBuf *iob) {
	flush_IOBuf(iob);

	if (iob->fd > 0) {
		shutdown(iob->fd, SHUT_RDWR);
		close(iob->fd);
	}
	iob->fd = -1;
	iob->in_idx = iob->out_idx = 0;
	iob->redirect = NULL;
}

/*
	redirect writes to a GString
	this allows for buffering large amounts of output, which is used for the pager
*/
void redirect_IOBuf(IOBuf *iob, GString *g) {
	flush_IOBuf(iob);

	iob->redirect = g;
}

/* EOB */
