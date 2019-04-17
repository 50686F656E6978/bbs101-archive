/*
	passwd.c	WJ109

	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	- crypt() is not thread-safe
*/

#include "config.h"
#include "passwd.h"
#include "backend.h"
#include "defines.h"
#include "cstring.h"
#include "log.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <crypt.h>
#include <pthread.h>


static pthread_mutex_t crypt_lock;

static CryptoMech crypto_mechanism = BBS101_CRYPTO;

static char salt_charset[] = "./abdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";


void init_passwd(void) {
	pthread_mutex_init(&crypt_lock, NULL);

	srand(time(NULL));
}


int check_password(char *username, char *passwd) {
char crypted[MAX_CRYPTED], *dollar, *pwd, *salt, saltbuf[3], *verify;

	if (get_crypted_passwd(username, crypted, MAX_CRYPTED) == -1)
		return -1;

	dollar = NULL;

/*
	support for glibc's alternative crypto's
*/
	if (crypted[0] == '$') {
		dollar = cstrrchr(crypted, '$');
		*dollar = 0;
		pwd = dollar+1;
		if (!*pwd)
			return -1;

		salt = crypted;
	} else {
/*
	standard DES 2-character salt
*/
		saltbuf[0] = crypted[0];
		saltbuf[1] = crypted[1];
		saltbuf[2] = 0;
		salt = saltbuf;
	}
	pthread_mutex_lock(&crypt_lock);

	if ((verify = crypt(passwd, salt)) == NULL) {
		pthread_mutex_unlock(&crypt_lock);
		return -1;
	}
	if (salt[0] == '$' && dollar != NULL)
		*dollar = '$';

	if (strcmp(verify, crypted)) {
		pthread_mutex_unlock(&crypt_lock);
		return -1;
	}
	pthread_mutex_unlock(&crypt_lock);
	return 0;
}

void make_salt(char *salt) {
int code, salt_size, l, i, n;

	switch(crypto_mechanism) {
		case CRYPTO_MD5:
			code = 1;
			salt_size = 8;
			break;

		case CRYPTO_SHA256:
			code = 5;
			salt_size = 16;
			break;

		case CRYPTO_SHA512:
			code = 6;
			salt_size = 16;
			break;

		case CRYPTO_DES:
		default:
			salt[0] = salt_charset[rand() % sizeof(salt_charset)];
			salt[1] = salt_charset[rand() % sizeof(salt_charset)];
			salt[2] = 0;
			return;
	}
	l = bufprintf(salt, 4, "$%d$", code);
	salt += l;

	for(i = 0; i < salt_size; i++) {
		n = rand() % sizeof(salt_charset);
		salt[i] = salt_charset[n];
	}
	salt[i] = '$';
}

int crypt_password(char *password, char *crypted, int max) {
char salt[24], *pw;

	make_salt(salt);

	pthread_mutex_lock(&crypt_lock);

	if ((pw = crypt(password, salt)) == NULL) {
		pthread_mutex_unlock(&crypt_lock);
		*crypted = 0;
		return -1;
	}
	cstrcpy(crypted, pw, max);

	pthread_mutex_unlock(&crypt_lock);
	return 0;
}

/* EOB */
