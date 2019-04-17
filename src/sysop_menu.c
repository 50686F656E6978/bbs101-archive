/*
	sysop_menu.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "sysop_menu.h"
#include "keys.h"
#include "edit.h"
#include "bufprintf.h"
#include "util.h"
#include "online.h"
#include "backend.h"
#include "db.h"
#include "log.h"
#include "Timer.h"
#include "edit.h"
#include "cstring.h"
#include "passwd.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


static int reboot_timer = -1;
static int shutdown_timer = -1;
static pthread_mutex_t reboot_mutex;	/* reboot_timer and shutdown_timer share the same mutex */


void init_reboot_mutex(void) {
	pthread_mutex_init(&reboot_mutex, NULL);
}

void sysop_menu(User *usr) {
int c, print_menu;

	print_menu = 1;

	for(;;) {
		if (print_menu) {
			Put(usr, "<magenta>\n"
				" <hotkey>Disconnect user                   <white>Ctrl-<hotkey>N<magenta>uke user\n"
				"\n");

			pthread_mutex_lock(&reboot_mutex);

			Print(usr, " <white>%sCtrl-<hotkey>R<magenta>eboot %s            ",
				(reboot_timer == -1) ? "" : "Cancel ",
				(reboot_timer == -1) ? "          " : "<white>[!]"
			);
			Print(usr, "<white>%sCtrl-<hotkey>S<magenta>hutdown%s\n",
				(shutdown_timer == -1) ? "" : "Cancel ",
				(shutdown_timer == -1) ? "" : " <white>[!]<magenta>"
			);
			pthread_mutex_unlock(&reboot_mutex);
		}
		print_menu = 1;

		Put(usr, "\n<yellow>Sysop# <white>");

		c = Getch(usr);

		switch(c) {
			case KEY_CTRL('C'):
			case KEY_CTRL('D'):
			case ' ':
			case KEY_RETURN:
			case KEY_BS:
				Put(usr, "\n");
				return;

			case KEY_CTRL('L'):			/* reprint menu */
				Put(usr, "\n");
				break;

			case 'd':
			case 'D':
				Put(usr, "Disconnect user\n");
				disconnect_user(usr);
				break;

			case KEY_CTRL('N'):
				Put(usr, "Nuke user\n");
				nuke_user(usr);
				break;

			case KEY_CTRL('R'):
				reboot_system(usr);
				break;

			case KEY_CTRL('S'):
				shutdown_system(usr);
				break;

			case '$':
				Put(usr, "Exiting Sysop mode\n");
				usr->runtime_flags &= ~RTF_SYSOP;
				return;

			default:
				print_menu = 0;
		}
	}
}

void disconnect_user(User *usr) {
char name[MAX_NAME];
User *u;

	Put(usr, "<green>Enter name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "<red>That's not a very good idea\n");
		return;
	}
	if (is_sysop(name)) {
		Put(usr, "<red>You are not allowed to disconnect a fellow sysop\n");
		return;
	}
	if (!is_online(name, NULL)) {
		if (!user_exists(name)) {
			Put(usr, "<red>No such user\n");
			return;
		}
		Print(usr, "<yellow>%s<red> is not online\n", name);
		return;
	}
	if (yesno(usr, "<cyan>Are you sure? (y/N): <white>", 'N') == YESNO_YES) {
		if ((u = lock_user(name)) == NULL) {
			if (!user_exists(name))
				Print(usr, "<yellow>%s<red> was already nuked by another Sysop\n", name);
			else
				Print(usr, "<yellow>%s<red> is not online anymore\n", name);
			return;
		}
		Put(u, "\n\n<yellow>*** <red>Sorry, but you are being disconnected <white>NOW <yellow>***\n");
		reset_colors(u);
		Put(u, "\n");
		Flush(u);

		log_info("%s is being disconnected by Sysop:%s", name, usr->name);

		save_User(u);
		u->name[0] = 0;
		u->runtime_flags |= RTF_DEADCONN;	/* signal dead connection */
		wakeup(u);
		unlock_user(u);
	}
}

