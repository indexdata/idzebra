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

/** test resources */
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "testlib.h"
#include <idzebra/api.h>

static void tst_res(void)
{
    Res default_res;  /* default resources */
    Res temp_res;     /* temporary resources */
    ZebraService zs;
    ZebraHandle zh;
    const char *val;
    int i;

    default_res = res_open(0, 0); /* completely empty */
    YAZ_CHECK(default_res);

    res_set(default_res, "name1", "value1");

    temp_res = res_open(0, 0); /* completely empty */
    YAZ_CHECK(temp_res);

    zs = zebra_start_res(0, default_res, temp_res);
    YAZ_CHECK(zs);

    zh = zebra_open(zs, 0);
    YAZ_CHECK(zh);

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    /* run this a few times so we can see leaks easier */
    for (i = 0; i<10; i++)
    {
        /* we should find the name1 from default_res */
        val = zebra_get_resource(zh, "name1", 0);
        YAZ_CHECK(val && !strcmp(val, "value1"));

        /* make local override */
        res_set(temp_res, "name1", "value2");
        res_set(temp_res, "name4", "value4");

        /* we should find the name1 from temp_res */
        val = zebra_get_resource(zh, "name1", 0);
        YAZ_CHECK(val && !strcmp(val, "value2"));

        val = zebra_get_resource(zh, "name4", 0);
        YAZ_CHECK(val && !strcmp(val, "value4"));

        res_clear(temp_res);
    }
    zebra_close(zh);
    zebra_stop(zs);

    res_close(temp_res);
    res_close(default_res);
}

static void tst_no_config(void)
{
    static char *xml_buf = "<gils><title>myx</title></gils>";
    ZebraService zs;
    ZebraHandle zh;
    zint sysno = 0;

    zs = zebra_start_res(0, 0, 0);
    YAZ_CHECK(zs);

    zh = zebra_open(zs, 0);
    YAZ_CHECK(zh);

    YAZ_CHECK_EQ(zebra_select_database(zh, "Default"), ZEBRA_OK);

    YAZ_CHECK_EQ(zebra_init(zh), ZEBRA_OK);

    zebra_set_resource(zh, "profilePath", "${srcdir:-.}/../../tab");

    YAZ_CHECK_EQ(zebra_update_record(zh /* handle */,
                                     action_insert,
                                     "grs.sgml" /* record type */,
                                     &sysno, 0 /* match */,
                                     0 /* fname */,
                                     xml_buf, strlen(xml_buf)),
              ZEBRA_OK);

    zebra_close(zh);
    zebra_stop(zs);
}

static void tst(int argc, char **argv)
{
    tst_res();
    tst_no_config();
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

