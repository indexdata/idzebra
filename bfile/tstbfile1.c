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

#include <stdlib.h>
#include <string.h>
#include <idzebra/bfile.h>
#include <yaz/test.h>

void tst1(BFiles bfs)
{
    int version = 1;
    BFile bf;
    const char *more_info = 0;

    bf_reset(bfs);
    bf = bf_xopen(bfs, "tst", /* block size */ 32, 
		  /* wr */ 1, "tstmagic",	&version, 0 /* more_info */);
    YAZ_CHECK(bf);
    if (!bf)
        return;
    bf_xclose(bf, version, "more info");
    
    bf = bf_xopen(bfs, "tst", /* block size */ 32, 
		  /* wr */ 1, "tstmagic",	&version, &more_info);
    
    YAZ_CHECK(bf);
    if (!bf)
        return;

    YAZ_CHECK(strcmp(more_info, "more info") == 0);
    bf_xclose(bf, version, 0 /* no more info */);
}

void tst2(BFiles bfs)
{
    int version = 1;
    int bno, i;
    zint blocks[1000];
    BFile bf;
    bf_reset(bfs);

    bf = bf_xopen(bfs, "tst", /* block size */ 32, 
			/* wr */ 1, "tstmagic",	&version, 0 /* more_info */);
    YAZ_CHECK(bf);
    if (!bf)
        return;
    bno = 0;
    for (i = 1; i<30; i++)
    {
	int j;
	for (j = 0; j<i; j++)
	    blocks[bno + j] = 0;
        YAZ_CHECK_EQ(bf_alloc(bf, i, blocks + bno), 0);
	for (j = 0; j < i; j++)
	{
            YAZ_CHECK(blocks[bno + j]);
	}
	bno += i;
    }
    for (i = 0; i<bno; i++)
    {
        YAZ_CHECK_EQ(bf_free(bf, 1, blocks + i), 0);
    }
    bf_xclose(bf, version, 0);
}

#define BLOCKS 100

void tst3(BFiles bfs)
{
    int version = 1;
    int i;
    zint blocks[BLOCKS];
    BFile bf;
    int no_in_use = 0;
    int pass = 0;
    bf_reset(bfs);

    for(i = 0; i<BLOCKS; i++)
	blocks[i] = 0;

    bf = bf_xopen(bfs, "tst", /* block size */ 32, 
			/* wr */ 1, "tstmagic",	&version, 0 /* more_info */);
    YAZ_CHECK(bf);
    if (!bf)
        return;
    no_in_use = 0;
    for (pass = 0; pass < 100; pass++)
    {
	int r = random() % 9;

        YAZ_CHECK(no_in_use >= 0);
        YAZ_CHECK(no_in_use <= BLOCKS);
	if (r < 5 && (BLOCKS - no_in_use) > 0)
	{
	    /* alloc phase */
	    int j = 0;
	    zint tblocks[BLOCKS];
	    int left = BLOCKS - no_in_use;
	    int to_alloc = 1 + (random() % left);

	    bf_alloc(bf, to_alloc, tblocks);
	    /* transfer from tblocks to blocks */
	    for (i = 0; i<BLOCKS; i++)
	    {
		if (blocks[i] == 0)
		{
		    blocks[i] = tblocks[j++];
		    no_in_use++;
		    if (j == to_alloc)
			break;
		}
	    }
	}
	else if (r < 8 && no_in_use > 0)
	{
	    /* free phase */
	    zint tblocks[BLOCKS];
	    int to_free = 1 + (random() % no_in_use);
	    int start = random() % to_free;
	    int j = 0;
	    for (i = 0; i<BLOCKS; i++)
	    {
		if (blocks[i])
		{
		    if (j >= start && j < to_free)
		    {
			tblocks[j-start] = blocks[i];
			blocks[i] = 0;
			no_in_use--;
		    }
		    j++;
		}
	    }
	    YAZ_CHECK(tblocks[to_free-start-1]);
	    bf_free(bf, to_free - start, tblocks);
	}
	else
	{
	    bf_xclose(bf, version, 0);
	    bf = bf_xopen(bfs, "tst", /* block size */ 32, 
			  /* wr */ 1, "tstmagic", &version, 0 /* more_info */);
	}
    }
    bf_xclose(bf, version, 0);
}

static void tst(void)
{
    BFiles bfs = bfs_create(0, /* register: current dir, no limit */
			    0  /* base: current dir */
	);
    YAZ_CHECK(bfs);
    if (!bfs)
	return;

    tst1(bfs);
    tst2(bfs);
    tst3(bfs);
    bf_reset(bfs);
    bfs_destroy(bfs);
}

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();
    tst();
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

