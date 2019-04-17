/*
	db.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- database interface to MySQL
	- MySQL does support threading, but since I abstracted the database connection,
	  only one thread can access it at a time (for now)
*/

#include "config.h"
#include "db.h"
#include "log.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>


static MYSQL mysql;
static pthread_mutex_t db_lock;

static pthread_once_t init_db_once = PTHREAD_ONCE_INIT;


static void do_init_db(void) {
my_bool opt_reconnect;

	pthread_mutex_init(&db_lock, NULL);

	mysql_init(&mysql);

	opt_reconnect = 1;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, &opt_reconnect);

	db_connect();
}

int init_db(void) {
int err;

	pthread_once(&init_db_once, do_init_db);

	pthread_mutex_lock(&db_lock);

	err = mysql_ping(&mysql);

	pthread_mutex_unlock(&db_lock);

	if (err)
		log_err("init_db(): %s", mysql_error(&mysql));

	return err;
}

int db_connect(void) {
	pthread_mutex_lock(&db_lock);

	if (mysql_real_connect(&mysql, DB_SERVER, DB_USER, DB_PASSWD, DB_NAME, DB_PORT, NULL, CLIENT_REMEMBER_OPTIONS) == NULL) {
		log_err("db_connect(): %s", mysql_error(&mysql));
		pthread_mutex_unlock(&db_lock);
		return -1;
	}
	pthread_mutex_unlock(&db_lock);

	log_info("database connection established");
	return 0;
}

void db_close(void) {
	pthread_mutex_lock(&db_lock);

	log_info("closing database connection");
	mysql_close(&mysql);

	pthread_mutex_unlock(&db_lock);
}

DB_RESULT *db_query(char *sql, int *error) {
MYSQL_RES *result;

	pthread_mutex_lock(&db_lock);
/*
	log_debug("db_query(): %s", sql);
*/
	if (mysql_real_query(&mysql, sql, strlen(sql)) != 0) {
		log_err("db_query(): %s", mysql_error(&mysql));
		pthread_mutex_unlock(&db_lock);
		return NULL;
	}
	if (error != NULL) {
		*error = mysql_errno(&mysql);

		if (*error) {
			pthread_mutex_unlock(&db_lock);
			return NULL;
		}
	}
	if ((result = mysql_store_result(&mysql)) == NULL) {
		int err;

		if ((err = mysql_errno(&mysql)) != 0) {
			if (error != NULL)
				*error = err;

			log_err("db_query(): %s", mysql_error(&mysql));
		}
		pthread_mutex_unlock(&db_lock);
		return NULL;
	}
	pthread_mutex_unlock(&db_lock);
	return result;
}

/*
	Note: sql_escape() may return str or a newly allocated buffer
	So, if (result != str) Free(str);
*/
char *sql_escape(char *str) {
char *dest, *p;
int n, buflen;

	if (str == NULL)
		return NULL;

/*
	count occurrences of characters that need to be escaped
*/
	n = 0;
	p = str;
	while(*p) {
		if (*p == '\r' || *p == '\n' || *p == '\\' || *p == '\'' || *p == '"' || *p == ('Z' - 'A' + 1))	/* <- this value is Ctrl-Z */
			n++;
		p++;
	}
	if (!n)
		return str;			/* no need to escape */

	buflen = strlen(str) + n + 1;

	if ((dest = (char *)Malloc(buflen)) == NULL)
		return NULL;

	if (mysql_real_escape_string(&mysql, dest, str, strlen(str)) <= 0) {
		Free(dest);
		return NULL;
	}
	return dest;
}

/* EOB */
