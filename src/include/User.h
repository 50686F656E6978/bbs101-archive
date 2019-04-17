/*
	User.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef USER_H_WJ109
#define USER_H_WJ109	1

#include "List.h"
#include "Telnet.h"
#include "IOBuf.h"
#include "Display.h"
#include "defines.h"
#include "PList.h"
#include "XMsg.h"
#include "sys_time.h"
#include "GString.h"

#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#define append_User(x, y)		append_List(&((x)->list), &((y)->list))
#define add_User(x, y)			append_User((x), (y))
#define prepend_User(x, y)		prepend_List(&((x)->list), &((y)->list))
#define unlink_User(x, y)		unlink_List_type((x), (y), User, list)
#define pop_User(x)				pop_List_type((x), User, list)
#define pop0_User(x)			pop0_List_type((x), User, list)

/*
	user config flags
*/
#define USR_HIDE_ADDRESS		1
#define USR_12HRCLOCK			2
#define USR_HELPING_HAND		4
#define USR_X_DISABLED			8
#define USR_BLOCK_FRIENDS		0x10
#define USR_SHORT_WHO			0x20
#define USR_SORT_BYNAME			0x40
#define USR_SORT_DESCENDING		0x80
#define USR_HIDE_ENEMIES		0x100
#define USR_NO_AWAY_REASON		0x200

/*
	runtime user flags
*/
#define RTF_BUSY				1
#define RTF_TIMEOUT_WARNING		2
#define RTF_TIMEOUT_WARNING2	4
#define RTF_WAS_HH				8
#define RTF_SYSOP				0x10
#define RTF_LOCKED				0x20
#define RTF_HOLD				0x40
#define RTF_DEADCONN			0x80000

/*
	timeout() directives
*/
#define RESET_TIMER	0
#define ADD_TIMER	1


typedef struct {
	pthread_t tid;
	pthread_mutex_t mutex;

	char name[MAX_NAME];

	List list;

	IOBuf iobuf;
	Telnet telnet;
	Display display;

	char hostname[NI_MAXHOST];

	int runtime_flags;
	time_t timeout;
	unsigned int online_timer;		/* used for tracking (live) total time online */

	GString recipients, talked_to, dirty, pager_buf;

	PList sent_xmsgs, recv_xmsgs;
	List *seen_xmsgs;

	char default_anon[MAX_NAME], timezone[MAX_HALFLINE];
	char colors[MAX_LINE], symbol_colors[MAX_LINE];

	char *real_name, *city, *state, *country, *email, *www;
	char *doing, *reminder, *vanity, *xmsg_header, *away, *last_from, *info;
	GString quick, friends, enemies, override;

	unsigned int birth, login_time, last_logout, logins;
	unsigned int total_time, last_online_time;
	unsigned int xsent, xrecv, esent, erecv, fsent, frecv, qsent, qansw;
	unsigned int msgs_posted, msgs_read, flags, default_room;
} User;

void init_User(User *);
void deinit_User(User *);
User *new_User(void);
void destroy_User(User *);

void Lock(User *);
void Unlock(User *);
void deepcopy_User(User *, User *);

void connect_User(User *, int);
void close_connection(User *, char *);
void linkdead(User *);

void Flush(User *);
void Put(User *, char *);
void Putc(User *, int);
void Print(User *, char *, ...);
void Write(User *, char *, ...);
int Getch(User *);
int iGetch(User *);

void timeout(User *, int);
int timeout_value(User *);
void wakeup(User *);
int user_dirty(User *, char *);
void save_User_flags(User *, GString *);
void load_User_flags(User *, char *);
void user_color(User *, ColorIndex);
void raw_user_color(User *, int);

#endif	/* USER_H_WJ109 */

/* EOB */
