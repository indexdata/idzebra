/* $Id: grs.marc.c,v 1.1 2005-06-14 20:03:02 adam Exp $
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
    ZebraHandle  zh = zebra_open(zs);
    char path[256];

    check_filter(zs, "grs.marc");

    zebra_select_database(zh, "Default");

    zebra_set_resource(zh, "recordType", "grs.marc.usmarc");

    zebra_init(zh);

    zebra_begin_trans(zh, 1);
    sprintf(path, "%.200s/record.mrc", get_srcdir());
    zebra_repository_update(zh, path);
    zebra_end_trans(zh);
    zebra_commit(zh);

    do_query(__LINE__, zh, "computer", 1);

    do_query(__LINE__, zh, "computee", 0);

    return close_down(zh, zs, 0);
}
