/*
 * Copyright (C) 1998, Index Data ApS
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: passwddb.h,v $
 * Revision 1.2  1998-06-25 09:55:47  adam
 * Minor changes - fixex headers.
 *
 */

#ifndef PASSWDDB_H
#define PASSWDDB_H

typedef struct passwd_db *Passwd_db;

Passwd_db passwd_db_open (void);
int passwd_db_auth (Passwd_db db, const char *user, const char *pass);
int passwd_db_file (Passwd_db db, const char *fname);
void passwd_db_close (Passwd_db db);
void passwd_db_show (Passwd_db db);

#endif

