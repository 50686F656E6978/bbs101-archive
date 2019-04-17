/*
	backend.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#include "config.h"
#include "backend.h"
#include "db.h"
#include "bufprintf.h"
#include "defines.h"
#include "cstring.h"
#include "passwd.h"
#include "log.h"
#include "Memory.h"
#include "GString.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int user_exists(char *name) {
char sql[MAX_LINE];
DB_RESULT *res;
int rows;

/*
	see if username exists in the database
*/
	cstrcpy(sql, "SELECT id FROM Users WHERE name = '", MAX_LINE);
	cstrcat(sql, name, MAX_LINE);
	cstrcat(sql, "'", MAX_LINE);

	if ((res = db_query(sql, NULL)) == NULL)
		return 0;

	rows = db_num_rows(res);
	assert(rows <= 1);

	db_free_result(res);
	return rows;
}

int get_crypted_passwd(char *name, char *buf, int len) {
	return get_user_field(name, "password", buf, len);
}

int get_user_field(char *name, char *field, char *buf, int len) {
char sql[MAX_LINE], **row;
DB_RESULT *res;
int num_rows, err;

	bufprintf(sql, MAX_LINE, "SELECT %s FROM Users WHERE name = '%s'", field, name);

	if ((res = db_query(sql, &err)) == NULL)
		return -1;

	if (err) {
		db_free_result(res);
		return -1;
	}
	num_rows = db_num_rows(res);
	if (num_rows != 1) {
		db_free_result(res);
		return -1;
	}
	if ((row = db_fetch_row(res)) == NULL) {
		db_free_result(res);
		return -1;
	}
	cstrcpy(buf, row[0], len);

	db_free_result(res);
	return 0;
}

int create_user(char *name, char *password) {
char sql[PRINT_BUF], crypted[MAX_CRYPTED];
DB_RESULT *res;
int err;

	if (crypt_password(password, crypted, MAX_CRYPTED) == -1)
		return -1;

	bufprintf(sql, PRINT_BUF, "INSERT INTO Users (name,password) VALUES ('%s', '%s')", name, crypted);

	if ((res = db_query(sql, &err)) != NULL)
		db_free_result(res);

	if (err)
		return -1;

	return 0;
}

/*
	load only the most needed data (so _not_ a full profile)
*/
int load_User(User *usr, char *name) {
char sql[400];
DB_RESULT *res;
int rows;
char **row;

	if (name == NULL || !*name)
		return -1;

	if (usr->name[0] && usr->dirty.len > 0)
		save_User(usr);

	cstrcpy(usr->name, name, MAX_NAME);

	cstrcpy(sql, "SELECT doing,reminder,default_anon,timezone,xmsg_header,"			\
		"birth,logins,total_time,xsent,xrecv,esent,erecv,fsent,frecv,qsent,qansw,"	\
		"msgs_posted,msgs_read,flags,default_room,colors,symbol_colors,quick,"		\
		"friends,enemies,display_flags,last_logout,last_online_time,last_from"		\
		" FROM Users WHERE name = '", sizeof(sql)-1);
	cstrcat(sql, name, sizeof(sql)-1);
	cstrcat(sql, "'", sizeof(sql)-1);

	if ((res = db_query(sql, NULL)) == NULL)
		return -1;

	rows = db_num_rows(res);
	assert(rows <= 1);

	if (rows < 1) {
		db_free_result(res);
		return -1;
	}
	if ((row = db_fetch_row(res)) == NULL) {
		db_free_result(res);
		return -1;
	}
	usr->doing = cstrdup(row[0]);
	usr->reminder = cstrdup(row[1]);
	cstrcpy(usr->default_anon, row[2], MAX_NAME);
	cstrcpy(usr->timezone, row[3], MAX_HALFLINE);
	usr->xmsg_header = cstrdup(row[4]);

	usr->birth = cstrtoul(row[5], 10);
	usr->logins = cstrtoul(row[6], 10);
	usr->total_time = cstrtoul(row[7], 10);
	usr->xsent = cstrtoul(row[8], 10);
	usr->xrecv = cstrtoul(row[9], 10);
	usr->esent = cstrtoul(row[10], 10);
	usr->erecv = cstrtoul(row[11], 10);
	usr->fsent = cstrtoul(row[12], 10);
	usr->frecv = cstrtoul(row[13], 10);
	usr->qsent = cstrtoul(row[14], 10);
	usr->qansw = cstrtoul(row[15], 10);
	usr->msgs_posted = cstrtoul(row[16], 10);
	usr->msgs_read = cstrtoul(row[17], 10);

	load_User_flags(usr, row[18]);

	usr->default_room = cstrtoul(row[19], 10);

	cstrcpy(usr->colors, row[20], MAX_LINE);
	cstrcpy(usr->symbol_colors, row[21], MAX_LINE);

	gstrcpy(&(usr->quick), row[22]);
	gstrcpy(&(usr->friends), row[23]);
	gstrcpy(&(usr->enemies), row[24]);

	load_Display_flags(&(usr->display), row[25]);

	usr->last_logout = cstrtoul(row[26], 10);
	usr->last_online_time = cstrtoul(row[27], 10);
	usr->last_from = cstrdup(row[28]);

	db_free_result(res);
	return 0;
}

