/*
 * Copyright (C) 1995-1998, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: cfile.h,v $
 * Revision 1.10  1998-10-15 13:09:31  adam
 * Minor changes.
 *
 * Revision 1.9  1998/08/07 15:07:15  adam
 * Fixed but in cf_commit_flat.
 *
 * Revision 1.8  1996/04/18 16:02:56  adam
 * Changed logging a bit.
 * Removed warning message when commiting flat shadow files.
 *
 * Revision 1.7  1996/02/07  14:03:48  adam
 * Work on flat indexed shadow files.
 *
 * Revision 1.6  1996/02/07  10:08:45  adam
 * Work on flat shadow (not finished yet).
 *
 * Revision 1.5  1995/12/15  12:36:52  adam
 * Moved hash file information to union.
 * Renamed commit files.
 *
 * Revision 1.4  1995/12/11  09:03:54  adam
 * New function: cf_unlink.
 * New member of commit file head: state (0) deleted, (1) hash file.
 *
 * Revision 1.3  1995/12/01  16:24:29  adam
 * Commit files use separate meta file area.
 *
 * Revision 1.2  1995/12/01  11:37:23  adam
 * Cached/commit files implemented as meta-files.
 *
 * Revision 1.1  1995/11/30  08:33:12  adam
 * Started work on commit facility.
 *
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
int cf_read (CFile cf, int no, int offset, int num, void *buf);
int cf_write (CFile cf, int no, int offset, int num, const void *buf);
void cf_unlink (CFile cf);
void cf_commit (CFile cf);

#endif
