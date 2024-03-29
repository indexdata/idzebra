/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

const char *myrec[] = {
    "<sort1>\n"
    "  <title>first computer</title>\n"
    "  <dateTime>2</dateTime>\n"
    "  <cost>2</cost>\n"
    "</sort1>\n"
    ,
    "<sort1>\n"
    "  <title>second computer</title>\n"
    "  <dateTime>1</dateTime>\n"
    "  <cost>21</cost>\n"
    "</sort1>\n"
    ,
    "<sort1>\n"
    "  <title>3rd computer</title>\n"
    "  <dateTime>a^3</dateTime>\n"
    "  <cost>15</cost>\n"
    "</sort1>\n"
    ,
    "<sort1>\n"
    "  <title>fourth computer</title>\n"
    "  <dateTime>4</dateTime>\n"
    "  <cost>11</cost>\n"
    "</sort1>\n"
    ,
    0
};

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up("test_sort1.cfg", argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    zint ids[5];

    YAZ_CHECK(tl_init_data(zh, myrec));

    ids[0] = 3;
    ids[1] = 2;
    ids[2] = 4;
    ids[3] = 5;
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=1 @attr 1=30 0", 4, ids));
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=1 @attr 1=Date 0", 4, ids));

    ids[0] = 5;
    ids[1] = 4;
    ids[2] = 2;
    ids[3] = 3;
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=1 @attr 1=1021 0", 4, ids));
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=1 @attr 1=Bib-Level 0", 4, ids));

    ids[0] = 2;
    ids[1] = 5;
    ids[2] = 4;
    ids[3] = 3;
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=1 @attr 1=1021 @attr 4=109 0", 4, ids));
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=1 @attr 1=Bib-Level @attr 4=109 0", 4, ids));

    ids[0] = 3;
    ids[1] = 4;
    ids[2] = 5;
    ids[3] = 2;
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 computer @attr 7=2 @attr 1=Bib-Level @attr 4=109 0", 4, ids));

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

