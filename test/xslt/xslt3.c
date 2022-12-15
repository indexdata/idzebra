/* This file is part of the Zebra server.
   Copyright (C) Index Data

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
#include <stdio.h>
#include "../api/testlib.h"

static void tst(int argc, char **argv)
{
    char path[FILENAME_MAX];
    char record_buf[20000];
    const char *records_array[] = {
        record_buf, 0
    };
    FILE *f;
    size_t r;

    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);

    tl_check_filter(zs, "alvis");

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    tl_profile_path(zh);

    zebra_set_resource(zh, "recordType", "alvis.marcschema-one.xml");

    yaz_snprintf(path, sizeof(path), "%s/marc-one.xml", tl_get_srcdir());
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

    /* index this one record */
    YAZ_CHECK(tl_init_data(zh, records_array));

    /* only get hits from first record .. */
    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224466", 1));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=titl computer", 0, 114));

    /* index one more time to see that we don't get dups, since
     index.xsl has a record ID associated with them */
    zebra_add_record(zh, record_buf, strlen(record_buf));

    /* only get hits from first record .. */
    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224466", 1));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=titl computer", 0, 114));

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