/*
	load user profile info
*/
int load_User_profile(User *usr, char *name) {
char sql[300];
DB_RESULT *res;
int rows;
char **row;

	if (usr->name[0] && usr->dirty.len > 0)
		save_User(usr);

	cstrcpy(usr->name, name, MAX_NAME);

	cstrcpy(sql, "SELECT real_name,city,state,country,email,www,flags,"	\
		"friends,enemies,info,last_logout,last_online_time,last_from,total_time FROM Users WHERE name = '", sizeof(sql));
	cstrcat(sql, name, sizeof(sql));
	cstrcat(sql, "'", sizeof(sql));

	if ((res = db_query(sql, NULL)) == NULL)
		return -1;

	rows = db_num_rows(res);
	assert(rows <= 1);

	if (rows < 1) {
		db_free_result(res);
		return -1;
	}
	if ((row = db_fetch_row(res)) == NULL) {
		db_free_result(res);
		return -1;
	}
	usr->real_name = cstrdup(row[0]);
	usr->city = cstrdup(row[1]);
	usr->state = cstrdup(row[2]);
	usr->country = cstrdup(row[3]);
	usr->email = cstrdup(row[4]);
	usr->www = cstrdup(row[5]);

	load_User_flags(usr, row[6]);
/*
	friends and enemies may already have been loaded in this user object
*/
	if (usr->friends.len <= 0)
		gstrcpy(&(usr->friends), row[7]);

	if (usr->enemies.len <= 0)
		gstrcpy(&(usr->enemies), row[8]);

	usr->info = cstrdup(row[9]);

	usr->last_logout = cstrtoul(row[10], 10);
	usr->last_online_time = cstrtoul(row[11], 10);
	usr->last_from = cstrdup(row[12]);
	usr->total_time = cstrtoul(row[13], 10);

	db_free_result(res);
	return 0;
}

/*
	ditch memory that is not frequently used anyway
*/
void unload_User_profile(User *usr) {
	Free(usr->real_name);
	usr->real_name = NULL;

	Free(usr->city);
	usr->city = NULL;

	Free(usr->state);
	usr->state = NULL;

	Free(usr->country);
	usr->country = NULL;

	Free(usr->email);
	usr->email = NULL;

	Free(usr->www);
	usr->www = NULL;

	Free(usr->info);
	usr->info = NULL;
}

