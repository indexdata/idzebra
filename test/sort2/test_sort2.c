/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

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

#include "../api/testlib.h"

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up("test_sort2.cfg", argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    char path[256];
    zint ids[5];

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    sprintf(path, "%.200s/test_sort2_rec.xml", tl_get_srcdir());
    zebra_repository_update(zh, path);

    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);

    ids[0] = 2;
    ids[1] = 5;
    ids[2] = 3;
    ids[3] = 4;
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=1 @attr 1=4 0", 4, ids));

    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

