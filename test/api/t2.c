/* $Id: t2.c,v 1.12 2004-08-25 09:23:39 adam Exp $
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
#include <idzebra/api.h>

/* read zebra.cfg from env var srcdir if it exists; otherwise current dir */
static ZebraService start_service()
{
    char cfg[256];
    char *srcdir = getenv("srcdir");
    sprintf(cfg, "%.200s%szebra.cfg", srcdir ? srcdir : "", srcdir ? "/" : "");
    return zebra_start(cfg);
}

int main(int argc, char **argv)
{
    int exit_code = 0;
    int hits;
    ZebraService zs;
    ZebraHandle zh;
    const char *myrec =
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n";

    yaz_log_init_file("t2.log");

    nmem_init ();

    zs = start_service();
    zh = zebra_open (zs);
    zebra_select_database(zh, "Default");
    zebra_init(zh);
    zebra_begin_trans (zh, 1);

    zebra_add_record (zh, myrec, strlen(myrec));

    zebra_search_PQF (zh, "@attr 1=4 my", "set1", &hits);
    if (hits != 1)
    {
        yaz_log(LOG_FATAL, "Expected 1 hit. Got %d", hits);
        exit_code = 1;
    }

    zebra_end_trans (zh);
    zebra_commit (zh);
    zebra_close (zh);
    zebra_stop (zs);

    nmem_exit ();
    xmalloc_trav ("x");
    exit (exit_code);
}