int user_int_field(User *u, char *field, unsigned int *ival) {
	if (!strcmp(field, "birth")) {
		*ival = u->birth;
		return 0;
	}
	if (!strcmp(field, "login_time")) {
		*ival = u->login_time;
		return 0;
	}
	if (!strcmp(field, "last_logout")) {
		*ival = u->last_logout;
		return 0;
	}
	if (!strcmp(field, "logins")) {
		*ival = u->logins;
		return 0;
	}
	if (!strcmp(field, "total_time")) {
		*ival = u->total_time;
		return 0;
	}
	if (!strcmp(field, "last_online_time")) {
		*ival = u->last_online_time;
		return 0;
	}
	if (!strcmp(field, "xsent")) {
		*ival = u->xsent;
		return 0;
	}
	if (!strcmp(field, "xrecv")) {
		*ival = u->xrecv;
		return 0;
	}
	if (!strcmp(field, "esent")) {
		*ival = u->esent;
		return 0;
	}
	if (!strcmp(field, "erecv")) {
		*ival = u->erecv;
		return 0;
	}
	if (!strcmp(field, "fsent")) {
		*ival = u->fsent;
		return 0;
	}
	if (!strcmp(field, "frecv")) {
		*ival = u->frecv;
		return 0;
	}
	if (!strcmp(field, "qsent")) {
		*ival = u->qsent;
		return 0;
	}
	if (!strcmp(field, "qansw")) {
		*ival = u->qansw;
		return 0;
	}
	if (!strcmp(field, "msgs_posted")) {
		*ival = u->msgs_posted;
		return 0;
	}
	if (!strcmp(field, "msgs_read")) {
		*ival = u->msgs_read;
		return 0;
	}
	if (!strcmp(field, "default_room")) {
		*ival = u->default_room;
		return 0;
	}
	return -1;
}

int user_safe_string_field(User *u, char *field, char **sval) {
/*
	if (!strcmp(field, "password")) {
		*sval = &(u->password);
		return 0;
	}
*/
	if (!strcmp(field, "default_anon")) {
		*sval = u->default_anon;
		return 0;
	}
	if (!strcmp(field, "timezone")) {
		*sval = u->timezone;
		return 0;
	}
	if (!strcmp(field, "last_from")) {
		*sval = u->last_from;
		return 0;
	}
	if (!strcmp(field, "colors")) {
		*sval = u->colors;
		return 0;
	}
	if (!strcmp(field, "quick")) {
		*sval = u->quick.str;
		return 0;
	}
	if (!strcmp(field, "friends")) {
		*sval = u->friends.str;
		return 0;
	}
	if (!strcmp(field, "enemies")) {
		*sval = u->enemies.str;
		return 0;
	}
/*
	the override list is always temporary for a session; it is never saved

	if (!strcmp(field, "override")) {
		*sval = u->override.str;
		return 0;
	}
*/
	return -1;
}

int user_quoted_string_field(User *u, char *field, char **sval) {
	if (!strcmp(field, "real_name")) {
		*sval = u->real_name;
		return 0;
	}
	if (!strcmp(field, "city")) {
		*sval = u->city;
		return 0;
	}
	if (!strcmp(field, "state")) {
		*sval = u->state;
		return 0;
	}
	if (!strcmp(field, "country")) {
		*sval = u->country;
		return 0;
	}
	if (!strcmp(field, "email")) {
		*sval = u->email;
		return 0;
	}
	if (!strcmp(field, "www")) {
		*sval = u->www;
		return 0;
	}
	if (!strcmp(field, "doing")) {
		*sval = u->doing;
		return 0;
	}
	if (!strcmp(field, "reminder")) {
		*sval = u->reminder;
		return 0;
	}
	if (!strcmp(field, "vanity")) {
		*sval = u->vanity;
		return 0;
	}
	if (!strcmp(field, "xmsg_header")) {
		*sval = u->xmsg_header;
		return 0;
	}
	if (!strcmp(field, "info")) {
		*sval = u->info;
		return 0;
	}
	return -1;
}

int user_gstring_field(User *u, char *field, GString *gval) {
	if (!strcmp(field, "flags")) {
		save_User_flags(u, gval);
		return 0;
	}
	if (!strcmp(field, "display_flags")) {
		save_Display_flags(&(u->display), gval);
		return 0;
	}
	return -1;
}

