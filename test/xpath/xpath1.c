/* $Id: xpath1.c,v 1.7 2006-06-22 15:07:21 adam Exp $
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

/** xpath1.c - index a simple sgml record and search in it */

static void tst(int argc, char **argv)
{
    ZebraService zs;
    ZebraHandle zh;
    const char *myrec[] = {
        "<sgml> \n"
        "  before \n"
        "  <tag x='v'> \n"
        "    inside it\n"
        "  </tag> \n"
        "  after \n"
        "</sgml> \n",
        0};

    zs = tl_start_up(0, argc, argv);
    zh = zebra_open(zs, 0);
    YAZ_CHECK(tl_init_data(zh, myrec));

    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag before", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag inside", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag {inside it}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag after", 0));

    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/none after", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/none inside", 0));

    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml before", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml inside", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml after", 1));

    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag/@x v", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag/@x no", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag/@y v", 0));

    YAZ_CHECK(tl_query(zh, "@attr 1=_XPATH_BEGIN @attr 4=3 tag/sgml/", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=_XPATH_BEGIN @attr 4=3 sgml/", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=_XPATH_BEGIN @attr 4=3 tag/", 0));

    /* bug #617 */
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag @attr 2=103 dummy", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml @attr 2=103 dummy", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/tag @attr 2=103 dummy", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag/@x @attr 2=103 dummy", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/sgml/tag/@y @attr 2=103 dummy", 0));

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

