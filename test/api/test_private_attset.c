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

const char *myrec[] ={
        "<private_attset>\n"
        "  <title>My title</title>\n"
        "</private_attset>\n",
        0};

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up("test_private_attset.cfg", argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    zebra_commit(zh);

    // string attributes in search
    YAZ_CHECK(tl_query(zh, "@attr 1=title my", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title my", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title titlex", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=extra_title my", 1));

    // numeric attributes with Bib-1 should produce an error
    YAZ_CHECK(tl_query_x(zh,
                         "@attr 1=4 my", 0, 121));
    YAZ_CHECK(tl_query_x(zh,
                         "@attr 1=7 my", 0, 121));
    // private OID with incorrect use attribute
    YAZ_CHECK(tl_query_x(zh,
                         "@attr 1.2.840.10003.3.1000.1000.1 1=4 my", 0, 114));
    // private OID with OK use attribute
    YAZ_CHECK(tl_query(zh,
                       "@attr 1.2.840.10003.3.1000.1000.1 1=7 my", 1));

    YAZ_CHECK(tl_query(zh,
                       "@attr 1.2.840.10003.3.1000.1000.1 1=8 my", 1));

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

