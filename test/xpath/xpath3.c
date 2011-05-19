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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "../api/testlib.h"

/** xpath3.c - attributes, with Danish characters */

const char *myrec[] = {
    "<root> \n"
    "  <!-- Space in attribute --> \n"
    "  <first attr=\"danish\">content</first> \n"
    "  <second attr=\"danish lake\">content</second> \n"
    "  <!-- Oslash in Latin-1 encoded below.. --> \n"
    "  <third attr=\"dansk s\xf8\">content</third> \n"
    "</root> \n",
    0};


static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);
    YAZ_CHECK(tl_init_data(zh, myrec));

    YAZ_CHECK(tl_query(zh, "@attr 1=/root content",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/root/first content",1));
    YAZ_CHECK(tl_query(zh, "@attr {1=/root/first[@attr='danish']} content",1));
    YAZ_CHECK(tl_query(zh, "@attr {1=/root/second[@attr='danish lake']} content",1));
    YAZ_CHECK(tl_query(zh, "@attr {1=/root/third[@attr='dansk s\xc3\xb8']} content",1)); 
    /* FIXME - This triggers bug200 */

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

