/*
 * Copyright (C) 1995-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: cfile.h,v 1.11 1999-05-12 13:08:06 adam Exp $
 */

#ifndef CFILE_H
#define CFILE_H

#define HASH_BUCKET 15

struct CFile_ph_bucket {     /* structure on disc */
    int no[HASH_BUCKET];     /* block number in original file */
    int vno[HASH_BUCKET];    /* block number in shadow file */
    int this_bucket;         /* this bucket number */
    int next_bucket;         /* next bucket number */
};

struct CFile_hash_bucket {
    struct CFile_ph_bucket ph;
    int dirty;
    struct CFile_hash_bucket *h_next, **h_prev;
    struct CFile_hash_bucket *lru_next, *lru_prev;
};

#define HASH_BSIZE sizeof(struct CFile_ph_bucket)

#define CFILE_FLAT 1

typedef struct CFile_struct
{
    struct CFile_head {
        int state;               /* 1 = hash, 2 = flat */
        int next_block;          /* next free block / last block */
        int block_size;          /* mfile/bfile block size */
        int hash_size;           /* no of chains in hash table */
        int first_bucket;        /* first hash bucket */
        int next_bucket;         /* last hash bucket + 1 = first flat bucket */
        int flat_bucket;         /* last flat bucket + 1 */
    } head;
    MFile block_mf;
    MFile hash_mf;
    int *array;
    struct CFile_hash_bucket **parray;
    struct CFile_hash_bucket *bucket_lru_front, *bucket_lru_back;
    int dirty;
    int bucket_in_memory;
    int max_bucket_in_memory;
    char *iobuf;
    MFile rmf;
    int  no_hits;
    int  no_miss;
} *CFile;

int cf_close (CFile cf);
CFile cf_open (MFile mf, MFile_area area, const char *fname, int block_size,
               int wflag, int *firstp);
int cf_read (CFile cf, int no, int offset, int nbytes, void *buf);
int cf_write (CFile cf, int no, int offset, int nbytes, const void *buf);
void cf_unlink (CFile cf);
void cf_commit (CFile cf);

#endif
