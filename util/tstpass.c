/* This file is part of the Zebra server.
   Copyright (C) 1994-2010 Index Data

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
#include <yaz/snprintf.h>
#include <stdlib.h>

/* use env srcdir as base directory - or current directory if unset */
const char *get_srcdir(void)
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir || ! *srcdir)
        srcdir=".";
    return srcdir;

}

static void tst(void)
{
    char path[1024];
    Passwd_db db;
    
    db = passwd_db_open();
    YAZ_CHECK(db);
    if (!db)
        return;

    yaz_snprintf(path, sizeof(path), "%s/no_such_file.txt", get_srcdir());
    YAZ_CHECK_EQ(passwd_db_file_plain(db, path), -1);
    YAZ_CHECK_EQ(passwd_db_file_crypt(db, path), -1);
    yaz_snprintf(path, sizeof(path), "%s/tstpass.txt", get_srcdir());
#if HAVE_CRYPT_H
    YAZ_CHECK_EQ(passwd_db_file_crypt(db, path), 0);
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
    YAZ_CHECK_LOG();
    tst();
    YAZ_CHECK_TERM;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

