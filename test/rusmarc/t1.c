/* $Id: t1.c,v 1.2 2004-12-14 10:53:49 adam Exp $
   Copyright (C) 2003,2004
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

#include <yaz/yaz-iconv.h>

#include "../api/testlib.h"

void check_koi8r()
{
    yaz_iconv_t cd = yaz_iconv_open("koi8-r", "utf-8");
    if (!cd)
    {
	yaz_log(YLOG_WARN, "koi8-r to utf-8 unsupported");
	exit(0);
    }
    yaz_iconv_close(cd);
}

int main(int argc, char **argv)
{
    
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs);
    char path[256];

    check_koi8r();

    zebra_select_database(zh, "Default");

    zebra_init(zh);

    
    zebra_begin_trans(zh, 1);
    sprintf(path, "%.200s/records/simple-rusmarc", get_srcdir());
    zebra_repository_update(zh, path);
    zebra_end_trans(zh);
    zebra_commit(zh);

    do_query(__LINE__, zh, "@attr 1=21 \xfa\xc1\xcd\xd1\xd4\xc9\xce", 1);

    return close_down(zh, zs, 0);
}
