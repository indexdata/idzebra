/* $Id: t2.c,v 1.21 2006-08-14 10:40:22 adam Exp $
   Copyright (C) 1995-2006
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

#include "testlib.h"

const char *myrec[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};

void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));
    YAZ_CHECK(tl_query(zh, "@attr 1=title my", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 my", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title nope", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 nope", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 2=103 dummy", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=103 dummy", 1));

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

