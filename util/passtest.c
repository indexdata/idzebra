/* $Id: passtest.c,v 1.4 2002-08-02 19:26:57 adam Exp $
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



#include <passwddb.h>

int main (int argc, char **argv)
{
	Passwd_db db;

	db = passwd_db_open();

	passwd_db_file (db, "/etc/passwd");
	passwd_db_auth (db, "adam", "xtx9Y=");
	passwd_db_close (db);
	return 0;
}
