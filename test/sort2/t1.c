/* $Id: t1.c,v 1.2 2005-01-15 19:38:39 adam Exp $
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
    ZebraMetaRecord *recs;
    char path[256];
    int i, errs = 0;

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

    do_query(__LINE__, zh, "@or computer @attr 7=1 @attr 1=4 0", 4);

    recs = zebra_meta_records_create_range (zh, "rsetname", 1, 4);
    if (!recs)
    {
	fprintf(stderr, "recs==0\n");
	exit(1);
    }
    if (recs[0].sysno != 2)
	errs++;
    if (recs[1].sysno != 5)
	errs++;
    if (recs[2].sysno != 3)
	errs++;
    if (recs[3].sysno != 4)
	errs++;

    zebra_meta_records_destroy (zh, recs, 4);

    if (errs)
    {
	fprintf(stderr, "%d sysnos did not match\n", errs);
	exit(1);
    }

    return close_down(zh, zs, 0);
}
