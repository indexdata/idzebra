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

#if YAZ_HAVE_XML2
#include <libxml/xmlversion.h>
#endif

static void tst(int argc, char **argv)
{
    char path[FILENAME_MAX];
    char record_buf[20000];
    FILE *f;
    size_t r;

    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);

    tl_check_filter(zs, "alvis");

    YAZ_CHECK_EQ(zebra_select_database(zh, "Default"), ZEBRA_OK);

    zebra_init(zh);

    tl_profile_path(zh);

    zebra_set_resource(zh, "recordType", "alvis.marcschema-one.xml");

    yaz_snprintf(path, sizeof(path), "%s/marc-missing-ns.xml", tl_get_srcdir());
    f = fopen(path, "rb");
    YAZ_CHECK(f);
    if (f)
    {
        r = fread(record_buf, 1, sizeof(record_buf)-1, f);
        YAZ_CHECK(r > 0);
        fclose(f);
        YAZ_CHECK(r > 2);
        record_buf[r] = '\0';

#if 0
/* disable this test for now: bug #730 */
/* http://xmlsoft.org/html/libxml-parser.html#xmlReadIO */
#if YAZ_HAVE_XML2
        /* On Mac OSX using Libxml 2.6.16, we xmlTextReaderExpand does
           not return 0 ptr even though the record has an error in it */
#if LIBXML_VERSION >= 20617
        YAZ_CHECK_EQ(zebra_add_record(zh, record_buf, strlen(record_buf)),
                     ZEBRA_FAIL);
#else
        zebra_add_record(zh, record_buf, strlen(record_buf));
#endif
#endif
#endif
    }
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