void nuke_user(User *usr) {
char name[MAX_NAME];
User *u;

	Put(usr, "<green>Enter name: <yellow>");

	edit_tabname(usr, name);

	if (!name[0])
		return;

	if (!strcmp(usr->name, name)) {
		Put(usr, "<red>Suicide is not painless ...\n");
		return;
	}
	if (is_sysop(name)) {
		Put(usr, "<red>You can't nuke other Sysops!\n");
		return;
	}
	if (!user_exists(name)) {
		Put(usr, "<red>No such user, maybe already nuked by another Sysop?\n");
		return;
	}
	if (yesno(usr, "<cyan>Are you sure? (y/N): <white>", 'N') == YESNO_YES) {
		if ((u = lock_user(name)) != NULL) {
			Put(u, "\n\n<yellow>*** <red>Sorry, but you are being disconnected <white>NOW <yellow>***\n");
			reset_colors(u);
			Put(u, "\n");
			Flush(u);

			log_info("%s is being disconnected by Sysop:%s", name, usr->name);

			locked_remove_online_user(u);
			u->name[0] = 0;

			u->name[0] = 0;
			u->runtime_flags |= RTF_DEADCONN;	/* signal dead connection */
			wakeup(u);
			unlock_user(u);
		}
		if (delete_user(usr, name) == -1)
			Put(usr, "<red>An error occurred, please check\n");
	}
}

void reboot_users(User *u) {
	Put(u, "\n\n<red>");
	hline(u, '*');
	Put(u, "\n<white>");
	center(u, "The BBS is rebooting NOW");
	Put(u, "\n<yellow>");
	center(u, "The system is going down and will return momentarily");
	center(u, "We apologize for the inconvenience");
	Put(u, "\n<red>");
	hline(u, '*');
	beep(u);

	reset_colors(u);
	Put(u, "\n");
	Flush(u);

	save_User(u);
	u->name[0] = 0;
	u->runtime_flags |= RTF_DEADCONN;	/* signal dead connection */
	wakeup(u);
}

static void reboot_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);		/* keep multiple reboots from running concurrently */

	log_msg("system reboot started");

	stop_inet_server();

	log_msg("logging off all users");

	broadcast_callback(reboot_users);

/* see if all users are gone yet */
	if (count_online() != 0) {
		log_msg("waiting on disconnect users ...");
		sleep(1);
	}
	if (count_online() != 0) {
		log_msg("waiting on disconnect users ...");
		sleep(1);
	}
	if (count_online() != 0)
		log_msg("no longer waiting for disconnect to complete");

	db_close();
	log_msg("system is rebooting now");
	log_msg("--");
/*
	restart the program
*/
	execv(sys_argv[0], sys_argv);

	log_err("system reboot failed!");
	exit(-1);
}

static void reboot10_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);

	if ((reboot_timer = start_Timer(TIMER_ONESHOT, 10, reboot_timerfunc, NULL)) == -1) {
		log_err("failed to start reboot timer for 10 seconds");
		pthread_mutex_unlock(&reboot_mutex);

		broadcast(NULL, "error in reboot timer; system not rebooting");
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	broadcast(NULL, "The system is rebooting in 10 seconds");
}

static void reboot30_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);

	if ((reboot_timer = start_Timer(TIMER_ONESHOT, 20, reboot10_timerfunc, NULL)) == -1) {
		log_err("failed to start reboot timer for 30 seconds");
		pthread_mutex_unlock(&reboot_mutex);

		broadcast(NULL, "error in reboot timer; system not rebooting");
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	broadcast(NULL, "The system is rebooting in 30 seconds");
}

static void reboot60_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);

	if ((reboot_timer = start_Timer(TIMER_ONESHOT, 30, reboot30_timerfunc, NULL)) == -1) {
		log_err("failed to start reboot timer for 60 seconds");
		pthread_mutex_unlock(&reboot_mutex);

		broadcast(NULL, "error in reboot timer; system not rebooting");
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	broadcast(NULL, "The system is rebooting in one minute");
}

