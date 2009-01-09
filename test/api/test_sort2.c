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

#include "testlib.h"

const char *myrec[] = {
    "<sort2>\n"
    "  <title>first computer</title>\n"
    "</sort2>\n"
    ,
    "<sort2>\n"
    "  <title>second computer</title>\n"
    "</sort2>\n"
    ,
    "<sort2>\n"
    "  <title>A third computer</title>\n"
    "</sort2>\n"
    ,
    "<sort2>\n"
    "  <title>the fourth computer</title>\n"
    "</sort2>\n"
    ,
    0 };

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up("test_sort2.cfg", argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    zint ids[5];

    YAZ_CHECK(tl_init_data(zh, myrec));

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

