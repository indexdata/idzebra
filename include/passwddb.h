/* $Id: passwddb.h,v 1.4.2.1 2005-05-30 13:24:53 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#ifndef PASSWDDB_H
#define PASSWDDB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct passwd_db *Passwd_db;

Passwd_db passwd_db_open (void);
int passwd_db_auth (Passwd_db db, const char *user, const char *pass);
int passwd_db_file_plain(Passwd_db db, const char *fname);
int passwd_db_file_crypt(Passwd_db db, const char *fname);
void passwd_db_close (Passwd_db db);
void passwd_db_show (Passwd_db db);

#ifdef __cplusplus
}
#endif

#endif

