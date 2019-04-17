/*
	main.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "main.h"
#include "inet.h"
#include "log.h"
#include "defines.h"
#include "sigs.h"
#include "User.h"
#include "keys.h"
#include "login.h"
#include "online.h"
#include "roomlevel.h"
#include "util.h"
#include "db.h"
#include "passwd.h"
#include "Stats.h"
#include "Timer.h"
#include "sysop_menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>


#ifndef TELOPT_NAWS
#define TELOPT_NAWS 31			/* negotiate about window size */
#endif

#ifndef TELOPT_NEW_ENVIRON
#define TELOPT_NEW_ENVIRON 39	/* set new environment variable */
#endif

/*
	the main() arguments are saved here
*/
int sys_argc;
char **sys_argv;

static pthread_t main_thread;
static int main_sock = -1;
static int stop_server = 0;

/*
	user thread code
*/
void *user_thread(void *arg) {
int sock;
User *usr;

	sock = (int)arg;
	usr = new_User();

	usr->tid = pthread_self();

	connect_User(usr, sock);

	Print(usr, "%c%c%c%c%c%c%c%c%c%c%c%c\n"
		"bbs101 Copyright (c) 2009 Walter de Jong <walter@heiho.net>\n",
		IAC, WILL, TELOPT_SGA, IAC, WILL, TELOPT_ECHO,
		IAC, DO, TELOPT_NAWS, IAC, DO, TELOPT_NEW_ENVIRON);

	login(usr);
	roomlevel(usr);
	return NULL;
}

void inet_server(void) {
int s;

	main_thread = pthread_self();

	if ((main_sock = inet_listen("0.0.0.0", "1234")) == -1)
		exit(-1);

	log_msg("main thread is accepting connections");
	for(;;) {
		if ((s = inet_accept(main_sock, NULL)) == -1) {
			if (errno == EINTR) {
				if (stop_server) {
					log_msg("main thread no longer accepting connections");
					shutdown(main_sock, SHUT_RDWR);
					close(main_sock);
					main_sock = -1;
					break;
				}
				continue;
			}
			log_err("inet_server(): error on the main socket, aborting");
			abort();
		}
		spawn(user_thread, (void *)s);
	}
/*
	do no exit the main thread yet; instead sleep forever
	the exit() will be performed by a shutdown function
*/
	for(;;) {
		sleep(3600);
	}
}

void stop_inet_server(void) {
/*
	no mutexes here ... I'll take my chances
*/
	stop_server = 1;
	pthread_kill(main_thread, SIGUSR1);
}

int main(int argc, char *argv[]) {
	sys_argc = argc;
	sys_argv = argv;			/* save this variable, comes in handy later */

	printf("bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>\n");

	init_signals();
	init_log();

	log_msg("system is starting");

	init_timerq();
	init_online_list();
	init_passwd();
	init_stats();
	init_reboot_mutex();

	if (init_db())
		exit(-1);

	load_stats();

	inet_server();
	return 0;
}

/* EOB */
