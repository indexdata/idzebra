/* $Id: t6.c,v 1.11 2005-09-13 11:51:07 adam Exp $
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

/** t6.c Insert a number of randomly generated words */

#include "testlib.h"

int main(int argc, char **argv)
{
    int i;
    ZebraService zs = start_up("zebra6.cfg", argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    srand(17);
    
    zebra_select_database(zh, "Default");
    zebra_init(zh);
    zebra_close(zh);

    for (i = 0; i<10; i++)
    {
	int l;

	zh = zebra_open (zs, 0);
	zebra_select_database(zh, "Default");
	
	zebra_begin_trans (zh, 1);

	for (l = 0; l<100; l++)
	{
	    char rec_buf[5120];
	    int j;
	    *rec_buf = '\0';
	    strcat(rec_buf, "<gils><title>");
	    if (i == 0)
	    {
		sprintf(rec_buf + strlen(rec_buf), "aaa");
	    }
	    else
	    {
		j = (rand() & 15) + 1;
		while (--j >= 0)
		{
		    int c = 65 + (rand() & 15);
		    sprintf(rec_buf + strlen(rec_buf), "%c", c);
		}
	    }
	    strcat(rec_buf, "</title><Control-Identifier>");
	    j = rand() & 31;
	    sprintf(rec_buf + strlen(rec_buf), "%d", j);
	    strcat(rec_buf, "</Control-Identifier></gils>");
	    zebra_add_record (zh, rec_buf, strlen(rec_buf));
	}
	zebra_end_trans(zh);
	zebra_close(zh);
    }
    zh = zebra_open(zs, 0);

    zebra_select_database(zh, "Default");

    zebra_set_resource(zh, "trunclimit", "2");

    /* check massive truncation: bug #281 */
    do_query(__LINE__, zh, "@attr 1=4 @attr 2=1 z", -1);

    return close_down(zh, zs, 0);
}
