/* $Id: t9.c,v 1.1 2004-10-28 10:37:15 heikki Exp $
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

/** t9.c - test rank-1 */

#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>
#include "testlib.h"
#include "rankingrecords.h"

#define qry(zh,query,hits,string,score) \
    RankingQuery(__LINE__,(zh),(query),(hits),(string),(score))

int main(int argc, char **argv)
{
    int i;
    char *addinfo;
    ZebraService zs;
    ZebraHandle zh;

    yaz_log_init_file("t9.log");
    /* yaz_log_init_level(LOG_ALL); */

    nmem_init ();
    
    zs = start_service(""); /* default to zebra.cfg */
    zh = zebra_open (zs);
    zebra_select_database(zh, "Default");
    logf(LOG_LOG,"going to call init");
    i=zebra_init(zh);
    logf(LOG_LOG,"init returned %d",i);
    if (i) {
        printf("init failed with %d\n",i);
        zebra_result(zh, &i, &addinfo);
        printf("  Error %d   %s\n",i,addinfo);
        exit(1);
    }

    zebra_begin_trans (zh, 1);
    for (i = 0; recs[i]; i++)
	zebra_add_record (zh, recs[i], strlen(recs[i]));
    zebra_end_trans (zh);

    zebra_select_database(zh, "Default");

    qry( zh, "@attr 1=1016 @attr 2=102 the",
            3, "first title", 872 );

    qry( zh, "@attr 1=1016 @attr 2=102 foo",
            3, "second title", 850 );

    /* get the record with the most significant hit, that is the 'bar' */
    /* as that is the rarest of my search words */
    qry( zh, "@attr 1=1016 @attr 2=102 @or @or the foo bar",
            3, "third title", 895 );

    
    zebra_commit (zh);
    zebra_close (zh);
    zebra_stop (zs);

    nmem_exit ();
    xmalloc_trav ("x");
    exit (0);
}
