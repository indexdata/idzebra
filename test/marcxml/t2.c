/* $Id: t2.c,v 1.3 2005-01-15 19:38:37 adam Exp $
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

int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs);
    char path[256];

    zebra_select_database(zh, "Default");

    zebra_init(zh);

    zebra_set_resource(zh, "recordType", "grs.marcxml.record");
    
    zebra_begin_trans(zh, 1);

    sprintf(path, "%.200s/sample-marc", get_srcdir());
    zebra_repository_update(zh, path);
    zebra_end_trans(zh);
    zebra_commit(zh);

    do_query(__LINE__,zh, "@and @attr 1=1003 jack @attr 1=4 computer", 2);

    return close_down(zh, zs, 0);
}
