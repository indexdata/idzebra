/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include <yaz/test.h>
#include "../api/testlib.h"

void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    char path[256];

    tl_check_filter(zs, "grs.xml");

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_set_resource(zh, "recordType", "grs.xml");

    zebra_init(zh);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    sprintf(path, "%.200s/record-idzebra.xml", tl_get_srcdir());
    zebra_repository_update(zh, path);
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);

    YAZ_CHECK(tl_query(zh, "@attr 1=/ text", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/ notexistent", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/ 1198", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/ 18", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/elem/idzebra/size 1198", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/elem/idzebra/localnumber 18", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/elem/idzebra/filename biblio", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/elem/idzebra/filename data", 0));

    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN

/*
 * Local variables: 
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

