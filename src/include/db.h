/*
	db.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef DB_H_WJ109
#define DB_H_WJ109	1

#include <mysql.h>

#define DB_SERVER	"localhost"
#define DB_USER		"bbs101"
#define DB_PASSWD	"bbs1oh1"
#define DB_NAME		"bbs101"
#define DB_PORT		0

#define DB_RESULT			MYSQL_RES

#define db_num_rows(x)		mysql_num_rows(x)
#define db_fetch_row(x)		(char **)mysql_fetch_row(x)
#define db_free_result(x)	mysql_free_result(x)

int init_db(void);
int db_connect(void);
void db_close(void);
DB_RESULT *db_query(char *, int *);
char *sql_escape(char *);

#endif	/* DB_H_WJ109 */

/* EOB */
