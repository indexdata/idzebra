/* $Id: tstbfile2.c,v 1.1 2006-11-08 12:59:27 adam Exp $
   Copyright (C) 1995-2006
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

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <yaz/test.h>
#include <idzebra/bfile.h>

void tst(void)
{
    int r;
    BFiles bfs;
    BFile bf;
    char buf[256];
    int block_size = 16;
    zint max_block = 200000;

    YAZ_CHECK(block_size <= sizeof(buf));
    if (!(block_size <= sizeof(buf)))
	return;

    YAZ_CHECK(max_block * block_size < 4 * 1000000); /* 4M */
    
    r = mkdir("register", 0777);
    YAZ_CHECK(r == 0 || (r == -1 && errno == EEXIST));

    r = mkdir("shadow", 0777);
    YAZ_CHECK(r == 0 || (r == -1 && errno == EEXIST));

    bfs = bfs_create("register.bad:4M", 0 /* base: current dir */);
    YAZ_CHECK(!bfs);
    if (bfs)
	return;

    bfs = bfs_create("register:4M", 0 /* base: current dir */);
    YAZ_CHECK(bfs);
    if (!bfs)
	return;

    r = bf_cache(bfs, "shadow.bad:4M");
    YAZ_CHECK_EQ(r, ZEBRA_FAIL);

    r = bf_cache(bfs, "shadow:4M");
    YAZ_CHECK_EQ(r, ZEBRA_OK);

    bf_reset(bfs);

    bf = bf_open(bfs, "file", block_size, 1);
    YAZ_CHECK(bf);
    if (bf)
    {
	zint bno[2];
	memset(buf, '\0', block_size);

	bno[0] = 0;
	bno[1] = 1;
	while (bno[0] < max_block)
	{
	    zint next = bno[0] + bno[1];

	    sprintf(buf, ZINT_FORMAT, bno[0]);
	    YAZ_CHECK_EQ(bf_write(bf, bno[0], 0, 0, buf), 0);

	    bno[0] = bno[1];
	    bno[1] = next;
	}
	bf_close(bf);
    }

    bf = bf_open(bfs, "file", block_size, 0);
    YAZ_CHECK(bf);
    if (bf)
    {
	zint bno[2];

	bno[0] = 0;
	bno[1] = 1;
	while (bno[0] < max_block)
	{
	    zint next = bno[0] + bno[1];
	    memset(buf, '\0', block_size);

	    YAZ_CHECK_EQ(bf_read(bf, bno[0], 0, 0, buf), 1);
	    YAZ_CHECK_EQ(atoi(buf), bno[0]);

	    bno[0] = bno[1];
	    bno[1] = next;
	}
	bf_close(bf);
    }

#if 1
    bf = bf_open(bfs, "file", block_size, 1);
    YAZ_CHECK(bf);
    if (bf)
    {
	zint bno = 0;
	while (bno < max_block)
	{
	    memset(buf, '\0', block_size);

	    sprintf(buf, ZINT_FORMAT, bno);
	    YAZ_CHECK_EQ(bf_write(bf, bno, 0, 0, buf), 0);

	    bno = bno + 2;
	}
	bf_close(bf);
    }

    bf = bf_open(bfs, "file", block_size, 0);
    YAZ_CHECK(bf);
    if (bf)
    {
	zint bno = 0;
	while (bno < max_block)
	{
	    memset(buf, '\0', block_size);

	    YAZ_CHECK_EQ(bf_read(bf, bno, 0, 0, buf), 1);
	    YAZ_CHECK_EQ(atoi(buf), bno);

	    bno = bno + 2;
	}
	bf_close(bf);
    }
#endif
    bfs_destroy(bfs);
}


int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    tst();
    YAZ_CHECK_TERM;
}

