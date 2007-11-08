/* $Id: t17.c,v 1.3 2007-11-08 07:54:17 adam Exp $
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
    \brief tests ICU enabled maps
*/
#include <yaz/test.h>
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n<title>My title</title>\n</gils>\n",
        "<gils>\n<title>My x title</title>\n</gils>\n",
        "<gils>\n<title>My title x</title>\n</gils>\n" ,
	0} ;
	
static void tst(int argc, char **argv)
{
#if HAVE_ICU
    ZebraService zs = tl_start_up("zebra17.cfg", argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    /* simple term */
    YAZ_CHECK(tl_query(zh, "@attr 1=title notfound", 0));

    /* we should get 3 hits. But 0 for now */
#if 1
    YAZ_CHECK(tl_query(zh, "@attr 1=title title", 3));
#else
    YAZ_CHECK(tl_query(zh, "@attr 1=title title", 0));
#endif

 
    YAZ_CHECK(tl_close_down(zh, zs));
#endif
}

TL_MAIN

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

