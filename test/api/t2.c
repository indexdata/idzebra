/* $Id: t2.c,v 1.5 2003-05-20 12:52:50 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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



#include <zebraapi.h>

int main(int argc, char **argv)
{
    int ret, hits;
    ZebraService zs;
    ZebraHandle zh;
    const char *myrec =
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n";
    ODR odr_input, odr_output;

    nmem_init ();

    zs = zebra_start("t2.cfg");
    zh = zebra_open (zs);
    zebra_select_database(zh, "Default");
    zebra_begin_trans (zh, 1);
    zebra_record_insert (zh, myrec, strlen(myrec));

    hits = zebra_search_PQF (zh, "@attr 1=4 my", "set1");
    printf ("hits: %d\n", hits);

    zebra_end_trans (zh);
    zebra_commit (zh);
    zebra_close (zh);
    zebra_stop (zs);

    nmem_exit ();
    xmalloc_trav ("x");
    exit (0);
}
