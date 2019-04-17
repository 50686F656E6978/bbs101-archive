/*
	version.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "version.h"
#include "build_time.h"

#include <stdio.h>
#include <stdlib.h>


void print_version_info(User *usr) {
	Put(usr, "<yellow>This is <white>bbs101<yellow>, version <white>" BBS_VERSION " "
		"<yellow>build " BUILD_TIME "\n");
}

/* EOB */
