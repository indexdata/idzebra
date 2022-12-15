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

/* Insert a number of randomly generated words and truncate */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "testlib.h"

static void tst(int argc, char **argv)
{
    int i;
    ZebraService zs = tl_start_up("test_trunc.cfg", argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);
    WRBUF rec_buf = wrbuf_alloc();

    srand(17);

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);
    zebra_init(zh);
    zebra_close(zh);

    for (i = 0; i < 10; i++)
    {
        int l;

        zh = zebra_open (zs, 0);
        YAZ_CHECK(zh);

        YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

        YAZ_CHECK(zebra_begin_trans (zh, 1) == ZEBRA_OK);

        for (l = 0; l < 100; l++)
        {
            int j;

            wrbuf_rewind(rec_buf);
            wrbuf_puts(rec_buf, "<gils><title>");
            if (i == 0)
            {
                wrbuf_puts(rec_buf, "aaa");
            }
            else
            {
                j = (rand() & 15) + 1;
                while (--j >= 0)
                {
                    int c = 65 + (rand() & 15);
                    wrbuf_printf(rec_buf, "%c", c);
                }
            }
            wrbuf_puts(rec_buf, "</title><Control-Identifier>");
            j = rand() & 31;
            wrbuf_printf(rec_buf, "%d", j);
            wrbuf_puts(rec_buf, "</Control-Identifier></gils>");
            zebra_add_record (zh, wrbuf_cstr(rec_buf), wrbuf_len(rec_buf));
        }
        YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
        zebra_close(zh);
    }
    zh = zebra_open(zs, 0);
    YAZ_CHECK(zh);

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_set_resource(zh, "trunclimit", "2");

    /* check massive truncation: bug #281 */
    YAZ_CHECK(tl_query(zh, "@attr 1=4 @attr 2=1 z", -1));

    YAZ_CHECK(tl_close_down(zh, zs));
    wrbuf_destroy(rec_buf);
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

