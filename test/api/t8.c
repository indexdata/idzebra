/* $Id: t8.c,v 1.4 2004-10-28 15:24:36 heikki Exp $
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

/* t8: test numeric attributes */

#include <assert.h>
#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>
#include "testlib.h"

#define LOGLEVEL LOG_ALL 

	
const char *recs[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "  <abstract>test record with single coordset, negatives</abstract>\n"
        "  <Spatial-Domain><Bounding-Coordinates>\n"
        "    <West-Bounding-Coordinate> -120 </West-Bounding-Coordinate>\n"
        "    <East-Bounding-Coordinate> -102 </East-Bounding-Coordinate>\n"
        "    <North-Bounding-Coordinate>  49 </North-Bounding-Coordinate>\n"
        "    <South-Bounding-Coordinate>  31 </South-Bounding-Coordinate>\n"
        "  </Bounding-Coordinates></Spatial-Domain>"
        "</gils>\n",

        "<gils>\n"
        "  <title>Another title</title>\n"
        "  <abstract>second test with two coord sets</abstract>\n"
        "  <Spatial-Domain><Bounding-Coordinates>\n"
        "    <West-Bounding-Coordinate> -120 </West-Bounding-Coordinate>\n"
        "    <East-Bounding-Coordinate> -102 </East-Bounding-Coordinate>\n"
        "    <North-Bounding-Coordinate>  49 </North-Bounding-Coordinate>\n"
        "    <South-Bounding-Coordinate>  31 </South-Bounding-Coordinate>\n"
        "  </Bounding-Coordinates>"
        "  <Bounding-Coordinates>\n"
        "    <West-Bounding-Coordinate> -125 </West-Bounding-Coordinate>\n"
        "    <East-Bounding-Coordinate> -108 </East-Bounding-Coordinate>\n"
        "    <North-Bounding-Coordinate>  41 </North-Bounding-Coordinate>\n"
        "    <South-Bounding-Coordinate>  25 </South-Bounding-Coordinate>\n"
        "  </Bounding-Coordinates></Spatial-Domain>"
        "</gils>\n",
        0};
        



int main(int argc, char **argv)
{
    ZebraService zs;
    ZebraHandle zh;
    yaz_log_init_file("t8.log");
#ifdef LOGLEVEL
    yaz_log_init_level(LOGLEVEL); 
#endif

    nmem_init ();
    
    zs = start_service("zebra8.cfg");
    zh = zebra_open (zs);

    init_data(zh, recs);

#define Q(q,n) Query(__LINE__,zh,q,n)
    /* couple of simple queries just to see that we have indexed the stuff */
    Q( "@attr 1=4 title",2 );
    Q( "title",2 );
    
    /* 1=2038: West-Bounding-Coordinate 2039: East: 2040: North: 2041 South*/
    /* 4=109: numeric string */
    /* 2=3: equal  2=1: less, 2=4: greater or equal 2=5 greater */

    /* N>25, search attributes work */
    Q( "@attr 2=4 @attr gils 1=2040 @attr 4=109 25",2);

    /* N=41, get rec1 only */
    Q( "@attr 2=3 @attr gils 1=2040 @attr 4=109 41",1);

    /* N=49, get both records */
    Q( "@attr 2=3 @attr gils 1=2040 @attr 4=109 49",2);

    /* W=-120 get both records */
    Q( "@attr 2=3 @attr gils 1=2038 @attr 4=109 -120",2);

    /* W<-122 get only rec1 */
    Q( "@attr 2=1 @attr gils 1=2038 @attr 4=109 '-120' ",1);

    /* N=41 and N=49 get only rec2 */
    Q( "@attr 2=3 @attr gils 1=2040 @attr 4=109 \"41 49\" ",1);

    zebra_commit (zh);
    zebra_close (zh);
    zebra_stop (zs);

    nmem_exit ();
    xmalloc_trav ("x");
    logf(LOG_LOG,"All tests OK");
    exit (0);
}
