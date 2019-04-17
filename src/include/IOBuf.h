/*
	IOBuf.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef IOBUF_H_WJ109
#define IOBUF_H_WJ109	1

#include "GString.h"

#define INPUTBUF_SIZE	256
#define OUTPUTBUF_SIZE	(1024 - INPUTBUF_SIZE - 3 * sizeof(int))

#define IO_ERR			-1
#define IO_EINTR		-2


typedef struct {
	int fd, in_idx, out_idx;
	unsigned char inbuf[INPUTBUF_SIZE], outbuf[OUTPUTBUF_SIZE];
	GString *redirect;
} IOBuf;


void init_IOBuf(IOBuf *);
IOBuf *new_IOBuf(int);
void destroy_IOBuf(IOBuf *);

void connect_IOBuf(IOBuf *, int);
int getch_IOBuf(IOBuf *);
int igetch_IOBuf(IOBuf *, int);
int peek_IOBuf(IOBuf *, char *, int);
int fetch_IOBuf(IOBuf *, char *, int);
int flush_IOBuf(IOBuf *);
int write_IOBuf(IOBuf *, void *, int);
int putc_IOBuf(IOBuf *, int);
void close_IOBuf(IOBuf *);
void redirect_IOBuf(IOBuf *, GString *);

#endif	/* IOBUF_H_WJ109 */

/* EOB */
