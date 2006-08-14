/* $Id: t11.c,v 1.6 2006-08-14 10:40:22 adam Exp $
   Copyright (C) 1995-2006
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

/** testing of scan */
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n<title>a b</title>\n</gils>\n",
        "<gils>\n<title>c d</title>\n</gils>\n",
        "<gils>\n<title>e f</title>\n</gils>\n" ,
	0} ;
	
static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(zh);

    YAZ_CHECK(tl_init_data(zh, myrec));

    if (1)
    {
	/* scan before. nothing must be returned */
	const char *ent[] = { "a", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 1, 1, 1, 1, 0, ent));
    }
    if (1)
    {
	/* scan after. nothing must be returned */
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 m", 1, 1, 1, 0, 1, 0));
    }
    if (1)
    {
	const char *ent[] = { "a", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", 1, 1, 1, 1, 0, ent));
    }
    if (1)
    {
	const char *ent[] = { "b", "c", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 aa", 1, 2, 1, 2, 0, ent));
    }
    if (1)
    {
	const char *ent[] = { "b", "c", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 aa", 1, 2, 1, 2, 0, ent));
    }
    if (1)
    {
	const char *ent[] = { "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 e", 1, 3, 1, 2, 1, ent));
    }
    if (1)
    {
	const char *ent[] = { "c", "d", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -1, 2, -1, 2, 0, ent));
    }
    if (1)
    {
	const char *ent[] = { "d", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -2, 1, -2, 1, 0, ent));
    }
    if (1)
    {
	const char *ent[] = { "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -4, 1, -4, 1, 0, ent));
    }
    if (1)
    {
	const char *ent[] = { "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -5, 1, -5, 0, 1, ent));
    }
    if (1)
    {
	const char *ent[] = { "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -2, 3, -2, 3, 0, ent));
    }
    if (1)
    {
	const char *ent[] = { "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -2, 4, -2, 3, 1, ent));
    }
    if (1)
    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 2, 100, 1, 6, 1, ent));
    }
    if (1)
    {
	const char *ent[] = { "b", "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 0, 100, 0, 5, 1, ent));
    }
    if (1)
    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 10, 100, 1, 6, 1, ent));
    }
    if (1)
    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 22, 10, 1, 0, 1, ent));
    }
    if (1)
    {
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 z", -22, 10, -22, 0, 1, 0));
    }
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

