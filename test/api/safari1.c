/* $Id: safari1.c,v 1.11 2006-05-10 08:13:35 adam Exp $
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

/* safari1 - insert a few Safari records */

#include "testlib.h"

const char *myrec[] = 
{
    "1234\n"  /* ID first record */
    /* chunk owner seq idx term */
    "00024338 125060 1 any the\n"
    "00024338 125060 2 any art\n"
    "00024338 125060 3 any mand\n",

    "5678\n"  /* other record - same owner id */
    "00024339 125060 1 any den\n"
    "00024339 125060 2 any gamle\n"
    "00024339 125060 3 any mand\n",

    "5678\n"  /* same record chunk id as before .. */
    "00024339 125060 1 any the\n"
    "00024339 125060 2 any gamle\n"
    "00024339 125060 3 any mand\n",

    "1000\n"  /* separate record */
    "00024339 125061 1 any the\n"
    "00024339 125061 2 any gamle\n"
    "00024339 125061 3 any mand\n",
    
    "1001\n"  /* separate record */
    "00024340 125062 1 any the\n"
    "00024340 125062 1 any the\n" /* DUP KEY, bug #432 */
    "00024340 125062 2 any old\n"
    "00024340 125062 3 any mand\n",

    0
};

static void tst(int argc, char **argv)
{
    zint ids[3];
    zint limits[3];
    ZebraService zs = tl_start_up("safari.cfg", argc, argv);
    
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=any the", 3));
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=any @and the art", 1));
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=any @and den gamle", 0));
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=any @and the gamle", 1));
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=any @and the of", 0));

    /* verify that we get these records exactly */
    ids[0] = 24338;
    ids[1] = 24339;
    ids[2] = 24340;
    YAZ_CHECK(tl_meta_query(zh, "@attr 4=3 @attr 1=any mand", 3, ids));

    /* limit to 125061 */
    limits[0] = 125061;
    limits[1] = 0;
    zebra_set_limit(zh, 0, limits);
    ids[0] = 24339;
    YAZ_CHECK(tl_meta_query(zh, "@attr 4=3 @attr 1=any mand", 1, ids));

    /* limit to 125060, 125061 */
    limits[0] = 125061;
    limits[1] = 125060;
    limits[2] = 0;
    zebra_set_limit(zh, 0, limits);
    ids[0] = 24338;
    ids[1] = 24339;
    YAZ_CHECK(tl_meta_query(zh, "@attr 4=3 @attr 1=any mand", 2, ids));

    /* all except 125062 */
    limits[0] = 125062;
    limits[1] = 0;
    zebra_set_limit(zh, 1, limits);

    ids[0] = 24338;
    ids[1] = 24339;
    YAZ_CHECK(tl_meta_query(zh, "@attr 4=3 @attr 1=any mand", 2, ids));

    /* no limit */
    zebra_set_limit(zh, 1, 0);
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=any mand", 3));

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

