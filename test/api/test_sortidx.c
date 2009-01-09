/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include <idzebra/bfile.h>
#include <sortidx.h>
#include "testlib.h"

static void sort_add_cstr(zebra_sort_index_t si, const char *str,
                          zint section_id)
{
    WRBUF w = wrbuf_alloc();
    wrbuf_puts(w, str);
    wrbuf_putc(w, '\0');
    zebra_sort_add(si, section_id, w);
    wrbuf_destroy(w);
}

static void tst1(zebra_sort_index_t si)
{
    zint sysno = 12; /* just some sysno */
    int my_type = 2; /* just some type ID */
    WRBUF w = wrbuf_alloc();

    zebra_sort_type(si, my_type);

    zebra_sort_sysno(si, sysno);
    YAZ_CHECK_EQ(zebra_sort_read(si, 0, w), 0);

    sort_add_cstr(si, "abcde1", 0);

    zebra_sort_sysno(si, sysno);
    YAZ_CHECK_EQ(zebra_sort_read(si, 0, w), 1);
    YAZ_CHECK(!strcmp(wrbuf_cstr(w), "abcde1"));

    zebra_sort_sysno(si, sysno+1);
    YAZ_CHECK_EQ(zebra_sort_read(si, 0, w), 0);

    zebra_sort_sysno(si, sysno-1);
    YAZ_CHECK_EQ(zebra_sort_read(si, 0, w), 0);

    zebra_sort_sysno(si, sysno);
    zebra_sort_delete(si, 0);
    YAZ_CHECK_EQ(zebra_sort_read(si, 0, w), 0);

    zebra_sort_type(si, my_type);

    zebra_sort_sysno(si, sysno);
    YAZ_CHECK_EQ(zebra_sort_read(si, 0, w), 0);

    wrbuf_rewind(w);
    sort_add_cstr(si, "abcde1", 0);

    zebra_sort_sysno(si, sysno);
    YAZ_CHECK_EQ(zebra_sort_read(si, 0, w), 1);
    YAZ_CHECK(!strcmp(wrbuf_cstr(w), "abcde1"));

    zebra_sort_sysno(si, sysno);
    zebra_sort_delete(si, 0);

    wrbuf_destroy(w);
}

static void tst2(zebra_sort_index_t si)
{
    zint sysno = 15; /* just some sysno */
    int my_type = 2; /* just some type ID */
    int i;

    zebra_sort_type(si, my_type);

    for (sysno = 1; sysno < 50; sysno++)
    {
        zint input_section_id = 12345;
        zint output_section_id = 0;
        WRBUF w1 = wrbuf_alloc();
        WRBUF w2 = wrbuf_alloc();
        zebra_sort_sysno(si, sysno);
        YAZ_CHECK_EQ(zebra_sort_read(si, 0, w2), 0);
        
        for (i = 0; i < 600; i++) /* 600 * 6 < max size =4K */
            wrbuf_write(w1, "12345", 6);
        
        zebra_sort_add(si, input_section_id, w1);
        
        zebra_sort_sysno(si, sysno);
        
        YAZ_CHECK_EQ(zebra_sort_read(si, &output_section_id, w2), 1);
        
        YAZ_CHECK_EQ(wrbuf_len(w1), wrbuf_len(w2));
        YAZ_CHECK(!memcmp(wrbuf_buf(w1), wrbuf_buf(w2), wrbuf_len(w2)));
        YAZ_CHECK_EQ(input_section_id, output_section_id);
        wrbuf_destroy(w1);
        wrbuf_destroy(w2);
    }
}

static void tst(int argc, char **argv)
{
    BFiles bfs = bfs_create(".:50M", 0);
    zebra_sort_index_t si;

    YAZ_CHECK(bfs);
    if (bfs)
    {
        bf_reset(bfs);
        si = zebra_sort_open(bfs, 1, ZEBRA_SORT_TYPE_FLAT);
        YAZ_CHECK(si);
        if (si)
        {
            tst1(si);
            zebra_sort_close(si);
        }
    }
    if (bfs)
    {
        bf_reset(bfs);
        si = zebra_sort_open(bfs, 1, ZEBRA_SORT_TYPE_ISAMB);
        YAZ_CHECK(si);
        if (si)
        {
            tst1(si);
            zebra_sort_close(si);
        }
    }
    if (bfs)
    {
        bf_reset(bfs);
        si = zebra_sort_open(bfs, 1, ZEBRA_SORT_TYPE_MULTI);
        YAZ_CHECK(si);
        if (si)
        {
            tst1(si);
            tst2(si);
            zebra_sort_close(si);
        }
    }
    if (bfs)
        bfs_destroy(bfs);
}

TL_MAIN
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