void reboot_system(User *usr) {
char buf[MAX_LINE], buf2[MAX_LINE];
int seconds, time2dd;
void (*timerfunc)(void *);

	pthread_mutex_lock(&reboot_mutex);

	if (reboot_timer != -1) {
		Put(usr, "Cancel reboot\n");

		if (cancel_Timer(reboot_timer) == -1) {
			reboot_timer = -1;
			pthread_mutex_unlock(&reboot_mutex);

			Put(usr, "<red>Failed to cancel reboot!\n"
				"(this means that the timer is already gone)\n");
			return;
		}
		reboot_timer = -1;
		pthread_mutex_unlock(&reboot_mutex);

		Put(usr, "<yellow>Reboot cancelled\n");
		log_msg("SYSOP %s cancelled reboot", usr->name);
		broadcast(usr, "Reboot cancelled");
		return;
	} else
		Put(usr, "Reboot\n");

	pthread_mutex_unlock(&reboot_mutex);

	Put(usr, "<red>Enter reboot time in seconds:<yellow> ");

	if (edit_number(usr, buf) == -1)
		return;

	seconds = cstrtoul(buf, 10);

	Put(usr, "\n<yellow>*** <white>WARNING <yellow>***\n"
		"\n"
		"<red>This is serious. Enter the reboot password and the system will reboot ");
	Print(usr, "in %s\n"
		"\n"
		"Enter reboot password: ", sprint_total_time(seconds, buf, sizeof(buf)));

	edit_password(usr, buf, MAX_LINE);

	if (!buf[0]) {
		pthread_mutex_lock(&reboot_mutex);

		if (reboot_timer != -1)
			Put(usr, "<red>Aborted, but note that another reboot procedure is already running\n");
		else
			Put(usr, "<yellow>Reboot cancelled\n");

		pthread_mutex_unlock(&reboot_mutex);
		return;
	}
	if (check_password(usr->name, buf)) {
		Put(usr, "Wrong password\n");

		pthread_mutex_lock(&reboot_mutex);

		if (reboot_timer != -1)
			Put(usr, "\nNote that another reboot procedure is already running\n");

		pthread_mutex_unlock(&reboot_mutex);
		return;
	}
	pthread_mutex_lock(&reboot_mutex);

	if (reboot_timer != -1) {
		if (cancel_Timer(reboot_timer) == -1) {
			Put(usr, "Failed to stop a previously initiated reboot\n");
			reboot_timer = -1;
		}
	}
/*
	start appropriate timer ... reannounce reboot at one minute, 30 secs, 10 secs
*/
	if (seconds > 60) {
		time2dd = seconds - 60;
		timerfunc = reboot60_timerfunc;
	} else {
		if (seconds > 30) {
			time2dd = seconds - 30;
			timerfunc = reboot30_timerfunc;
		} else {
			if (seconds > 10) {
				time2dd = seconds - 10;
				timerfunc = reboot10_timerfunc;
			} else {
				time2dd = seconds;
				timerfunc = reboot_timerfunc;
			}
		}
	}
	if ((reboot_timer = start_Timer(TIMER_ONESHOT, time2dd, timerfunc, NULL)) == -1) {
		Put(usr, "Failed to start reboot timer\n");

		pthread_mutex_unlock(&reboot_mutex);
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	log_msg("SYSOP %s initiated reboot in %d seconds", usr->name, seconds);

	bufprintf(buf, sizeof(buf), "The system is rebooting in %s", sprint_total_time(seconds, buf2, sizeof(buf2)));
	broadcast(usr, buf);
	log_debug("%s", buf);
}

static void shutdown_users(User *u) {
	Put(u, "\n\n<red>");
	hline(u, '*');
	Put(u, "\n<white>");
	center(u, "The BBS is going down NOW");
	Put(u, "\n<yellow>");
	center(u, "The system is going down for maintenance");
	center(u, "We apologize for the inconvenience");
	Put(u, "\n<red>");
	hline(u, '*');
	beep(u);

	reset_colors(u);
	Put(u, "\n");
	Flush(u);

	save_User(u);
	u->name[0] = 0;
	u->runtime_flags |= RTF_DEADCONN;	/* signal dead connection */
	wakeup(u);
}

static void shutdown_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);		/* keep multiple shutdowns from running concurrently */

	log_msg("system shutting down now");

	stop_inet_server();

	log_msg("logging off all users");

	broadcast_callback(shutdown_users);

/* see if all users are gone yet */
	if (count_online() != 0) {
		log_msg("waiting on disconnect users ...");
		sleep(1);
	}
	if (count_online() != 0) {
		log_msg("waiting on disconnect users ...");
		sleep(1);
	}
	if (count_online() != 0)
		log_msg("no longer waiting for disconnect to complete");

	db_close();
	log_msg("system halted.");
	log_msg("--");
	exit(0);
}

static void shutdown10_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);

	if ((shutdown_timer = start_Timer(TIMER_ONESHOT, 10, shutdown_timerfunc, NULL)) == -1) {
		log_err("failed to start shutdown timer for 10 seconds");
		pthread_mutex_unlock(&reboot_mutex);

		broadcast(NULL, "error in shutdown timer; system not shutting down");
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	broadcast(NULL, "The system is shutting down in 10 seconds");
}

