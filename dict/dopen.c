/* $Id: dopen.c,v 1.14 2006-08-14 10:40:09 adam Exp $
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



#include <sys/types.h>
#include <fcntl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "dict-p.h"

static void common_init (Dict_BFile bf, int block_size, int cache)
{
    int i;

    bf->block_size = block_size;
    bf->compact_flag = 0;
    bf->cache = cache;
    bf->hash_size = 31;

    bf->hits = bf->misses = 0;

    /* Allocate all blocks in one chunk. */
    bf->all_data = xmalloc (block_size * cache);

    /* Allocate and initialize hash array (as empty) */
    bf->hash_array = (struct Dict_file_block **)
	xmalloc(sizeof(*bf->hash_array) * bf->hash_size);
    for (i=bf->hash_size; --i >= 0; )
        bf->hash_array[i] = NULL;

    /* Allocate all block descriptors in one chunk */
    bf->all_blocks = (struct Dict_file_block *)
	xmalloc (sizeof(*bf->all_blocks) * cache);

    /* Initialize the free list */
    bf->free_list = bf->all_blocks;
    for (i=0; i<cache-1; i++)
        bf->all_blocks[i].h_next = bf->all_blocks+(i+1);
    bf->all_blocks[i].h_next = NULL;

    /* Initialize the data for each block. Will never be moved again */
    for (i=0; i<cache; i++)
        bf->all_blocks[i].data = (char*) bf->all_data + i*block_size;

    /* Initialize lru queue */
    bf->lru_back = NULL;
    bf->lru_front = NULL;
}


Dict_BFile dict_bf_open (BFiles bfs, const char *name, int block_size,
			 int cache, int rw)
{
    Dict_BFile dbf;

    dbf = (Dict_BFile) xmalloc (sizeof(*dbf));
    dbf->bf = bf_open (bfs, name, block_size, rw);
    if (!dbf->bf)
        return NULL;
    common_init (dbf, block_size, cache);
    return dbf;
}

void dict_bf_compact (Dict_BFile dbf)
{
    dbf->compact_flag = 1;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

