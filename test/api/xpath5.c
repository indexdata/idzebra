

/* $Id: xpath5.c,v 1.2 2004-11-01 16:09:05 heikki Exp $
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

#include "testlib.h"


/** xpath4.c - Ranking in xpath */


const char *recs[] = {
        "<record>\n"
        "  <title>The first title</title>\n"
        "  <abstract> \n"
        "    The first common word is the: the the the \n"
        "    The second common word is word \n"
        "    but all have the foo bar \n"
        "  </abstract>\n"
        "</record>\n",

        "<record>\n"
        "  <title>The second title</title>\n"
        "  <abstract> \n"
        "    The first common word is the: the \n"
        "    The second common word is foo: foo foo \n"
        "    but all have the foo bar \n"
        "  </abstract>\n"
        "</record>\n",

        "<record>\n"
        "  <title>The third title</title>\n"
        "  <abstract> \n"
        "    The first common word is the: the \n"
        "    The third common word is bar: bar \n"
        "    but all have the foo bar \n"
        "  </abstract>\n"
        "</record>\n",
    
        0 };


int main(int argc, char **argv)
{
    ZebraService zs = start_up("zebraxpath.cfg", argc, argv);
    ZebraHandle zh = zebra_open (zs);
    init_data(zh,recs);

//     yaz_log_init_level(LOG_ALL);

#define q(qry,hits,string,score) \
    ranking_query(__LINE__,zh,qry,hits,string,score)

    q("@attr 1=/record/title @attr 2=102 the",
            3,"first title",846);
    q("@attr 1=/ @attr 2=102 @or third foo",
            3,"third title",802);

    q("@attr 1=/ @attr 2=102 foo",
            3,"second title",850);

#if 0 
    /* FIXME - This query triggers bug #202 */
    /* http://bugs.index/show_bug.cgi?id=202 */
    /* it seems that the ranking does not see all occurrences */
    /* of 'foo', and thus ranks all records as high. */
    q("@attr 1=/record/ @attr 2=102 foo",
            3,"second title",846);
#endif

    return close_down(zh, zs, 0);
}