/*
	update all dirty fields in the user
*/
int save_User(User *usr) {
GString sql, gval;
char *escape_buf, *p, *endp;
DB_RESULT *res;
int err;
unsigned int ival;
char *sval;

	if (!usr->name[0])
		return -1;

	if (usr->dirty.len <= 0)
		return 0;

	init_GString(&sql);

	gstrcpy(&sql, "UPDATE Users SET ");

	p = usr->dirty.str;
	while((endp = cstrchr(p, ',')) != NULL) {
		*endp = 0;

		if (!user_int_field(usr, p, &ival))
			gprint_add(&sql, "%s = %u,", p, ival);
		else
			if (!user_safe_string_field(usr, p, &sval)) {
				if (sval == NULL)
					gprint_add(&sql, "%s = NULL,", p);
				else
					gprint_add(&sql, "%s = '%s',", p, sval);
			} else
				if (!user_quoted_string_field(usr, p, &sval)) {
					if (sval == NULL)
						gprint_add(&sql, "%s = NULL,", p);
					else {
						if ((escape_buf = sql_escape(sval)) == NULL)
							log_err("save_User(): sql_escape() returned NULL");
						else {
							gprint_add(&sql, "%s = '", p);
							gstrcat(&sql, escape_buf);
							gputc(&sql, '\'');

							if (escape_buf != sval)
								Free(escape_buf);
						}
					}
				} else {
					init_GString(&gval);

					if (!user_gstring_field(usr, p, &gval)) {
						if (gval.len <= 0)
							gprint_add(&sql, "%s = NULL,", p);
						else
							gprint_add(&sql, "%s = '%s',", p, gval.str);

						deinit_GString(&gval);
					} else {
						log_err("save_User(): don't know how to handle field '%s'", p);
						assert(1 == 0);
					}
				}

		endp++;
		p = endp;
	}
	if (!user_int_field(usr, p, &ival))
		gprint_add(&sql, "%s = %u", p, ival);
	else
		if (!user_safe_string_field(usr, p, &sval)) {
			if (sval == NULL)
				gprint_add(&sql, "%s = NULL", p);
			else
				gprint_add(&sql, "%s = '%s'", p, sval);
		} else
			if (!user_quoted_string_field(usr, p, &sval)) {
				if (sval == NULL)
					gprint_add(&sql, "%s = NULL", p);
				else {
					if ((escape_buf = sql_escape(sval)) == NULL)
						log_err("save_User(): sql_escape() returned NULL");
					else {
						gprint_add(&sql, "%s = '", p);
						gstrcat(&sql, escape_buf);
						gputc(&sql, '\'');

						if (escape_buf != sval)
							Free(escape_buf);
					}
				}
			} else {
				init_GString(&gval);

				if (!user_gstring_field(usr, p, &gval)) {
					if (gval.len <= 0)
						gprint_add(&sql, "%s = NULL", p);
					else
						gprint_add(&sql, "%s = '%s'", p, gval.str);

					deinit_GString(&gval);
				} else {
					log_err("save_User(): don't know how to handle field '%s'", p);
					assert(1 == 0);
				}
			}

	gprint_add(&sql, " WHERE name = '%s'", usr->name);

	if ((res = db_query(sql.str, &err)) != NULL)
		db_free_result(res);

	deinit_GString(&sql);

	if (err)
		return -1;

	reset_GString(&(usr->dirty));
	return 0;
}

int save_User_int(User *usr, char *field, unsigned int i) {
char sql[MAX_LINE];
DB_RESULT *res;
int err;

	if (!usr->name[0])
		return -1;

	bufprintf(sql, MAX_LINE, "UPDATE Users SET %s = %u WHERE name = '%s'", field, i, usr->name);

	if ((res = db_query(sql, &err)) != NULL)
		db_free_result(res);

	if (err)
		return -1;

	return 0;
}

int save_password(char *username, char *crypted) {
char sql[256];
DB_RESULT *res;
int err;

	if (username == NULL || !*username || crypted == NULL || !*crypted)
		return -1;

	bufprintf(sql, sizeof(sql), "UPDATE Users SET password = '%s' WHERE name = '%s'", crypted, username);

	if ((res = db_query(sql, &err)) != NULL)
		db_free_result(res);

	if (err)
		return -1;

	return 0;
}

/*
	see if this user is a registered Sysop
*/
int is_sysop(char *name) {
char sql[MAX_LINE];
DB_RESULT *res;
int rows;

	if (name == NULL || !*name)
		return 0;

	cstrcpy(sql, "SELECT id FROM Sysops WHERE name = '", MAX_LINE);
	cstrcat(sql, name, MAX_LINE);
	cstrcat(sql, "'", MAX_LINE);

	if ((res = db_query(sql, NULL)) == NULL)
		return 0;

	rows = db_num_rows(res);
	assert(rows <= 1);

	db_free_result(res);
	return rows;
}

