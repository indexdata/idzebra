/* $Id: xpath1.c,v 1.2 2004-11-19 10:27:16 heikki Exp $
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

#include <stdlib.h>
#include <idzebra/api.h>
#include "testlib.h"

/** xpath1.c - index a simple sgml record and search in it */

int main(int argc, char **argv)
{
    ZebraService zs;
    ZebraHandle zh;
    const char *myrec[] = {
        "<sgml> \n"
        "  before \n"
        "  <tag> \n"
        "    inside \n"
        "  </tag> \n"
        "  after \n"
        "</sgml> \n",
        0};

    zs = start_up("zebraxpath.cfg", argc, argv);
    zh = zebra_open (zs);
    init_data(zh,myrec);

    do_query(__LINE__,zh, "@attr 1=/sgml/tag before", 0);
    do_query(__LINE__,zh, "@attr 1=/sgml/tag inside", 1);
    do_query(__LINE__,zh, "@attr 1=/sgml/tag after", 0);

    do_query(__LINE__,zh, "@attr 1=/sgml/none after", 0);
    do_query(__LINE__,zh, "@attr 1=/sgml/none inside", 0);

    do_query(__LINE__,zh, "@attr 1=/sgml before", 1);
    do_query(__LINE__,zh, "@attr 1=/sgml inside", 1);
    do_query(__LINE__,zh, "@attr 1=/sgml after", 1);

    return close_down(zh, zs, 0);
}
