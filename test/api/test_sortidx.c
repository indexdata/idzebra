/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

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

static void tst1(zebra_sort_index_t si)
{
    zint sysno = 12; /* just some sysno */
    int my_type = 2; /* just some type ID */
    char read_buf[SORT_IDX_ENTRYSIZE];

    zebra_sort_type(si, my_type);

    zebra_sort_sysno(si, sysno);
    YAZ_CHECK_EQ(zebra_sort_read(si, read_buf), 0);

    zebra_sort_add(si, "abcde1", 6);

    zebra_sort_sysno(si, sysno);
    YAZ_CHECK_EQ(zebra_sort_read(si, read_buf), 1);
    YAZ_CHECK(!strcmp(read_buf, "abcde1"));

    zebra_sort_sysno(si, sysno+1);
    YAZ_CHECK_EQ(zebra_sort_read(si, read_buf), 0);

    zebra_sort_sysno(si, sysno-1);
    YAZ_CHECK_EQ(zebra_sort_read(si, read_buf), 0);

    zebra_sort_sysno(si, sysno);
    zebra_sort_delete(si);
    YAZ_CHECK_EQ(zebra_sort_read(si, read_buf), 0);
}

static void tst(int argc, char **argv)
{
    BFiles bfs = bfs_create(".:50M", 0);
    zebra_sort_index_t si;

    YAZ_CHECK(bfs);
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
        si = zebra_sort_open(bfs, 1, ZEBRA_SORT_TYPE_FLAT);
        YAZ_CHECK(si);
        if (si)
        {
            tst1(si);
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

