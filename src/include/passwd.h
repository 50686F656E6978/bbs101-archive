/*
	passwd.h	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
*/

#ifndef PASSWD_H_WJ109
#define PASSWD_H_WJ109	1

#define BBS101_CRYPTO	CRYPTO_SHA256

typedef enum {
	CRYPTO_DES = 0,
	CRYPTO_MD5,
	CRYPTO_SHA256,
	CRYPTO_SHA512
} CryptoMech;

void init_passwd(void);
int check_password(char *, char *);
void make_salt(char *);
int crypt_password(char *, char *, int);

#endif	/* PASSWD_H_WJ109 */

/* EOB */