/*
	return list of sysops
	return value must be Free()d afterwards
*/
char *get_sysops(void) {
DB_RESULT *res;
int rows, i;
char **row, *ret;
GString gstr;

	if ((res = db_query("SELECT name from Sysops", NULL)) == NULL)
		return NULL;

	rows = db_num_rows(res);
	if (rows <= 0) {
		db_free_result(res);
		return NULL;
	}
	if ((row = db_fetch_row(res)) == NULL) {
		db_free_result(res);
		return NULL;
	}
	init_GString(&gstr);
	gstrcpy(&gstr, row[0]);

	for(i = 1; i < rows; i++) {
		gputc(&gstr, ',');
		gstrcat(&gstr, row[i]);
	}
	db_free_result(res);

	ret = gstr.str;
	gstr.str = NULL;
	deinit_GString(&gstr);
	return ret;
}

int backend_load_stats(Stats *s) {
char sql[400];
DB_RESULT *res;
int rows;
char **row;

	cstrcpy(sql, "SELECT oldest,youngest,most_logins,most_xsent,most_xrecv,"	\
		"most_esent,most_erecv,most_fsent,most_frecv,most_qsent,most_qansw,"	\
		"most_posted,most_read,oldest_birth,youngest_birth,oldest_age,"			\
		"logins,xsent,xrecv,esent,erecv,fsent,frecv,qsent,qansw,msgs_posted,msgs_read"	\
		" FROM Stats", sizeof(sql)-1);

	if ((res = db_query(sql, NULL)) == NULL)
		return -1;

	rows = db_num_rows(res);
	assert(rows <= 1);

	if (rows < 1) {
		db_free_result(res);
		return -1;
	}
	if ((row = db_fetch_row(res)) == NULL) {
		db_free_result(res);
		return -1;
	}
	cstrcpy(s->oldest, row[0], MAX_NAME);
	cstrcpy(s->youngest, row[1], MAX_NAME);
	cstrcpy(s->most_logins, row[2], MAX_NAME);
	cstrcpy(s->most_xsent, row[3], MAX_NAME);
	cstrcpy(s->most_xrecv, row[4], MAX_NAME);
	cstrcpy(s->most_esent, row[5], MAX_NAME);
	cstrcpy(s->most_erecv, row[6], MAX_NAME);
	cstrcpy(s->most_fsent, row[7], MAX_NAME);
	cstrcpy(s->most_frecv, row[8], MAX_NAME);
	cstrcpy(s->most_qsent, row[9], MAX_NAME);
	cstrcpy(s->most_qansw, row[10], MAX_NAME);
	cstrcpy(s->most_posted, row[11], MAX_NAME);
	cstrcpy(s->most_read, row[12], MAX_NAME);

	s->oldest_birth = cstrtoul(row[13], 10);
	s->youngest_birth = cstrtoul(row[14], 10);
	s->oldest_age = cstrtoul(row[15], 10);
	s->logins = cstrtoul(row[16], 10);
	s->xsent = cstrtoul(row[17], 10);
	s->xrecv = cstrtoul(row[18], 10);
	s->esent = cstrtoul(row[19], 10);
	s->erecv = cstrtoul(row[20], 10);
	s->fsent = cstrtoul(row[21], 10);
	s->frecv = cstrtoul(row[22], 10);
	s->qsent = cstrtoul(row[23], 10);
	s->qansw = cstrtoul(row[24], 10);
	s->msgs_posted = cstrtoul(row[25], 10);
	s->msgs_read = cstrtoul(row[26], 10);

	db_free_result(res);
	return 0;
}

int stats_int_field(Stats *s, char *field, unsigned int *ival) {
	if (!strcmp(field, "oldest_birth")) {
		*ival = (unsigned int)(s->oldest_birth);
		return 0;
	}
	if (!strcmp(field, "youngest_birth")) {
		*ival = (unsigned int)(s->youngest_birth);
		return 0;
	}
	if (!strcmp(field, "oldest_age")) {
		*ival = s->oldest_age;
		return 0;
	}
	if (!strcmp(field, "logins")) {
		*ival = s->logins;
		return 0;
	}
	if (!strcmp(field, "xsent")) {
		*ival = s->xsent;
		return 0;
	}
	if (!strcmp(field, "xrecv")) {
		*ival = s->xrecv;
		return 0;
	}
	if (!strcmp(field, "esent")) {
		*ival = s->esent;
		return 0;
	}
	if (!strcmp(field, "erecv")) {
		*ival = s->erecv;
		return 0;
	}
	if (!strcmp(field, "fsent")) {
		*ival = s->fsent;
		return 0;
	}
	if (!strcmp(field, "frecv")) {
		*ival = s->frecv;
		return 0;
	}
	if (!strcmp(field, "qsent")) {
		*ival = s->qsent;
		return 0;
	}
	if (!strcmp(field, "qansw")) {
		*ival = s->qansw;
		return 0;
	}
	if (!strcmp(field, "msgs_posted")) {
		*ival = s->msgs_posted;
		return 0;
	}
	if (!strcmp(field, "msgs_read")) {
		*ival = s->msgs_read;
		return 0;
	}
	return -1;
}

