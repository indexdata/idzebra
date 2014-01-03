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

/* insert a small pile of records, search and fetch them */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};

#define NUMBER_TO_FETCH_MAX 1000

static void tst(int argc, char **argv)
{
    int i;
    int number_to_be_inserted = 5;
    int number_to_fetch = 5;

    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    YAZ_CHECK(zebra_begin_trans (zh, 1) == ZEBRA_OK);

    for (i = 0; i< number_to_be_inserted-1; i++)
    {  /* -1 since already inserted one in init_data */
	zebra_add_record(zh, myrec[0], strlen(myrec[0]));
    }
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);

    zebra_close(zh);
    zebra_stop(zs);

    zs = tl_zebra_start("");
    zh = zebra_open(zs, 0);
    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);
    zebra_set_resource(zh, "sortmax", "3"); /* make small sort boundary */

    for (i = 0; i<2; i++)
    {
	ZEBRA_RES ret;
        ZebraRetrievalRecord retrievalRecord[NUMBER_TO_FETCH_MAX];
        char setname[20];
        int j;
        ODR odr_input = odr_createmem(ODR_DECODE);
        ODR odr_output = odr_createmem(ODR_DECODE);
        YAZ_PQF_Parser parser = yaz_pqf_create();
        Z_RPNQuery *query = yaz_pqf_parse(parser, odr_input, "@attr 1=4 my");
        zint hits;

        sprintf(setname, "s%d", i+1);
        ret = zebra_search_RPN(zh, odr_input, query, setname, &hits);
	if (ret != ZEBRA_OK)
	{
	    int code = zebra_errCode(zh);
	    yaz_log(YLOG_WARN, "Unexpected error code=%d", code);
	    exit(1);
	}
	if (hits != number_to_be_inserted)
	{
	    yaz_log(YLOG_WARN, "Unexpected hit count " ZINT_FORMAT
		    "(should be %d)", hits, number_to_be_inserted);
	    exit(1);
	}

        yaz_pqf_destroy(parser);

        odr_destroy(odr_input);

        YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);

	for (j = 0; j < number_to_fetch; j++)
	    retrievalRecord[j].position = j+1;

        ret = zebra_records_retrieve(zh, odr_output, setname, 0,
				     yaz_oid_recsyn_xml, number_to_fetch,
				     retrievalRecord);
	if (ret != ZEBRA_OK)
	{
	    int code = zebra_errCode(zh);
	    yaz_log(YLOG_FATAL, "zebra_records_retrieve returned error %d",
		    code);
	    exit(1);
	}

	for (j = 0; j < number_to_fetch; j++)
	{
	    if (!retrievalRecord[j].buf)
	    {
		yaz_log(YLOG_FATAL, "No record buf at position %d", j);
		exit(1);
	    }
	    if (!retrievalRecord[j].len)
	    {
		yaz_log(YLOG_FATAL, "No record len at position %d", j);
		exit(1);
	    }
	}
        odr_destroy(odr_output);

        YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
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

