/* $Id: safari1.c,v 1.3 2005-01-15 19:38:35 adam Exp $
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

/* safari1 - insert a few Safari records */

#include "testlib.h"

const char *myrec[] = {
    "1234\n"  /* first record */
    "00024338 125060 1 any the\n"
    "00024338 125060 2 any art\n"
    "00024338 125060 3 any of\n",

    "5678\n"  /* other record */
    "00024339 125060 1 any den\n"
    "00024339 125060 2 any gamle\n"
    "00024339 125060 3 any mand\n",

    "5678\n"  /* same record identifier as before .. */
    "00024339 125060 1 any the\n"
    "00024339 125060 2 any gamle\n"
    "00024339 125060 3 any mand\n",

        0};
	
int main(int argc, char **argv)
{
    zint ids[2];
    ZebraService zs = start_up("safari.cfg", argc, argv);
    
    ZebraHandle zh = zebra_open(zs);

    init_data(zh, myrec);
    do_query(__LINE__, zh, "@attr 1=1016 the", 1);
    do_query(__LINE__, zh, "@attr 1=1016 {the art}", 1);
    do_query(__LINE__, zh, "@attr 1=1016 {den gamle}", 1);
    do_query(__LINE__, zh, "@attr 1=1016 {the of}", 0);

    ids[0] = 24338;
    meta_query(__LINE__, zh, "@attr 1=1016 the", 1, ids);
	
    return close_down(zh, zs, 0);
}