static void shutdown30_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);

	if ((shutdown_timer = start_Timer(TIMER_ONESHOT, 20, shutdown10_timerfunc, NULL)) == -1) {
		log_err("failed to start shutdown timer for 30 seconds");
		pthread_mutex_unlock(&reboot_mutex);

		broadcast(NULL, "error in shutdown timer; system not shutting down");
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	broadcast(NULL, "The system is shutting down in 30 seconds");
}

static void shutdown60_timerfunc(void *arg) {
	pthread_mutex_lock(&reboot_mutex);

	if ((shutdown_timer = start_Timer(TIMER_ONESHOT, 30, shutdown30_timerfunc, NULL)) == -1) {
		log_err("failed to start shutdown timer for 60 seconds");
		pthread_mutex_unlock(&reboot_mutex);

		broadcast(NULL, "error in shutdown timer; system not shutting down");
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	broadcast(NULL, "The system is shutting down in one minute");
}

void shutdown_system(User *usr) {
char buf[MAX_LINE], buf2[MAX_LINE];
int seconds, time2dd;
void (*timerfunc)(void *);

	pthread_mutex_lock(&reboot_mutex);

	if (shutdown_timer != -1) {
		Put(usr, "Cancel shutdown\n");

		if (cancel_Timer(shutdown_timer) == -1) {
			shutdown_timer = -1;
			pthread_mutex_unlock(&reboot_mutex);

			Put(usr, "<red>Failed to cancel shutdown!\n"
				"(this means that the timer is already gone)\n");
			return;
		}
		shutdown_timer = -1;
		pthread_mutex_unlock(&reboot_mutex);

		Put(usr, "<yellow>Shutdown cancelled\n");
		log_msg("SYSOP %s cancelled shutdown", usr->name);
		broadcast(usr, "Shutdown cancelled");
		return;
	} else
		Put(usr, "Shutdown\n");

	pthread_mutex_unlock(&reboot_mutex);

	Put(usr, "<red>Enter shutdown time in seconds:<yellow> ");

	if (edit_number(usr, buf) == -1)
		return;

	seconds = cstrtoul(buf, 10) /* + 60	*/;

	Put(usr, "\n<yellow>*** <white>WARNING <yellow>***\n"
		"\n"
		"<red>This is serious. Enter the shutdown password and the system will shutdown ");
	Print(usr, "in %s\n"
		"\n"
		"Enter shutdown password: ", sprint_total_time(seconds, buf, sizeof(buf)));

	edit_password(usr, buf, MAX_LINE);

	if (!buf[0]) {
		pthread_mutex_lock(&reboot_mutex);

		if (shutdown_timer != -1)
			Put(usr, "<red>Aborted, but note that another shutdown procedure is already running\n");
		else
			Put(usr, "<yellow>Shutdown cancelled\n");

		pthread_mutex_unlock(&reboot_mutex);
		return;
	}
	if (check_password(usr->name, buf)) {
		Put(usr, "Wrong password\n");

		pthread_mutex_lock(&reboot_mutex);

		if (shutdown_timer != -1)
			Put(usr, "\nNote that another shutdown procedure is already running\n");

		pthread_mutex_unlock(&reboot_mutex);
		return;
	}
	pthread_mutex_lock(&reboot_mutex);

	if (shutdown_timer != -1) {
		if (cancel_Timer(shutdown_timer) == -1) {
			Put(usr, "Failed to stop a previously initiated shutdown\n");
			shutdown_timer = -1;
		}
	}
/*
	start appropriate timer ... reannounce shutdown at one minute, 30 secs, 10 secs
*/
	if (seconds > 60) {
		time2dd = seconds - 60;
		timerfunc = shutdown60_timerfunc;
	} else {
		if (seconds > 30) {
			time2dd = seconds - 30;
			timerfunc = shutdown30_timerfunc;
		} else {
			if (seconds > 10) {
				time2dd = seconds - 10;
				timerfunc = shutdown10_timerfunc;
			} else {
				time2dd = seconds;
				timerfunc = shutdown_timerfunc;
			}
		}
	}
	if ((reboot_timer = start_Timer(TIMER_ONESHOT, time2dd, timerfunc, NULL)) == -1) {
		Put(usr, "Failed to start shutdown timer\n");

		pthread_mutex_unlock(&reboot_mutex);
		return;
	}
	pthread_mutex_unlock(&reboot_mutex);

	log_msg("SYSOP %s initiated shutdown in %d seconds", usr->name, seconds);

	bufprintf(buf, sizeof(buf), "The system is shutting down in %s", sprint_total_time(seconds, buf2, sizeof(buf2)));
	broadcast(usr, buf);
	log_debug("%s", buf);
}

/* EOB */
