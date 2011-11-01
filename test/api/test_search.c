/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/** \file
    \brief test various search attributes */
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <yaz/test.h>
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n<title>My title</title>\n"
        "  <abstract>test record with single coordset, negatives</abstract>\n"
        "  <Spatial-Domain><Bounding-Coordinates>\n"
        "    <West-Bounding-Coordinate> -120 </West-Bounding-Coordinate>\n"
        "    <East-Bounding-Coordinate> -102 </East-Bounding-Coordinate>\n"
        "    <North-Bounding-Coordinate>  49 </North-Bounding-Coordinate>\n"
        "    <South-Bounding-Coordinate>  31 </South-Bounding-Coordinate>\n"
        "  </Bounding-Coordinates></Spatial-Domain>\n"
        "</gils>\n",

        "<gils>\n<title>My x title</title>\n"
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
        "  </Bounding-Coordinates></Spatial-Domain>\n"
        "</gils>\n",

        "<gils>\n<title>My title x</title><abstract>a b c c c a y</abstract>\n</gils>\n" ,
        "<gils>\n<title>test</title><abstract>a1 a2 c a1 a2 a3</abstract>\n</gils>\n" ,

        "<test_search>\n"
        " <date>2107-09-19 00:00:00</date>\n"
        "</test_search>\n"
        ,
        "<gils>\n"
        "<title>"
        "1234567890" "1234567890""1234567890""1234567890""1234567890"
        "1234567890" "1234567890""1234567890""1234567890""1234567890"
        "1234567890" "1234567890""1234567890""1234567890""1234567890"
        "1234567890" "1234567890""1234567890""1234567890""1234567890"
        "1234567890" "1234567890""1234567890""1234567890""1234567890"
        "12345"
        "</title>"
        "</gils>"
        ,
	0} ;
	
