/* $Id: tstbfile1.c,v 1.1 2005-03-30 09:25:23 adam Exp $
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

#include <assert.h>
#include <stdlib.h>
#include <idzebra/bfile.h>

void tst1(BFiles bfs)
{
    int version = 1;
    BFile bf;

    bf_reset(bfs);
    bf = bf_xopen(bfs, "tst", /* block size */ 32, 
		  /* wr */ 1, "tstmagic",	&version, 0 /* more_info */);
    
    bf_xclose(bf, version, 0 /* no more info */);
    
    bf = bf_xopen(bfs, "tst", /* block size */ 32, 
		  /* wr */ 1, "tstmagic",	&version, 0 /* more_info */);
    
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

    bno = 0;
    for (i = 1; i<30; i++)
    {
	int j;
	for (j = 0; j<i; j++)
	    blocks[bno + j] = 0;
	if (bf_alloc(bf, i, blocks + bno))
	{
	    fprintf(stderr, "bf_alloc failed i=%d bno=%d\n", i, bno);
	    exit(1);
	}
	for (j = 0; j < i; j++)
	{
	    if (!blocks[bno + j])
	    {
		fprintf(stderr, "zero block i=%d bno=%d j=%d\n", i, bno, j);
		exit(1);
	    }
	}
	bno += i;
    }
    for (i = 0; i<bno; i++)
    {
	if (bf_free(bf, 1, blocks + i))
	{
	    fprintf(stderr, "bf_freec failed i=%d\n", i);
	    exit(1);
	}
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

    no_in_use = 0;
    for (pass = 0; pass < 100; pass++)
    {
	int r = random() % 9;

	assert (no_in_use >= 0 && no_in_use <= BLOCKS);
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
	    assert(tblocks[to_free-start-1]);
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

int main(int argc, char **argv)
{
    BFiles bfs = bfs_create(0, /* register: current dir, no limit */
			    0  /* base: current dir */
	);
    if (!bfs)
	exit(1);

    tst1(bfs);
    tst2(bfs);
    tst3(bfs);
    bf_reset(bfs);
    bfs_destroy(bfs);
    exit(0);
}
