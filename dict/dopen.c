/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dopen.c,v $
 * Revision 1.8  1999-05-26 07:49:12  adam
 * C++ compilation.
 *
 * Revision 1.7  1999/05/15 14:36:37  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.6  1999/02/02 14:50:20  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.5  1997/09/17 12:19:07  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.4  1997/09/09 13:38:01  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.3  1994/09/01 17:49:37  adam
 * Removed stupid line. Work on insertion in dictionary. Not finished yet.
 *
 */

#include <sys/types.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include <dict.h>

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
