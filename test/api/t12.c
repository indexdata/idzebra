/* $Id: t12.c,v 1.1 2005-05-09 13:22:44 adam Exp $
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

int main(int argc, char **argv)
{
    int i;
    int no_db = 140;
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs);

    zebra_select_database(zh, "Default");
    zebra_init(zh);
    zebra_close(zh);

    zh = zebra_open (zs);
    
    zebra_begin_trans (zh, 1);
    for (i = 0; i<no_db; i++)
    {
	char dbstr[20];
	char rec_buf[100];

	sprintf(dbstr, "%d", i);
	zebra_select_database(zh, dbstr);
    
	sprintf(rec_buf, "<gils><title>title %d</title></gils>\n", i);
	zebra_add_record (zh, rec_buf, strlen(rec_buf));

    }
    zebra_end_trans(zh);
    zebra_close(zh);
    zh = zebra_open(zs);
    for (i = 0; i<=no_db; i++)
    {
	char dbstr[20];
	char querystr[50];
	sprintf(dbstr, "%d", i);
	zebra_select_database(zh, dbstr);

	sprintf(querystr, "@attr 1=4 %d", i);
	if (i == no_db)
	    do_query_x(__LINE__, zh, querystr, 0, 109);
	else
	    do_query(__LINE__, zh, querystr, 1);
    }

    return close_down(zh, zs, 0);
}
