/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "testlib.h"

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    char path[256];

    tl_check_filter(zs, "grs.xml");
    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    sprintf(path, "%.200s/m1.xml", tl_get_srcdir());
    zebra_repository_update(zh, path);
    sprintf(path, "%.200s/m2.xml", tl_get_srcdir());
    zebra_repository_update(zh, path);
    sprintf(path, "%.200s/m3.xml", tl_get_srcdir());
    zebra_repository_update(zh, path);
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);
    
    YAZ_CHECK(tl_query(zh, "@and "
	     "@attr 1=54 eng "
	     "@and @attr 1=1003 jack @attr 1=4 computer", 2));

    YAZ_CHECK(tl_query(zh, "@attr 1=leader 00366", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=leader2 nam", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=1003 jack", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=1003 jack", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=1003 collins", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=1003 @attr 3=1 collins", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 3=1 program", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 3=1 to", 0));

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