static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    /* simple term */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 notfound", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 title", 3));
    YAZ_CHECK_EQ(tl_fetch_compare(zh, 1, "zebra::facet::title:w",
                                  yaz_oid_recsyn_sutrs,
                                  "facet w title\n"
                                  "term 3 3: my\n"
                                  "term 3 3: title\n"
                                  "term 2 2: x\n"), ZEBRA_OK);
    
    YAZ_CHECK_EQ(tl_fetch_compare(zh, 1, "zebra::facet::title:s",
                                  yaz_oid_recsyn_sutrs,
                                  "facet s title\n"
                                  "term 1 1: my title\n"
                                  "term 1 1: my title x\n"
                                  "term 1 1: my x title\n"), ZEBRA_OK);

    /* trunc right */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=1 titl", 3));

    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=1 x", 2));
    YAZ_CHECK_EQ(tl_fetch_compare(zh, 1, "zebra::facet::title:w",
                                  yaz_oid_recsyn_sutrs,
                                  "facet w title\n"
                                  "term 2 2: my\n"
                                  "term 2 2: title\n"
                                  "term 2 2: x\n"), ZEBRA_OK);
                 
    /* trunc left */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=2 titl", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=2 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=2 le", 3));

    /* trunc left&right */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=3 titl", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=3 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=3 le", 3));

    /* trunc none */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=100 titl", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=100 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=100 le", 0));

    /* trunc: process # in term */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=101 titl", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=101 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=101 le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=101 #le", 3));

    /* trunc: re-1 */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=102 titl", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=102 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=102 le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=102 .*le", 3));

    /* trunc: re-2 */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=103 titl", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=103 titlx", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=103 titlxx", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=103 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=103 le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=103 .*le", 3));

    /* trunc: CCL #=. ?=.* (?[0-9] = n times .) */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 titl", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 tit#e", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 ?le", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 ?1le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 ?2le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=104 ?3le", 3));

    /* trunc: * = .*   ! = . and right truncate */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=105 titl", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=105 tit!e", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=105 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=105 le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=105 *le", 3));

    /* trunc: * = .*   ! = . and do not truncate */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=106 titl", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=106 tit!e", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=106 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=106 le", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=106 *le", 3));

    /* string relations, < */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=1 0", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=1 my", 1));

    /* string relations, <= */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=2 my", 4));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=2 mn", 1));

    /* = */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=3 my", 3));
    YAZ_CHECK(
        tl_query(zh, 
                 "@attr 1=4 @attr 2=3 "
                 "1234567890" "1234567890""1234567890""1234567890""1234567890"
                 "1234567890" "1234567890""1234567890""1234567890""1234567890"
                 "1234567890" "1234567890""1234567890""1234567890""1234567890"
                 "1234567890" "1234567890""1234567890""1234567890""1234567890"
                 "1234567890" "1234567890""1234567890""1234567890""1234567890"
                 "12345"
                 , 1));
    
    YAZ_CHECK(
        tl_query_x(zh, 
                   "@attr 1=4 @attr 2=3 "
                   "1234567890" "1234567890""1234567890""1234567890""1234567890"
                   "1234567890" "1234567890""1234567890""1234567890""1234567890"
                   "1234567890" "1234567890""1234567890""1234567890""1234567890"
                   "1234567890" "1234567890""1234567890""1234567890""1234567890"
                   "1234567890" "1234567890""1234567890""1234567890""1234567890"
                   "123456"
                   , 0, 11));
    

    /* string relations, >= */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=4 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=4 tu", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=4 title", 3));

    /* string relations, > */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=5 x", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=5 tu", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=5 title", 2));

    /* always-matches relation */
    YAZ_CHECK(tl_query(zh, "@attr 1=_ALLRECORDS @attr 2=103 {ym}", 6));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=103 {x my}", 5));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=1 @attr 2=103 {x my}", 0, 114));

    /* and searches */
    YAZ_CHECK(tl_query(zh, "@and @attr 1=4 notfound @attr 1=4 x", 0)); 
    YAZ_CHECK(tl_query(zh, "@and @attr 1=4 x @attr 1=4 notfound", 0)); 
    YAZ_CHECK(tl_query(zh, "@and @attr 1=4 notfound @attr 1=4 notfound", 0)); 
    YAZ_CHECK(tl_query(zh, "@and @attr 1=4 x @attr 1=4 x", 2)); 
    YAZ_CHECK(tl_query(zh, "@and @attr 1=4 x @attr 1=4 my", 2)); 
    YAZ_CHECK(tl_query(zh, "@and @attr 1=4 my @attr 1=4 x", 2)); 
    YAZ_CHECK(tl_query(zh, "@and @attr 1=4 my @attr 1=4 my", 3)); 

    /* or searches */
    YAZ_CHECK(tl_query(zh, "@or @attr 1=4 notfound @attr 1=4 x", 2)); 
    YAZ_CHECK(tl_query(zh, "@or @attr 1=4 x @attr 1=4 notfound", 2)); 
    YAZ_CHECK(tl_query(zh, "@or @attr 1=4 notfound @attr 1=4 notfound", 0)); 
    YAZ_CHECK(tl_query(zh, "@or @attr 1=4 x @attr 1=4 x", 2)); 
    YAZ_CHECK(tl_query(zh, "@or @attr 1=4 x @attr 1=4 my", 3)); 
    YAZ_CHECK(tl_query(zh, "@or @attr 1=4 my @attr 1=4 x", 3)); 
    YAZ_CHECK(tl_query(zh, "@or @attr 1=4 my @attr 1=4 my", 3)); 

    /* not searches */
    /* bug 619 */
    YAZ_CHECK(tl_query(zh, "@not @attr 1=4 notfound @attr 1=4 x", 0)); 
    YAZ_CHECK(tl_query(zh, "@not @attr 1=4 x @attr 1=4 x", 0));
    YAZ_CHECK(tl_query(zh, "@not @attr 1=4 my @attr 1=4 x", 1));
    YAZ_CHECK(tl_query(zh, "@not @attr 1=4 my @attr 1=4 notfound", 3));
    YAZ_CHECK(tl_query(zh, "@not @attr 1=4 notfound @attr 1=4 notfound", 0));
    
    /* phrase searches */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 my", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 {my x}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=1 {my x}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 {my x}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 {x my}", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 {my x title}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 {my title}", 2));

    /* and-list searches */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=6 {x my}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=6 {my x}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=6 {my my}", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=6 {e x}", 0));

    /* or-list searches */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=105 {x my}", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=105 {my x}", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=105 {my my}", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=105 {e x}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 4=106 {e x}", 2));

    /* exl=0 distance=2 order=1 relation=2 (<=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @prox 0 2 1 2 k 2 my x", 2));

    /* exl=0 distance=2 order=1 relation=2 (<=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @prox 0 2 1 2 k 2 x my", 0));

    /* exl=0 distance=2 order=0 relation=2 (<=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @prox 0 2 0 2 k 2 x my", 2));

    /* exl=0 distance=2 order=0 relation=3 (=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @prox 0 2 1 3 k 2 my x", 1));

    /* exl=1 distance=2 order=0 relation=3 (=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @prox 1 2 1 3 k 2 my x", 1));
    
    /* exl=0 distance=2 order=1 relation=2 (<=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 2 1 2 k 2 a y", 1));

    /* exl=0 distance=1 order=1 relation=3 (=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 3 k 2 a b", 1));


    /* exl=0 distance=1 order=1 relation=3 (=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 3 k 2 c a", 1));
    /* exl=0 distance=1 order=1 relation=2 (<=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 2 k 2 c a", 1));

    /* exl=0 distance=1 order=1 relation=2 (<=), known, unit=word */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 2 k 2 @prox 0 1 1 2 k 2 a1 a2 a3", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 3 k 2 @prox 0 1 1 3 k 2 a1 a2 a3", 1));

    /* 3 term @prox test.. */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 \"a b c\"", 1));

    /* exl=0 distance=1 order=1 relation=2 (<=), known, unit=word */
    /* right associative (does not work, so zero hits) */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 2 k 2 a @prox 0 1 1 2 k 2 b c", 0));
    /* left associative (works fine) */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 2 k 2 @prox 0 1 1 2 k 2 a b c", 1));

    /* exl=0 distance=1 order=1 relation=3 (=), known, unit=word */
    /* right associative (does not work, so zero hits) */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 3 k 2 a @prox 0 1 1 3 k 2 b c", 0));
    /* left associative (works fine) */
    YAZ_CHECK(tl_query(zh, "@attr 1=1016 @prox 0 1 1 3 k 2 @prox 0 1 1 3 k 2 a b c", 1));

    /* Non-indexed numeric use, but specified in bib1.att (bug #1142) */
    YAZ_CHECK(tl_query_x(zh, "@attr 1=1000 x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=1000 @attr 14=0 x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=1000 @attr 14=1 x", 0, 0));
    /* Non-indexed numeric use and unspecified in bib1.att */
    YAZ_CHECK(tl_query_x(zh, "@attr 1=999 x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=999 @attr 14=1 x", 0, 114));
    /* Non-indexed string use attribute */
    YAZ_CHECK(tl_query_x(zh, "@attr 1=gyf  x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=gyf @attr 14=0 x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=gyf @attr 14=1 x", 0, 0));

    /* provoke unsupported use attribute */
    YAZ_CHECK(tl_query_x(zh, "@attr 1=999 @attr 4=1 x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=999 @attr 4=6 x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=999 @attr 4=105 x", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=999 @attr 4=109 123", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attrset 1.2.840.10003.3.1 @attr 1=999 x",
			 0, 114));
    /* provoke unsupported attribute set */
    YAZ_CHECK(tl_query_x(zh, "@attrset 1.2.8 @attr 1=999 @attr 4=1 x", 0, 121));
    YAZ_CHECK(tl_query_x(zh, "@attrset 1.2.8 @attr 1=999 @attr 4=6 x", 0,
	       121));
    YAZ_CHECK(tl_query_x(zh, "@attrset 1.2.8 @attr 1=999 @attr 4=105 x", 0,
	       121));
    YAZ_CHECK(tl_query_x(zh, "@attrset 1.2.8 @attr 1=999 @attr 4=109 123",
	       0, 121));

    /* provoke unsupported relation */
    YAZ_CHECK(tl_query_x(zh, "@attr 1=4 @attr 2=6 x", 0, 117));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=1016 @attr 2=6 @attr 4=109 x", 0, 114));

    /* position , phrase searches */
    YAZ_CHECK(tl_query(zh, "@attr 3=1 title", 0));
    YAZ_CHECK(tl_query(zh, "@attr 3=1 my", 3));

    YAZ_CHECK(tl_query(zh, "@attr 3=1 {my title}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 4=1 @attr 3=1 {my title}", 2));

    YAZ_CHECK(tl_query(zh, "@attr 3=1 {title my}", 0));
    YAZ_CHECK(tl_query(zh, "@attr 4=1 @attr 3=1 {title my}", 0));

    YAZ_CHECK(tl_query(zh, "@attr 4=1 @attr 3=1 {title my}", 0));

    /* position , or-list */
    YAZ_CHECK(tl_query(zh, "@attr 4=105 @attr 3=1 {title my}", 3));
    YAZ_CHECK(tl_query(zh, "@attr 4=105 @attr 3=1 {title x}", 0));
    
    /* position, and-list */
    YAZ_CHECK(tl_query(zh, "@attr 4=6 @attr 3=1 {title my}", 0));
    YAZ_CHECK(tl_query(zh, "@attr 4=6 @attr 3=1 {title x}", 0));
    YAZ_CHECK(tl_query(zh, "@attr 4=6 @attr 3=1 my", 3));


    /* 1=2038: West-Bounding-Coordinate 2039: East: 2040: North: 2041 South*/
    /* 4=109: numeric string */
    /* 2=3: equal  2=1: less, 2=4: greater or equal 2=5 greater */

    /* N>=25, search attributes work */
    YAZ_CHECK(tl_query(zh,  "@attr 2=4 @attr gils 1=2040 @attr 4=109 25", 2));

    /* N>49, search attributes work */
    YAZ_CHECK(tl_query(zh,  "@attr 2=5 @attr gils 1=2040 @attr 4=109 49", 0));

    /* N>=49, search attributes work */
    YAZ_CHECK(tl_query(zh,  "@attr 2=4 @attr gils 1=2040 @attr 4=109 49", 2));

    /* N>48, search attributes work */
    YAZ_CHECK(tl_query(zh,  "@attr 2=5 @attr gils 1=2040 @attr 4=109 48", 2));

    /* N<48, search attributes work */
    YAZ_CHECK(tl_query(zh,  "@attr 2=1 @attr gils 1=2040 @attr 4=109 48", 1));

    /* N<=48, search attributes work */
    YAZ_CHECK(tl_query(zh,  "@attr 2=2 @attr gils 1=2040 @attr 4=109 48", 1));

    /* N=41, get rec1 only */
    YAZ_CHECK(tl_query(zh,  "@attr 2=3 @attr gils 1=2040 @attr 4=109 41", 1));

    /* N=49, get both records */
    YAZ_CHECK(tl_query(zh,  "@attr 2=3 @attr gils 1=2040 @attr 4=109 49", 2));

    /* W=-120 get both records */
    YAZ_CHECK(tl_query(zh,  "@attr 2=3 @attr gils 1=2038 @attr 4=109 -120", 2));

    /* W<-122 get only rec1 */
    YAZ_CHECK(tl_query(zh,  "@attr 2=1 @attr gils 1=2038 @attr 4=109 '-120' ", 1));

    /* N=41 and N=49 get only rec2 */
    YAZ_CHECK(tl_query(zh, "@attr 2=3 @attr gils 1=2040 @attr 4=109 \"41 49\" ", 1));


    /* = */
    YAZ_CHECK(tl_query(zh, "@attr 1=30 @attr 4=5 @attr 2=3 {2107-09-19 00:00:00}", 1));
    /* < */
    YAZ_CHECK(tl_query(zh, "@attr 1=30 @attr 4=5 @attr 2=1 {2107-09-19 00:00:00}", 0));
    /* <= */
    YAZ_CHECK(tl_query(zh, "@attr 1=30 @attr 4=5 @attr 2=2 {2107-09-19 00:00:00}", 1));
    /* >= */
    YAZ_CHECK(tl_query(zh, "@attr 1=30 @attr 4=5 @attr 2=4 {2107-09-19 00:00:00}", 1));
    /* > */
    YAZ_CHECK(tl_query(zh, "@attr 1=30 @attr 4=5 @attr 2=5 {2107-09-19 00:00:00}", 0));
    

    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

