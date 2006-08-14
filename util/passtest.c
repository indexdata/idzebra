/* $Id: passtest.c,v 1.4.2.2 2006-08-14 10:39:24 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#include <passwddb.h>

int main (int argc, char **argv)
{
    Passwd_db db;
    
    db = passwd_db_open();
    
    passwd_db_file_plain(db, "/etc/passwd");
    passwd_db_auth(db, "adam", "xtx9Y=");
    passwd_db_close(db);
    return 0;
}
