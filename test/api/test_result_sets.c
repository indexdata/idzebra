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

/* Creates a few result sets */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "testlib.h"

const char *myrec[] ={
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};

static void tst(int argc, char **argv)
{
    int i;
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    for (i = 0; i<4; i++)
    {
        char setname[20];
        char *setnamep = setname;
        int status;
        ODR odr_input = odr_createmem(ODR_DECODE);
        ODR odr_output = odr_createmem(ODR_ENCODE);
        YAZ_PQF_Parser parser = yaz_pqf_create();
        Z_RPNQuery *query = yaz_pqf_parse(parser, odr_input,
                                          "@attr 1=4 my");
        zint hits;
        if (zebra_begin_trans(zh, 1) != ZEBRA_OK)
        {
            fprintf(stderr, "zebra_begin_trans failed\n");
            exit(1);
        }
        if (zebra_begin_trans(zh, 0) != ZEBRA_OK)
        {
            fprintf(stderr, "zebra_begin_trans failed\n");
            exit(1);
        }

        yaz_snprintf(setname, sizeof(setname), "s%d", i+1);
        zebra_search_RPN (zh, odr_input, query, setname, &hits);

        if (zebra_end_trans(zh) != ZEBRA_OK)
        {
            fprintf(stderr, "zebra_end_trans failed\n");
            exit(1);
        }
        if (zebra_end_trans(zh) != ZEBRA_OK)
        {
            fprintf(stderr, "zebra_end_trans failed\n");
            exit(1);
        }
        yaz_pqf_destroy(parser);
        zebra_deleteResultSet(zh, Z_DeleteResultSetRequest_list,
                              1, &setnamep, &status);
        odr_destroy(odr_input);
        odr_destroy(odr_output);
    }
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

