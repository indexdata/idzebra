/* $Id: t5.c,v 1.24 2007-12-03 12:57:55 adam Exp $
   Copyright (C) 1995-2007
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/** \file
    \brief test attributes: proximity and position */
#include <yaz/test.h>
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n<title>My title</title>\n</gils>\n",
        "<gils>\n<title>My x title</title>\n</gils>\n",
        "<gils>\n<title>My title x</title>\n</gils>\n" ,
	0} ;
	
static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    /* simple term */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 notfound", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 title", 3));

    /* trunc right */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=1 titl", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 5=1 x", 2));

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
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=1 my", 0));

    /* string relations, <= */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=2 my", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=2 mn", 0));

    /* = */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=3 my", 3));

    /* string relations, >= */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=4 x", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=4 tu", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=4 title", 3));

    /* string relations, > */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=5 x", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=5 tu", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=5 title", 2));

    
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

    /* position */
    YAZ_CHECK(tl_query(zh, "@attr 3=1 my", 3));
    YAZ_CHECK(tl_query(zh, "@attr 3=1 x", 0));
    YAZ_CHECK(tl_query_x(zh, "@attr 3=4 x", 0, 119));
 
    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

