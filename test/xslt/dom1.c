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
#include <yaz/test.h>
#include <yaz/diagbib1.h>
#include "../api/testlib.h"


void index_more(ZebraHandle zh, const char *filter, const char *file)
{
    char path[FILENAME_MAX];

    tl_profile_path(zh);

    zebra_set_resource(zh, "recordType", filter);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);
    yaz_snprintf(path, sizeof(path), "%s/%s", tl_get_srcdir(), file);

    YAZ_CHECK(zebra_repository_update(zh, path) == ZEBRA_OK);
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);
}

ZebraHandle index_some(ZebraService zs,
                       const char *filter, const char *file)
{
    ZebraHandle zh = zebra_open(zs, 0);

    tl_check_filter(zs, "dom");

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    index_more(zh, filter, file);
    return zh;
}

void tst(int argc, char **argv)
{
    ZebraHandle zh;

    ZebraService zs = tl_start_up(0, argc, argv);

    zh = index_some(zs, "dom.bad.xml", "marc-col.xml");
    zebra_close(zh);


    /* testing XMLREADER input with PI stylesheet */
    zh = index_some(zs, "dom.dom-config-col.xml", "marc-col.xml");
    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 3));

    /* fetch first using dom-brief.xsl */
    YAZ_CHECK_EQ(tl_fetch_first_compare(
                     zh, "B", yaz_oid_recsyn_xml,
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<title>How to program a computer</title>\n"),
                 ZEBRA_OK);
    /* fetch first using dom-snippets.xsl */
    YAZ_CHECK_EQ(tl_fetch_first_compare(
                     zh, "snippet", yaz_oid_recsyn_xml,
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<root>\n"
                     "  <record xmlns=\"http://www.indexdata.com/zebra/\">\n"
                     "  <snippet name=\"title\" type=\"w\">How to program a <s>computer</s></snippet>\n"
                     "</record>\n"
                     "  <title>How to program a computer</title>\n"
                     "</root>\n"),
                 ZEBRA_OK);

    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224466", 1));

    YAZ_CHECK(tl_query_x(zh, "@attr 1=titl computer", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=4 computer", 0, 121));
    zebra_close(zh);


    /* testing XMLREADER input with ELEMENT stylesheet */
    zh = index_some(zs, "dom.dom-config-one.xml", "marc-one.xml");
    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224466", 1));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=titl computer", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=4 computer", 0, 121));
    zebra_close(zh);

    /* testing MARC input with ELEMENT stylesheet */
    zh = index_some(zs, "dom.dom-config-marc.xml", "marc-col.mrc");
    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 3));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224466", 1));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=titl computer", 0, 114));
    YAZ_CHECK(tl_query_x(zh, "@attr 1=4 computer", 0, 121));
    zebra_close(zh);

    /* testing XMLREADER input with ELEMENT stylesheet and skipped records */
    zh = index_some(zs, "dom.dom-config-skipped.xml", "marc-col.xml");
    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224466", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 11224467", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=control 73090924", 0));

    /* testing XMLREADER input with type attributes (insert,delete,..) */
    zh = index_some(zs, "dom.dom-config-del.xml", "del-col.xml");
    YAZ_CHECK(tl_query(zh, "@attr 1=title a", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title 1", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=title 2", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=title 3", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title b", 1));

    zh = index_some(zs, "dom.gutenberg.xml", "gutenberg-sample.xml");
    YAZ_CHECK(tl_query(zh, "oscar", 1));

    YAZ_CHECK_EQ(tl_fetch_first_compare(
                     zh, "zebra::snippet", yaz_oid_recsyn_xml,
                     "<record xmlns=\"http://www.indexdata.com/zebra/\">\n"
                     "  <snippet name=\"any\" type=\"w\" fields=\"title\">"
                     "Selected Prose of <s>Oscar</s> Wilde"
                     "</snippet>\n"
                     "  <snippet name=\"any\" type=\"w\" fields=\"creator\">"
                     "Wilde, <s>Oscar</s>, 1854-1900"
                     "</snippet>\n"
                     "</record>"),
                 ZEBRA_OK);

    zebra_close(zh);

    zh = index_some(zs, "dom.biblio-config.xml", "biblio-record.xml");
    YAZ_CHECK(tl_query(zh, "@attr 1=Author leary", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=Author valen", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=Author {valen line}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=Author {valen, line}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=Relator-term medarb", 1));
    YAZ_CHECK(tl_query(zh, "@and "
                       "@attr 1=Author valen "
                       "@attr 1=Relator-term medarb", 1));
    YAZ_CHECK(tl_query(zh, "@and "
                       "@attr 1=Author {leary ian} "
                       "@attr 1=Relator-term medarb", 1));
    YAZ_CHECK(tl_query(zh, "@prox 0 0 0 0 k 8 "
                       "@attr 1=Author leary "
                       "@attr 1=Relator-term medarb", 1));
    YAZ_CHECK(tl_query(zh, "@prox 0 0 0 0 k 8 "
                       "@attr 1=Author valen "
                       "@attr 1=Relator-term medarb", 0));
    YAZ_CHECK(tl_query(zh, "@prox 0 0 0 0 k 8 "
                       "@attr 1=Author {leary ian} "
                       "@attr 1=Relator-term medarb", 1));
    YAZ_CHECK(tl_query(zh, "@prox 0 0 0 0 k 8 "
                       "@attr 1=Relator-term medarb "
                       "@attr 1=Author {leary ian}", 1));
    YAZ_CHECK(tl_query_x(zh, "@prox 0 0 0 0 k 6 "
                         "@attr 1=Author {leary ian} "
                         "@attr 1=Relator-term medarb", 0,
                         YAZ_BIB1_UNSUPP_PROX_UNIT_CODE));
    zebra_close(zh);

    /* testing indexing of bad UTF-8 encoded MARC record */
    zh = index_some(zs, "dom.dom-config-utf8.xml", "bad-utf8.mrc");
    zebra_close(zh);

    YAZ_CHECK(tl_close_down(0, zs));
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

