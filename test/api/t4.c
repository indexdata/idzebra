/* $Id: t4.c,v 1.18 2005-09-13 11:51:07 adam Exp $
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

/* t4 - insert a small pile of records, search and fetch them */

#include "testlib.h"

const char *myrec[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};
	
#define NUMBER_TO_FETCH_MAX 1000

int main(int argc, char **argv)
{
    int i;
    int number_to_be_inserted = 5;
    int number_to_fetch = 5;

    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    init_data(zh, myrec);

    zebra_begin_trans (zh, 1);
    for (i = 0; i< number_to_be_inserted-1; i++)
    {  /* -1 since already inserted one in init_data */
	zebra_add_record(zh, myrec[0], strlen(myrec[0]));
    }
    zebra_end_trans(zh);
    zebra_close(zh);
    zebra_stop(zs);

    zs = start_service("");
    zh = zebra_open(zs, 0);
    zebra_select_database(zh, "Default");
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

        zebra_begin_trans(zh, 1);

	for (j = 0; j < number_to_fetch; j++)
	    retrievalRecord[j].position = j+1;

        ret = zebra_records_retrieve(zh, odr_output, setname, 0,
				     VAL_TEXT_XML, number_to_fetch,
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

        zebra_end_trans(zh);

    }
    zebra_commit(zh);
    return close_down(zh, zs, 0);
}
