/* $Id: t2.c,v 1.16 2005-01-15 19:38:35 adam Exp $
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

#include "testlib.h"

const char *myrec[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};

int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs);

    init_data(zh, myrec);
    do_query(__LINE__,zh, "@attr 1=4 my", 1);

    return close_down(zh, zs, 0);
}
