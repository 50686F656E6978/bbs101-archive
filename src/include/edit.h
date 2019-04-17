/*
	edit.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef EDIT_H_WJ109
#define EDIT_H_WJ109	1

#include "User.h"

extern char *Wrap_Charset1;
extern char *Wrap_Charset2;

void line_editor(User *, char *, int, int, int *, int *, char);
void text_editor(User *, GString *, char *, char *, int, int, int *, int *, int *, int);
void name_editor(User *, char *, int, int *, int *);
int edit_name(User *, char *);
int edit_tabname(User *, char *);
int edit_line(User *, char *, int);
int edit_wrap_line(User *, char *, int);
int edit_lines(User *, GString *, char *, int);
int upload_lines(User *, GString *, char *, int);
int edit_recipients(User *, int (*)(User *, char *, int));
int add_recipient(User *, char *);
void remove_recipient(User *, char *);
void filter_tablist(User *, GString *, char *, int);
void tab_list(User *, GString *, char **, char *, int, int *, int *);
void backtab_list(User *, GString *, char **, char *, int, int *, int *);
int edit_password(User *, char *, int);
int edit_number(User *, char *);
int edit_text(User *, GString *, char *, int);
int upload_text(User *, GString *, char *, int);

#endif	/* EDIT_H_WJ109 */

/* EOB */
