/* $Id: t12.c,v 1.4 2006-05-10 08:13:35 adam Exp $
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

/** Create many databases */

#include "testlib.h"

static void tst(int argc, char **argv)
{
    int i;
    int no_db = 140;
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);
    YAZ_CHECK(zebra_init(zh) == ZEBRA_OK);
    zebra_close(zh);

    zh = zebra_open(zs, 0);
    YAZ_CHECK(zh);
    
    YAZ_CHECK(zebra_begin_trans (zh, 1) == ZEBRA_OK);

    for (i = 0; i<no_db; i++)
    {
	char dbstr[20];
	char rec_buf[100];

	sprintf(dbstr, "%d", i);
	YAZ_CHECK(zebra_select_database(zh, dbstr) == ZEBRA_OK);
    
	sprintf(rec_buf, "<gils><title>title %d</title></gils>\n", i);
	zebra_add_record (zh, rec_buf, strlen(rec_buf));

    }
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);

    zebra_close(zh);
    zh = zebra_open(zs, 0);
    YAZ_CHECK(zh);
    for (i = 0; i<=no_db; i++)
    {
	char dbstr[20];
	char querystr[50];
	sprintf(dbstr, "%d", i);
	YAZ_CHECK(zebra_select_database(zh, dbstr) == ZEBRA_OK);

	sprintf(querystr, "@attr 1=4 %d", i);
	if (i == no_db)
	{
	    YAZ_CHECK(tl_query_x(zh, querystr, 0, 109));
	}
	else
	{
	    YAZ_CHECK(tl_query(zh, querystr, 1));
	}
    }
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

