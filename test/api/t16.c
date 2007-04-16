/* $Id: t16.c,v 1.10 2007-04-16 08:44:32 adam Exp $
   Copyright (C) 1995-2007
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/** \brief test special element set names zebra:: and friends */

#include "testlib.h"

const char *myrec[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};
	
#define NUMBER_TO_FETCH_MAX 1000

static ZEBRA_RES fetch_first(ZebraHandle zh, const char *element_set,
                             const int * format, ODR odr,
                             const char **rec_buf, size_t *rec_len)
{
    ZebraRetrievalRecord retrievalRecord[1];
    Z_RecordComposition *comp;
    ZEBRA_RES res;

    retrievalRecord[0].position = 1; /* get from this position */
    
    yaz_set_esn(&comp, element_set, odr->mem);

    res = zebra_records_retrieve(zh, odr, "default", comp, format, 1, 
                                 retrievalRecord);
    if (res != ZEBRA_OK)
    {
        int code = zebra_errCode(zh);
        yaz_log(YLOG_FATAL, "zebra_records_retrieve returned error %d",
                code);
    }
    else
    {
        *rec_buf = retrievalRecord[0].buf;
        *rec_len = retrievalRecord[0].len;
    }
    return res;
}

static ZEBRA_RES fetch_first_compare(ZebraHandle zh, const char *element_set,
                                     const int *format, const char *cmp_rec)
{
    const char *rec_buf = 0;
    size_t rec_len = 0;
    ODR odr = odr_createmem(ODR_ENCODE);
    ZEBRA_RES res = fetch_first(zh, element_set, format, odr,
                                &rec_buf, &rec_len);
    if (res == ZEBRA_OK)
    {
        if (strlen(cmp_rec) != rec_len)
            res = ZEBRA_FAIL;
        else if (memcmp(cmp_rec, rec_buf, rec_len))
            res = ZEBRA_FAIL;
    }
    odr_destroy(odr);
    return res;
}



static void tst(int argc, char **argv)
{
    zint hits;
    ZEBRA_RES res;
    const char * zebra_xml_sysno 
        = "<record xmlns=\"http://www.indexdata.com/zebra/\" sysno=\"2\"/>\n";

    const char * zebra_xml_meta 
        = "<record xmlns=\"http://www.indexdata.com/zebra/\" sysno=\"2\" base=\"Default\" type=\"grs.sgml\" rank=\"0\" size=\"41\" set=\"zebra::meta\"/>\n";

    const char * zebra_xml_index_title_p
        = "<record xmlns=\"http://www.indexdata.com/zebra/\" sysno=\"2\" set=\"zebra::index::title:p/\">\n"
"  <index name=\"title\" type=\"p\" seq=\"4\">my title</index>\n"
"</record>\n";

    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    res = zebra_search_PQF(zh, "@attr 1=4 my", "default", &hits);
    YAZ_CHECK_EQ(res, ZEBRA_OK);
    YAZ_CHECK_EQ(hits, 1);
    
    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::data", yaz_oid_xml(),
                                     "mismatch"), ZEBRA_FAIL);

    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::data", yaz_oid_sutrs(),
                                     myrec[0]), ZEBRA_OK);

    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::data", yaz_oid_xml(),
                                     myrec[0]), ZEBRA_OK);

    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::meta::sysno", yaz_oid_sutrs(),
                                     "2"), ZEBRA_OK);

    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::meta::sysno", yaz_oid_xml(),
                                     zebra_xml_sysno), ZEBRA_OK);
    
    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::meta", yaz_oid_xml(),
                                     zebra_xml_meta), ZEBRA_OK);
    
    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::index::title:p", 
                                     yaz_oid_xml(),
                                     zebra_xml_index_title_p), ZEBRA_OK);
    
    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::nonexistent", 
                                     yaz_oid_xml(), ""), ZEBRA_OK);
    
    YAZ_CHECK_EQ(fetch_first_compare(zh, "zebra::index::nonexistent", 
                                     yaz_oid_xml(), ""), ZEBRA_OK);
    
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

