#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <dict.h>

static void common_init (Dict_BFile bf, int block_size, int cache)
{
    int i;

    bf->block_size = block_size;
    bf->cache = cache;
    bf->hash_size = 31;

    bf->hits = bf->misses = 0;

    /* Allocate all blocks in one chunk. */
    bf->all_data = xmalloc (block_size * cache);

    /* Allocate and initialize hash array (as empty) */
    bf->hash_array = xmalloc(sizeof(*bf->hash_array) * bf->hash_size);
    for (i=bf->hash_size; --i >= 0; )
        bf->hash_array[i] = NULL;

    /* Allocate all block descriptors in one chunk */
    bf->all_blocks = xmalloc (sizeof(*bf->all_blocks) * cache);

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


Dict_BFile dict_bf_open (const char *name, int block_size, int cache, int rw)
{
    Dict_BFile dbf;

    dbf = xmalloc (sizeof(*dbf));
    dbf->bf = bf_open (name, block_size, rw);
    if (!dbf->bf)
        return NULL;
    common_init (dbf, block_size, cache);
    return dbf;
}
