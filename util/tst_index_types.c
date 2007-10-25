/* $Id: tst_index_types.c,v 1.3 2007-10-25 19:25:00 adam Exp $
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

#include <yaz/test.h>
#include <index_types.h>
#include <stdlib.h>
#include <string.h>

const char *xml_str = 
"    <indextypes>"
"      <indextype id=\"*:w:el\" position=\"1\" alwaysmatches=\"1\" firstinfield=\"1\"\n"
"       locale=\"el\">\n"
"        <simple/>\n"
"      </indextype>\n"
"      <indextype id=\"*:w\" position=\"1\" alwaysmatches=\"1\" firstinfield=\"1\"\n"
"       locale=\"en\">\n"
"        <simple/>\n"
"      </indextype>\n"
"      <indextype id=\"*:p\" position=\"0\" alwaysmatches=\"0\" firstinfield=\"0\"\n"
"        locale=\"en\">\n"
"        <simple/>\n"
"      </indextype>\n"
"      <indextype id=\"*:s\" sort=\"1\" \n"
"        locale=\"en\">\n"
"        <simple/>\n"
"      </indextype>\n"
"    </indextypes>\n"
;

int compare_lookup(zebra_index_types_t r, const char *id,
                   const char *expected_id)
{
    const char *got_id = zebra_index_type_lookup_str(r, id);
    if (!got_id && !expected_id)
        return 1;  /* none expected */

    if (got_id && expected_id && !strcmp(got_id, expected_id))
        return 1;
    return 0;
}

void tst1(void)
{
#if YAZ_HAVE_XML2
    xmlDocPtr doc = xmlParseMemory(xml_str, strlen(xml_str));

    YAZ_CHECK(doc);
    if (doc)
    {
        zebra_index_types_t rules = zebra_index_types_create_doc(doc);
        zebra_index_type_t type;
        YAZ_CHECK(rules);

        if (!rules)
            return ;
        
        YAZ_CHECK(compare_lookup(rules, "title:s", "*:s"));
        YAZ_CHECK(compare_lookup(rules, "title:sx", 0));
        YAZ_CHECK(compare_lookup(rules, "title:Sx", 0));
        YAZ_CHECK(compare_lookup(rules, "any:w", "*:w"));
        YAZ_CHECK(compare_lookup(rules, "any:w:en", 0));
        YAZ_CHECK(compare_lookup(rules, "any:w:el", "*:w:el"));
        
        {
            int i, iter = 3333;
            for (i = 0; i < iter; i++)
            {
                compare_lookup(rules, "title:s", "*:s");
                compare_lookup(rules, "title:sx", 0);
                compare_lookup(rules, "title:Sx", 0);
            }
        }

        type = zebra_index_type_get(rules, "any:w");
        YAZ_CHECK(type);
        if (type)
        {
            const char *buf = " How are you?";
            size_t len = strlen(buf);
            int r = 1;

            if (r)
            {
                const char *result_buf = 0;
                size_t result_len = 0;
                r = zebra_index_type_tokenize(type, buf, len,
                                          &result_buf, &result_len);
                YAZ_CHECK_EQ(r, 1);
                YAZ_CHECK(result_len == 3 &&
                          !memcmp(result_buf, "how", result_len));
            }

            if (r)
            {
                const char *result_buf = 0;
                size_t result_len = 0;
                r = zebra_index_type_tokenize(type, 0,  0,
                                              &result_buf, &result_len);
                YAZ_CHECK_EQ(r, 1);
                YAZ_CHECK(result_len == 3 && 
                          !memcmp(result_buf, "are", result_len));
            }            

            if (r)
            {
                const char *result_buf = 0;
                size_t result_len = 0;
                r = zebra_index_type_tokenize(type, 0,  0,
                                              &result_buf, &result_len);
                YAZ_CHECK_EQ(r, 1);
                YAZ_CHECK(result_len == 3 && 
                          !memcmp(result_buf, "you", result_len));
            }            

            if (r)
            {
                const char *result_buf = 0;
                size_t result_len = 0;
                r = zebra_index_type_tokenize(type, 0,  0,
                                              &result_buf, &result_len);
                YAZ_CHECK_EQ(r, 0);
            }            
        }
        zebra_index_types_destroy(rules);
    }
#else
    zebra_index_types_t rules = zebra_index_types_create_doc(doc);
    YAZ_CHECK(!rules);
#endif
}

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();

    tst1();

    YAZ_CHECK_TERM;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

