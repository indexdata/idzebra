/* $Id: xslt1.c,v 1.7 2006-03-31 15:58:10 adam Exp $
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

#include <yaz/test.h>
#include "testlib.h"

void tst(int argc, char **argv)
{
    char path[256];

    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);

    tl_check_filter(zs, "xslt");

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    zebra_set_resource(zh, "recordType", "xslt.marcschema-col.xml");

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    sprintf(path, "%.200s/marc-col.xml", tl_get_srcdir());
    YAZ_CHECK(zebra_repository_update(zh, path) == ZEBRA_OK);

    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);

    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224466", 1));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=titl computer", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=4 computer", 0, 121));
    
    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN

