/* $Id: tstpass.c,v 1.2 2007-01-15 15:10:26 adam Exp $
   Copyright (C) 1995-2007
   Index Data ApS

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
#include <yaz/test.h>

static void tst(void)
{
    Passwd_db db;
    
    db = passwd_db_open();
    YAZ_CHECK(db);
    if (!db)
        return;

    YAZ_CHECK_EQ(passwd_db_file_plain(db, "no_such_file.txt"), -1);
    YAZ_CHECK_EQ(passwd_db_file_crypt(db, "no_such_file.txt"), -1);
#if HAVE_CRYPT_H
    YAZ_CHECK_EQ(passwd_db_file_crypt(db, "tstpass.txt"), 0);
    YAZ_CHECK_EQ(passwd_db_auth(db, "other", "x1234"), -1);
    YAZ_CHECK_EQ(passwd_db_auth(db, "admin", "abcd"), -2);
    YAZ_CHECK_EQ(passwd_db_auth(db, "admin", "fruitbat"), 0);
    YAZ_CHECK_EQ(passwd_db_auth(db, "admin", "fruitbatx"), -2);
    YAZ_CHECK_EQ(passwd_db_auth(db, "admin", "fruitba"), -2);
#else
    YAZ_CHECK_EQ(passwd_db_file_plain(db, "passtest.txt"), -1);
#endif
    passwd_db_close(db);
}

int main (int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    tst();
    YAZ_CHECK_TERM;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

