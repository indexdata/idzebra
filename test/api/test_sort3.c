/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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

/** \file
    \brief sort using various sortindex types
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <yaz/test.h>
#include "testlib.h"

const char *myrec[] = {
    /* 2 */
    "<gils>\n"
    "  <title>My title</title>\n"
    "  <title>X</title>\n"
    "</gils>\n",

    /* 3 */
    "<gils>\n"
    "  <title>My x title</title>\n"
    "  <title>B</title>\n"
    "</gils>\n",

    /* 4 */
    "<gils>\n"
    "  <title>My title x</title>\n"
    "  <title>A</title>\n"
    "</gils>\n" ,
    0} ;

static void tst_sortindex(int argc, char **argv, const char *type)
{
    zint ids[5];
    Res res = res_open(0, 0);

    ZebraService zs = tl_start_up("test_sort3.cfg", argc, argv);
    ZebraHandle  zh;

    res_set(res, "sortindex", type);

    zh = zebra_open(zs, res);

    YAZ_CHECK(tl_init_data(zh, myrec));

    if (strcmp(type, "m"))
    {
        /* i, f only takes first title into consideration */
        ids[0] = 2;
        ids[1] = 4;
        ids[2] = 3;
    }
    else
    {
        /* m takes all titles into consideration */
        ids[0] = 4;
        ids[1] = 3;
        ids[2] = 2;
    }
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 title @attr 7=1 @attr 1=4 0", 3, ids));

    if (strcmp(type, "m"))
    {
        /* i, f only takes first title into consideration */
        ids[0] = 3;
        ids[1] = 4;
        ids[2] = 2;
    }
    else
    {
        /* m takes all titles into consideration */
        ids[0] = 2;
        ids[1] = 3;
        ids[2] = 4;
    }
    YAZ_CHECK(tl_sort(zh, "@or @attr 1=4 title @attr 7=2 @attr 1=4 0", 3, ids));

    YAZ_CHECK(tl_close_down(zh, zs));
}

static void tst(int argc, char **argv)
{
    tst_sortindex(argc, argv, "i");
    tst_sortindex(argc, argv, "f");
    tst_sortindex(argc, argv, "m");
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

