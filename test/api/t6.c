/* $Id: t6.c,v 1.1.2.1 2004-08-13 09:55:29 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

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

#include <stdlib.h>

#include <yaz/log.h>
#include <yaz/pquery.h>
#include <zebraapi.h>
/* read zebra.cfg from env var srcdir if it exists; otherwise current dir */
static ZebraService start_service()
{
    char cfg[256];
    char *srcdir = getenv("srcdir");
    sprintf(cfg, "%.200s%szebra6.cfg", srcdir ? srcdir : "", srcdir ? "/" : "");
    return zebra_start(cfg);
}
	
int main(int argc, char **argv)
{
    int i;
    ZebraService zs;
    ZebraHandle zh;

    yaz_log_init_file("t6.log");

    nmem_init ();
    
    srand(17);
    
    zs = start_service();
    zh = zebra_open(zs);
    zebra_select_database(zh, "Default");
    zebra_init(zh);
    zebra_close(zh);

    for (i = 0; i<100; i++)
    {
	char rec_buf[5120];
	int j;

	zh = zebra_open (zs);
	zebra_select_database(zh, "Default");
	
	zebra_begin_trans (zh, 1);

	*rec_buf = '\0';
	strcat(rec_buf, "<gils><title>");
	j = (rand() & 15) + 1;
	while (--j >= 0)
	{
	    int c = 65 + (rand() & 15);
	    sprintf(rec_buf + strlen(rec_buf), "%c ", c);
	}
	strcat(rec_buf, "</title><Control-Identifier>");
	j = rand() & 31;
	sprintf(rec_buf + strlen(rec_buf), "%d", j);
	strcat(rec_buf, "</Control-Identifier></gils>");
	zebra_add_record (zh, rec_buf, strlen(rec_buf));

	zebra_end_trans (zh);
	zebra_close(zh);
    }
    zebra_stop(zs);

    nmem_exit ();
    xmalloc_trav ("x");
    exit (0);
}