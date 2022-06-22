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

/** \file
    \brief tests ICU enabled maps
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <yaz/test.h>
#include "testlib.h"

/* utf-8 sequences for some characters */
#define char_ae "\xc3\xa6"
#define char_AE "\xc3\x86"

#define char_oslash "\xc3\xb8"
#define char_Oslash "\xc3\x98"

#define char_aring "\xc3\xa5"
#define char_Aring "\xc3\x85"

#define char_comb_ring_above "\xcc\x8a"

#define char_aring1 "a" char_comb_ring_above
#define char_Aring1 "A" char_comb_ring_above

const char *myrec[] = {
        "<gils>\n<title>My computer</title>\n</gils>\n",
        "<gils>\n<title>My x computer</title>\n</gils>\n",
        "<gils>\n<title>My computer x</title>\n</gils>\n" ,
        "<gils>\n<title>" char_ae "rme</title>\n</gils>\n" ,
        "<gils>\n<title>B" char_aring "d</title>\n"
        "<abstract>זיהוי סדר הארועים בסיפור המרד הגדול מאת צביה בן-שלום 提示:直接点击数据库名称,将进入单库检索 Ngày xửa ngày xưa D.W. all wet</abstract>\n</gils>\n" ,
        0} ;

static void tst(int argc, char **argv)
{
#if YAZ_HAVE_ICU
    ZebraService zs = tl_start_up("test_icu_indexing.cfg", argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    tl_check_filter(zs, "grs.xml");

    YAZ_CHECK(tl_init_data(zh, myrec));

    /* simple term */
    YAZ_CHECK(tl_query(zh, "@attr 1=title notfound", 0));

    YAZ_CHECK(tl_query(zh, "@attr 1=title computer", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=1 @attr 1=title computer", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=1 @attr 1=title compute", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=1 @attr 1=title computee", 0));

    YAZ_CHECK(tl_query(zh, "@attr 5=1 @attr 1=title co", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=2 @attr 1=title computer", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=2 @attr 1=title compute", 0));

    YAZ_CHECK(tl_query(zh, "@attr 5=2 @attr 1=title er", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=3 @attr 1=title computer", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=3 @attr 1=title compute", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=3 @attr 1=title er", 4));

    YAZ_CHECK(tl_query(zh, "@attr 5=3 @attr 1=title ompute", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title com.*er", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title cm.*er", 0));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title com.*ër", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title com?m.*er", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title coy?m.*er", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title co[m].*er", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title co[mn].*er", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title co[m-n].*er", 3));

#if 0
    /* fails on some systems with older ICU */
    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title co[a-z].*er", 3));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 @attr 1=title co[a-n].*er", 3));
#endif

    YAZ_CHECK(tl_query(zh, "@attr 1=title com.*ër", 0));

    YAZ_CHECK(tl_query(zh, "@attr 1=title @and @attr 5=102 com.*er x", 2));

    YAZ_CHECK(tl_query(zh, "@attr 1=title @and x @attr 5=102 com.*er", 2));

    YAZ_CHECK(tl_query(zh, "@attr 1=title .computer.", 3));

    YAZ_CHECK(tl_query(zh, "@attr 1=title x", 2));

    YAZ_CHECK(tl_query(zh, "@attr 1=title my", 3));

    YAZ_CHECK(tl_query(zh, "@attr 1=title mY", 3));

    YAZ_CHECK(tl_query(zh, char_ae "rme", 1));
    YAZ_CHECK(tl_query(zh, char_AE "RME", 1));

    YAZ_CHECK(tl_query(zh, "b" char_aring "d", 1));
    YAZ_CHECK(tl_query(zh, "B" char_Aring "D", 1));
    YAZ_CHECK(tl_query(zh, "b" char_aring1 "d", 1));
    YAZ_CHECK(tl_query(zh, "B" char_Aring1 "D", 1));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 b" char_aring "d", 1));
    YAZ_CHECK(tl_query(zh, "@attr 5=102 b.d", 1));

    YAZ_CHECK(tl_query(zh, "@attr 5=102 " char_ae "rme", 1));
    YAZ_CHECK(tl_query(zh, "@attr 5=102 " "..rme", 1));

    /* Abstract searches . Chinese mostly */
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract בן-שלום", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract צביה", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract הגדול", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract בסיפור", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract בסיפ", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract 点", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract wet", 1));

    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=1 בסיפ", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=1 סיפ", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=1 בסי", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=1 בס", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=1 ב", 1));

    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=1 בן-שלום", 1));
    /* below: should be 1, but the dash (-) is probably a problem */
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=102 בן-שלום", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 5=102 צביה", 1));

    /* phrase search */
    YAZ_CHECK(tl_query(zh, "@attr 1=title {my computer}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=title {my-computer}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=1 {my computer}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=1 {my-computer}", 2));
    YAZ_CHECK(tl_query(zh, "@attr 1=title {computer x}", 1));

    /* complete-subfield search */
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=2 {my computer}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=2 {my}", 0));

    /* always matches */
    YAZ_CHECK(tl_query(zh, "@attr 1=_ALLRECORDS @attr 2=103 {}", 5));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 2=103 {}", 5));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 2=103 {}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=abstract @attr 2=103 {does not match}", 1));

    /* scan */
    {   /* word search */
        const char *ent[] = { char_ae "rme", "B" char_aring "d", "computer",
                              "My", "x", 0 };
        YAZ_CHECK(tl_scan(zh, "@attr 1=title 0", 1, 10, 1, 5, 1, ent));
    }

    {   /* word search */
        const char *ent[] = { "My", "x", 0 };
        YAZ_CHECK(tl_scan(zh, "@attr 1=title cp", 1, 10, 1, 2, 1, ent));
    }

    {   /* phrase search */
        const char *ent[] = { char_ae "rme", "B" char_aring "d", "My computer" };
        YAZ_CHECK(tl_scan(zh, "@attr 1=title @attr 6=2 0", 1, 3, 1, 3, 0, ent));
    }

    YAZ_CHECK(tl_close_down(zh, zs));
#endif
}

static void tst2(int argc, char **argv)
{
#if YAZ_HAVE_ICU
    const char *myrec2[] = {
        "<gils>\n<title>one two three four\n</gils>\n",
        "<gils>\n<title>Jensen, S. E.\n</gils>\n",
        0} ;

    ZebraService zs = tl_start_up("test_icu_indexing.cfg", argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    tl_check_filter(zs, "grs.xml");

    YAZ_CHECK(tl_init_data(zh, myrec2));

    YAZ_CHECK(tl_query(zh, "@attr 1=title {one two three four}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title {one three four}", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=title {jensen s e}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title {jensen s}", 1));

    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=2 {jensen s e}", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=2 {jensen s }", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=2 {jensen s}", 0));
    YAZ_CHECK(tl_query(zh, "@attr 1=title @attr 6=2 e", 0));

    YAZ_CHECK(tl_close_down(zh, zs));
#endif
}

int main(int argc, char **argv)
{                                                                       \
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();
    tst(argc, argv);
    tst2(argc, argv);
    YAZ_CHECK_TERM;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

