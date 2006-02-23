/* $Id: xslt2.c,v 1.6 2006-02-23 11:26:00 adam Exp $
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

#include <stdio.h>
#include "testlib.h"

int main(int argc, char **argv)
{
    char path[256];
    char record_buf[20000];
    const char *records_array[] = {
	record_buf, 0
    };
    FILE *f;
    size_t r;

    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);

    check_filter(zs, "xslt");

    zebra_select_database(zh, "Default");
    
    zebra_init(zh);

    zebra_set_resource(zh, "recordType", "xslt.marcschema-col.xml");

    sprintf(path, "%.200s/marc-col.xml", get_srcdir());
    f = fopen(path, "rb");
    if (!f)
    {
	yaz_log(YLOG_FATAL|YLOG_ERRNO, "Cannot open %s", path);
	exit(1);
    }
    r = fread(record_buf, 1, sizeof(record_buf)-1, f);
    if (r < 2 || r == sizeof(record_buf)-1)
    {
	yaz_log(YLOG_FATAL, "Bad size of %s", path);
	exit(1);
    }
    fclose(f);

    record_buf[r] = '\0';

    /* for now only the first of the records in the collection is
       indexed. That can be seen as a bug */
    init_data(zh, records_array);

    /* only get hits from first record .. */
    do_query(__LINE__, zh, "@attr 1=title computer", 1);
    do_query(__LINE__, zh, "@attr 1=control 11224466", 1);
    do_query_x(__LINE__, zh, "@attr 1=titl computer", 0, 114);
    
    return close_down(zh, zs, 0);
}
