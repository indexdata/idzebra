/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.h,v $
 * Revision 1.1  1994-08-16 16:16:02  adam
 * bfile header created.
 *
 */

#ifndef BFILE_H
#define BFILE_H
struct BFile_block
{
    struct BFile_block *h_next, **h_prev;
    struct BFile_block *lru_next, *lru_prev;
    void *data;
    int dirty;
    int no;
};

typedef struct BFile_struct
{
    int fd;
    int block_size;
    int cache;
    struct BFile_block *all_blocks;
    struct BFile_block *free_list;
    struct BFile_block **hash_array;

    struct BFile_block *lru_back, *lru_front;
    int hash_size;
    void *all_data;

    int  hits;
    int  misses;
} *BFile;

int bf_close (BFile);
BFile bf_open (const char *name, int block_size, int cache);
BFile bf_open_w (const char *name, int block_size, int cache);
int bf_read (BFile bf, int no, void *buf);
int bf_write (BFile bf, int no, const void *buf);

int bf_readp (BFile bf, int no, void **bufp);
int bf_newp (BFile bf, int no, void **bufp);
int bf_touch (BFile bf, int no);
void bf_flush_blocks (BFile bf, int no_to_flush);

void *xmalloc_f (size_t size);
#define xmalloc(x) xmalloc_f(x)

extern char *prog;

#endif
