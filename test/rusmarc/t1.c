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

#include <yaz/yaz-iconv.h>

#include "../api/testlib.h"

void check_koi8r(void)
{
    yaz_iconv_t cd = yaz_iconv_open("koi8-r", "utf-8");
    if (!cd)
    {
	yaz_log(YLOG_WARN, "koi8-r to utf-8 unsupported");
	exit(0);
    }
    yaz_iconv_close(cd);
}

static void tst(int argc, char **argv)
{
    
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    char path[256];

    check_koi8r();

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    sprintf(path, "%.200s/records/simple-rusmarc", tl_get_srcdir());
    zebra_repository_update(zh, path);
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);

    YAZ_CHECK(tl_query(zh, "@attr 1=21 \xfa\xc1\xcd\xd1\xd4\xc9\xce", 1));

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

