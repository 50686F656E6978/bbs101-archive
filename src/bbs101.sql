/*
	bbs101.sql
	
	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>

	MySQL code for creating the database
*/

CREATE USER bbs101 IDENTIFIED BY 'bbs1oh1';

create database bbs101 default character set = latin1;

GRANT SELECT,INSERT,UPDATE,DELETE,CREATE,DROP,ALTER,INDEX,LOCK TABLES ON bbs101.*
  TO 'bbs101'@'localhost' IDENTIFIED BY 'bbs1oh1';

use bbs101;

CREATE TABLE Users ( id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name CHAR(18) UNIQUE,
  password VARCHAR(255),
  real_name VARCHAR(80),
  city VARCHAR(80),
  state VARCHAR(80),
  country VARCHAR(80),
  email VARCHAR(80),
  www VARCHAR(255),
  doing VARCHAR(80),
  reminder VARCHAR(80),
  default_anon CHAR(18),
  timezone VARCHAR(40),
  vanity VARCHAR(80),
  xmsg_header VARCHAR(80),
  last_from VARCHAR(80),
  birth INT UNSIGNED,
  login_time INT UNSIGNED,
  last_logout INT UNSIGNED,
  logins INT UNSIGNED,
  total_time INT UNSIGNED,
  last_online_time INT UNSIGNED,
  xsent INT UNSIGNED,
  xrecv INT UNSIGNED,
  esent INT UNSIGNED,
  erecv INT UNSIGNED,
  fsent INT UNSIGNED,
  frecv INT UNSIGNED,
  qsent INT UNSIGNED,
  qansw INT UNSIGNED,
  msgs_posted INT UNSIGNED,
  msgs_read INT UNSIGNED,
  flags VARCHAR(255),
  default_room INT UNSIGNED,
  colors VARCHAR(80),
  symbol_colors VARCHAR(80),
  quick VARCHAR(190),
  friends TEXT,
  enemies TEXT,
  info TEXT,
  display_flags VARCHAR(255),
  display_height INT UNSIGNED,
  display_width INT UNSIGNED
);

CREATE UNIQUE INDEX Users_byname ON Users (name);

CREATE TABLE Sysops ( id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name CHAR(18) UNIQUE
);

CREATE UNIQUE INDEX Sysops_byname ON Sysops (name);

CREATE TABLE Stats (
  oldest CHAR(18),
  youngest CHAR(18),
  most_logins CHAR(18),
  most_xsent CHAR(18),
  most_xrecv CHAR(18),
  most_esent CHAR(18),
  most_erecv CHAR(18),
  most_fsent CHAR(18),
  most_frecv CHAR(18),
  most_qsent CHAR(18),
  most_qansw CHAR(18),
  most_posted CHAR(18),
  most_read CHAR(18),
  oldest_birth INT UNSIGNED,
  youngest_birth INT UNSIGNED,
  oldest_age INT UNSIGNED,
  logins INT UNSIGNED,
  xsent INT UNSIGNED,
  xrecv INT UNSIGNED,
  esent INT UNSIGNED,
  erecv INT UNSIGNED,
  fsent INT UNSIGNED,
  frecv INT UNSIGNED,
  qsent INT UNSIGNED,
  qansw INT UNSIGNED,
  msgs_posted INT UNSIGNED,
  msgs_read INT UNSIGNED
);

INSERT INTO Stats VALUES (
  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0
);

/*
	EOB
*/