int stats_safe_string_field(Stats *s, char *field, char **sval) {
	if (!strcmp(field, "oldest")) {
		*sval = s->oldest;
		return 0;
	}
	if (!strcmp(field, "youngest")) {
		*sval = s->youngest;
		return 0;
	}
	if (!strcmp(field, "most_logins")) {
		*sval = s->most_logins;
		return 0;
	}
	if (!strcmp(field, "most_xsent")) {
		*sval = s->most_xsent;
		return 0;
	}
	if (!strcmp(field, "most_xrecv")) {
		*sval = s->most_xrecv;
		return 0;
	}
	if (!strcmp(field, "most_esent")) {
		*sval = s->most_esent;
		return 0;
	}
	if (!strcmp(field, "most_erecv")) {
		*sval = s->most_erecv;
		return 0;
	}
	if (!strcmp(field, "most_fsent")) {
		*sval = s->most_fsent;
		return 0;
	}
	if (!strcmp(field, "most_frecv")) {
		*sval = s->most_frecv;
		return 0;
	}
	if (!strcmp(field, "most_qsent")) {
		*sval = s->most_qsent;
		return 0;
	}
	if (!strcmp(field, "most_qansw")) {
		*sval = s->most_qansw;
		return 0;
	}
	if (!strcmp(field, "most_posted")) {
		*sval = s->most_posted;
		return 0;
	}
	if (!strcmp(field, "most_read")) {
		*sval = s->most_read;
		return 0;
	}
	return -1;
}

int backend_save_stats(Stats *stats) {
GString sql;
char *p, *endp;
DB_RESULT *res;
int err;
unsigned int ival;
char *sval;

	if (stats->dirty.len <= 0)
		return 0;

	init_GString(&sql);

	gstrcpy(&sql, "UPDATE Stats SET ");

	p = stats->dirty.str;
	while((endp = cstrchr(p, ',')) != NULL) {
		*endp = 0;

		if (!stats_int_field(stats, p, &ival))
			gprint_add(&sql, "%s = %u,", p, ival);
		else
			if (!stats_safe_string_field(stats, p, &sval)) {
				if (sval == NULL)
					gprint_add(&sql, "%s = NULL,", p);
				else
					gprint_add(&sql, "%s = '%s',", p, sval);
			}

		endp++;
		p = endp;
	}
	if (!stats_int_field(stats, p, &ival))
		gprint_add(&sql, "%s = %u", p, ival);
	else
		if (!stats_safe_string_field(stats, p, &sval)) {
			if (sval == NULL)
				gprint_add(&sql, "%s = NULL", p);
			else
				gprint_add(&sql, "%s = '%s'", p, sval);
		}

	if ((res = db_query(sql.str, &err)) != NULL)
		db_free_result(res);

	deinit_GString(&sql);

	if (err)
		return -1;

	reset_GString(&(stats->dirty));
	return 0;
}

int delete_user(User *usr, char *username) {
char sql[MAX_LINE];
DB_RESULT *res;
int err;

	if (username == NULL || !*username)
		return -1;

	if (!(usr->runtime_flags & RTF_SYSOP)) {
		log_err("unauthorized access to delete_user() blocked");
		return -1;
	}
	bufprintf(sql, MAX_LINE, "DELETE FROM Users WHERE name = '%s'", username);

	if ((res = db_query(sql, &err)) != NULL)
		db_free_result(res);

	if (err)
		return -1;

	log_info("Sysop:%s deleted user %s", usr->name, username);
	return 0;
}

/* EOB */
