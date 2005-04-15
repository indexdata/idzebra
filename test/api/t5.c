/* $Id: t5.c,v 1.12 2005-04-15 10:47:49 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

/** t5.c: proximity searches */
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n<title>My title</title>\n</gils>\n",
        "<gils>\n<title>My x title</title>\n</gils>\n",
        "<gils>\n<title>My title x</title>\n</gils>\n" ,
	0} ;
	

int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs);

    init_data(zh, myrec);

    /* phrase searches */
    do_query(__LINE__, zh, "@attr 1=4 my", 3);
    do_query(__LINE__, zh, "@attr 1=4 {my x}", 1);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=1 {my x}", 1);
    do_query(__LINE__, zh, "@attr 1=4 {my x}", 1);
    do_query(__LINE__, zh, "@attr 1=4 {x my}", 0);
    do_query(__LINE__, zh, "@attr 1=4 {my x title}", 1);
    do_query(__LINE__, zh, "@attr 1=4 {my title}", 2);

    /* and-list searches */
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=6 {x my}", 2);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=6 {my x}", 2);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=6 {my my}", 3);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=6 {e x}", 0);

    /* or-list searches */
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=105 {x my}", 3);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=105 {my x}", 3);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=105 {my my}", 3);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=105 {e x}", 2);
    do_query(__LINE__, zh, "@attr 1=4 @attr 4=106 {e x}", 2);

    do_query(__LINE__, zh, "@attr 1=4 @and x title", 2);

    /* exl=0 distance=2 order=1 relation=2 (<=), known, unit=word */
    do_query(__LINE__, zh, "@prox 0 2 1 2 k 2 my x", 2);

    /* exl=0 distance=2 order=1 relation=2 (<=), known, unit=word */
    do_query(__LINE__, zh, "@prox 0 2 1 2 k 2 x my", 0);

    /* exl=0 distance=2 order=0 relation=2 (<=), known, unit=word */
    do_query(__LINE__, zh, "@prox 0 2 0 2 k 2 x my", 2);

    /* exl=0 distance=2 order=0 relation=3 (=), known, unit=word */
    do_query(__LINE__, zh, "@prox 0 2 1 3 k 2 my x", 1);

    /* exl=1 distance=2 order=0 relation=3 (=), known, unit=word */
    do_query(__LINE__, zh, "@prox 1 2 1 3 k 2 my x", 1);

    /* provoke unsupported use attribute */
    do_query_x(__LINE__, zh, "@attr 1=999 @attr 4=1 x", 0, 114);
    do_query_x(__LINE__, zh, "@attr 1=999 @attr 4=6 x", 0, 114);
    do_query_x(__LINE__, zh, "@attr 1=999 @attr 4=105 x", 0, 114);
    do_query_x(__LINE__, zh, "@attr 1=999 @attr 4=109 123", 0, 114);
    do_query_x(__LINE__, zh, "@attrset 1.2.840.10003.3.1 @attr 1=999 x",
	       0, 114);
    /* provoke unsupported attribute set */
    do_query_x(__LINE__, zh, "@attrset 1.2.8 @attr 1=999 @attr 4=1 x", 0, 121);
    do_query_x(__LINE__, zh, "@attrset 1.2.8 @attr 1=999 @attr 4=6 x", 0,
	       121);
    do_query_x(__LINE__, zh, "@attrset 1.2.8 @attr 1=999 @attr 4=105 x", 0,
	       121);
    do_query_x(__LINE__, zh, "@attrset 1.2.8 @attr 1=999 @attr 4=109 123",
	       0, 121);

    /* provoke unsupported relation */
    do_query_x(__LINE__, zh, "@attr 1=1016 @attr 2=6 x", 0, 117);
    do_query_x(__LINE__, zh, "@attr 1=1016 @attr 2=6 @attr 4=109 x", 0, 117);
 
    return close_down(zh, zs, 0);
}
