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

/** \brief test special element set names zebra:: and friends */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};

static void tst(int argc, char **argv)
{
    zint hits;
    ZEBRA_RES res;
    const char * zebra_xml_sysno
        = "<record xmlns=\"http://www.indexdata.com/zebra/\" sysno=\"2\"/>\n";

    const char * zebra_xml_meta
        = "<record xmlns=\"http://www.indexdata.com/zebra/\" sysno=\"2\" base=\"Default\" type=\"grs.sgml\" rank=\"0\" size=\"41\" set=\"zebra::meta\"/>\n";

    const char * zebra_xml_index_title_p
        = "<record xmlns=\"http://www.indexdata.com/zebra/\" sysno=\"2\" set=\"zebra::index::title:p\">\n"
"  <index name=\"title\" type=\"p\" seq=\"4\">My title</index>\n"
"</record>\n";

    const char * zebra_xml_index_title_s
        = "<record xmlns=\"http://www.indexdata.com/zebra/\" sysno=\"2\" set=\"zebra::index::title:s\">\n"
"  <index name=\"title\" type=\"s\">my title</index>\n"
"</record>\n";

    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(tl_init_data(zh, myrec));

    res = zebra_search_PQF(zh, "@attr 1=4 my", "rsetname", &hits);
    YAZ_CHECK_EQ(res, ZEBRA_OK);
    YAZ_CHECK_EQ(hits, 1);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::data", yaz_oid_recsyn_xml,
                                        "mismatch"), ZEBRA_FAIL);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::data", yaz_oid_recsyn_sutrs,
                                        myrec[0]), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::data", yaz_oid_recsyn_xml,
                                        myrec[0]), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::meta::sysno",
                                        yaz_oid_recsyn_sutrs,
                                        "2"), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::meta::sysno",
                                        yaz_oid_recsyn_xml,
                                        zebra_xml_sysno), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::meta", yaz_oid_recsyn_xml,
                                        zebra_xml_meta), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::index::title:p",
                                        yaz_oid_recsyn_xml,
                                        zebra_xml_index_title_p), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::index::title:s",
                                        yaz_oid_recsyn_xml,
                                        zebra_xml_index_title_s), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::nonexistent",
                                        yaz_oid_recsyn_xml, ""), ZEBRA_OK);

    YAZ_CHECK_EQ(tl_fetch_first_compare(zh, "zebra::index::nonexistent",
                                        yaz_oid_recsyn_xml, ""), ZEBRA_OK);

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

