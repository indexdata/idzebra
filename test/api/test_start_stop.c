/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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
#include <yaz/test.h>
#include "testlib.h"

static void tst(int argc, char **argv)
{
    YAZ_CHECK(!zebra_start("xxxxpoiasdfasfd.cfg")); /* should fail */

    {
        ZebraService zs = tl_start_up(0, argc, argv);
        ZebraHandle  zh = 0;
        YAZ_CHECK(zs);

        if (zs)
        {
            zh = zebra_open(zs, 0);
            YAZ_CHECK(zh);
        }

        YAZ_CHECK(tl_close_down(zh, zs));
    }
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

