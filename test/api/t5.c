/* $Id: t5.c,v 1.2 2004-06-14 23:43:32 adam Exp $
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

#include <yaz/log.h>
#include <yaz/pquery.h>
#include <zebraapi.h>

/* read zebra.cfg from env var srcdir if it exists; otherwise current dir */
static ZebraService start_service()
{
    char cfg[256];
    char *srcdir = getenv("srcdir");
    sprintf(cfg, "%.200s%szebra.cfg", srcdir ? srcdir : "", srcdir ? "/" : "");
    return zebra_start(cfg, 0, 0);
}
	
static void expect(ZebraHandle zh, const char *pqf, int hits_expect,
		   int *exit_code)
{
    int hits;
    if (zebra_search_PQF (zh, pqf, "set1", &hits) != 0)
    {
        yaz_log(LOG_FATAL, "Search %s: failed", pqf);
        *exit_code = 1;
    }
    else if (hits != hits_expect)
    {
        yaz_log(LOG_FATAL, "Search %s: Expected %d, got %d", pqf,
		hits_expect, hits);
        *exit_code = 2;
    }
}

int main(int argc, char **argv)
{
    int i, hits = -1;
    int exit_code = 0;
    ZebraService zs;
    ZebraHandle zh;
    const char *myrec[] = {
        "<gils>\n<title>My title</title>\n</gils>\n",
        "<gils>\n<title>My x title</title>\n</gils>\n",
        "<gils>\n<title>My title x</title>\n</gils>\n" ,
	0}
    ;

    yaz_log_init_file("t5.log");

    nmem_init ();
    
    zs = start_service();
    zh = zebra_open (zs);
    zebra_select_database(zh, "Default");
    zebra_init(zh);

    zebra_begin_trans (zh, 1);
    for (i = 0; myrec[i]; i++)
	zebra_add_record (zh, myrec[i], strlen(myrec[i]));
    zebra_end_trans (zh);

    expect(zh, "@attr 1=4 my", 3, &exit_code);
    expect(zh, "@attr 1=4 {my x}", 1, &exit_code);
    expect(zh, "@attr 1=4 {my x}", 1, &exit_code);
    expect(zh, "@attr 1=4 {x my}", 0, &exit_code);
    expect(zh, "@attr 1=4 {my x title}", 1, &exit_code);
    expect(zh, "@attr 1=4 {my title}", 2, &exit_code);
    expect(zh, "@attr 1=4 @and x title", 2, &exit_code);

    zebra_close (zh);
    zebra_stop (zs);

    nmem_exit ();
    xmalloc_trav ("x");
    exit (exit_code);
}
