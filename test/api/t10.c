/* $Id: t10.c,v 1.3 2004-10-29 13:02:39 heikki Exp $
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

/** t10.c - test zv-rank */

#include "testlib.h"
#include "rankingrecords.h"

#define qry(zh,query,hits,string,score) \
    ranking_query(__LINE__,(zh),(query),(hits),(string),(score))

struct tst {
    char *schema;
    char *hit1;
    int score1;
    char *hit2;
    int score2;
    char *hit3;
    int score3;
};



struct tst tests[] = {
    {"ntc-atn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"ntc-ntn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"ntc-btn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"ntc-apn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"ntc-npn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"ntc-bpn", "first title", 1000, "first title", 1000, "third title", 826 },

    {"atc-atn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"atc-ntn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"atc-btn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"atc-apn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"atc-npn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"atc-bpn", "first title", 1000, "first title", 1000, "first title", 972 },

    {"npc-atn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"npc-ntn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"npc-btn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"npc-apn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"npc-npn", "first title", 1000, "first title", 1000, "third title", 826 },
    {"npc-bpn", "first title", 1000, "first title", 1000, "third title", 826 },

    {"apc-atn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"apc-ntn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"apc-btn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"apc-apn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"apc-npn", "first title", 1000, "first title", 1000, "first title", 972 },
    {"apc-bpn", "first title", 1000, "first title", 1000, "first title", 972 },

    {0,0,0,0,0,0,0},
};

int main(int argc, char **argv)
{
    int i;
    ZebraService zs = start_up("zebrazv.cfg", argc, argv);
    ZebraHandle  zh = zebra_open (zs);
  
    init_data(zh, recs);
    zebra_close(zh);

    yaz_log_init_level(LOG_ALL);
    for (i=0; tests[i].schema; i++)
    {
        zh = zebra_open (zs);
        zebra_select_database(zh, "Default");
        zebra_set_resource(zh, "zvrank.weighting-scheme", tests[i].schema);
        logf(LOG_LOG,"============%d: %s ============", i,tests[i].schema);

        ranking_query( __LINE__, zh, "@attr 1=1016 @attr 2=102 the",
                3, tests[i].hit1, tests[i].score1);
        ranking_query( __LINE__, zh, "@attr 1=1016 @attr 2=102 @or foo bar",
                3, tests[i].hit2, tests[i].score2);
        ranking_query( __LINE__, zh, 
                "@attr 1=1016 @attr 2=102 @or @or the foo bar",
                3, tests[i].hit3, tests[i].score3);

        zebra_close(zh);
    }
    
    return close_down(0,zs,0);
}
