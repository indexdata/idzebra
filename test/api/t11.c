/* $Id: t11.c,v 1.2 2005-05-02 09:25:12 adam Exp $
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

/** testing of scan */
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n<title>a b</title>\n</gils>\n",
        "<gils>\n<title>c d</title>\n</gils>\n",
        "<gils>\n<title>e f</title>\n</gils>\n" ,
	0} ;
	
int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs);

    init_data(zh, myrec);

    if (1)
    {
	/* scan before. nothing must be returned */
	const char *ent[] = { "a", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 0", 1, 1, 1, 1, 0, ent);
    }
    if (1)
    {
	/* scan after. nothing must be returned */
	do_scan(__LINE__, zh, "@attr 1=4 m", 1, 1, 1, 0, 1, 0);
    }
    if (1)
    {
	const char *ent[] = { "a", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 a", 1, 1, 1, 1, 0, ent);
    }
    if (1)
    {
	const char *ent[] = { "b", "c", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 aa", 1, 2, 1, 2, 0, ent);
    }
    if (1)
    {
	const char *ent[] = { "b", "c", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 aa", 1, 2, 1, 2, 0, ent);
    }
    if (1)
    {
	const char *ent[] = { "e", "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 e", 1, 3, 1, 2, 1, ent);
    }
    if (1)
    {
	const char *ent[] = { "c", "d", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 a", -1, 2, -1, 2, 0, ent);
    }
    if (1)
    {
	const char *ent[] = { "d", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 a", -2, 1, -2, 1, 0, ent);
    }
    if (1)
    {
	const char *ent[] = { "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 a", -4, 1, -4, 1, 0, ent);
    }
    if (1)
    {
	const char *ent[] = { "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 a", -5, 1, -5, 0, 1, ent);
    }
    if (1)
    {
	const char *ent[] = { "d", "e", "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 a", -2, 3, -2, 3, 0, ent);
    }
    if (1)
    {
	const char *ent[] = { "d", "e", "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 a", -2, 4, -2, 3, 1, ent);
    }
    if (1)
    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 0", 2, 100, 1, 6, 1, ent);
    }
    if (1)
    {
	const char *ent[] = { "b", "c", "d", "e", "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 0", 0, 100, 0, 5, 1, ent);
    }
    if (1)
    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 0", 10, 100, 1, 6, 1, ent);
    }
    if (1)
    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	do_scan(__LINE__, zh, "@attr 1=4 0", 22, 10, 1, 0, 1, ent);
    }
    if (1)
    {
	do_scan(__LINE__, zh, "@attr 1=4 z", -22, 10, -22, 0, 1, 0);
    }
    return close_down(zh, zs, 0);
}
