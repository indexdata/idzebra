/* $Id: sort1.c,v 1.5 2005-08-09 12:30:47 adam Exp $
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

#include <assert.h>
#include "../api/testlib.h"

int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs);
    zint ids[5];
    char path[256];
    int i;

    zebra_select_database(zh, "Default");

    zebra_init(zh);

    zebra_begin_trans(zh, 1);
    for (i = 1; i <= 4; i++)
    {
        sprintf(path, "%.200s/rec%d.xml", get_srcdir(), i);
        zebra_repository_update(zh, path);
    }
    zebra_end_trans(zh);
    zebra_commit(zh);

    ids[0] = 3;
    ids[1] = 2;
    ids[2] = 4;
    ids[3] = 5;
    do_sort(zh, "@or computer @attr 7=1 @attr 1=30 0", 4, ids);

    ids[0] = 5;
    ids[1] = 4;
    ids[2] = 2;
    ids[3] = 3;
    do_sort(zh, "@or computer @attr 7=1 @attr 1=1021 0", 4, ids);

    ids[0] = 2;
    ids[1] = 5;
    ids[2] = 4;
    ids[3] = 3;
    do_sort(zh, "@or computer @attr 7=1 @attr 1=1021 @attr 4=109 0", 4, ids);

    return close_down(zh, zs, 0);
}
