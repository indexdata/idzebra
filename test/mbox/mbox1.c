/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include "testlib.h"
	
static void tst(int argc, char **argv)
{
    char path[256];

    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);

    tl_check_filter(zs, "grs.regx");
    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    sprintf(path, "%.200s/mail1.mbx", tl_get_srcdir());
    zebra_repository_update(zh, path);

    sprintf(path, "%.200s/mail3.mbx", tl_get_srcdir());
    zebra_repository_update(zh, path);

#if 1
    /* bug #234 */
    sprintf(path, "%.200s/invalid.mbx", tl_get_srcdir());
    zebra_repository_update(zh, path);
#endif

    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);
    
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

