/* $Id: xslt4.c,v 1.3 2006-02-23 11:26:00 adam Exp $
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

#include "testlib.h"
	
int main(int argc, char **argv)
{
    char path[256];

    ZebraService zs = start_up("zebrastaticrank.cfg", argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);

    check_filter(zs, "xslt");

    zebra_select_database(zh, "Default");

    zebra_init(zh);

    zebra_set_resource(zh, "recordType", "xslt.marcschema-col.xml");
    zebra_set_resource(zh, "staticrank", "1");

    zebra_begin_trans(zh, 1);
    sprintf(path, "%.200s/marc-col.xml", get_srcdir());
    zebra_repository_update(zh, path);

    zebra_end_trans(zh);
    zebra_commit(zh);


    do_query(__LINE__, zh, "@attr 1=title computer", 3);
    do_query(__LINE__, zh, "@attr 1=control 11224466", 1);
    do_query_x(__LINE__, zh, "@attr 1=titl computer", 0, 114);

    if (1)
    {
	zint ids[5];
	ids[0] = 2;
	ids[1] = 4;
	ids[2] = 3;
	do_sort(zh, "@attr 1=title computer", 3, ids);
    }
    return close_down(zh, zs, 0);
}
