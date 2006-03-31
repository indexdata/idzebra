/* $Id: t1.c,v 1.5 2006-03-31 15:58:09 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include "../api/testlib.h"

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    char path[256];
    int i;
    zint ids[5];

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    for (i = 1; i <= 4; i++)
    {
        sprintf(path, "%.200s/rec%d.xml", tl_get_srcdir(), i);
        zebra_repository_update(zh, path);
    }
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);

    ids[0] = 2;
    ids[1] = 5;
    ids[2] = 3;
    ids[3] = 4;
    YAZ_CHECK(tl_sort(zh, "@or computer @attr 7=1 @attr 1=4 0", 4, ids));

    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN
