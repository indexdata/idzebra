/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: cfile.h,v $
 * Revision 1.3  1995-12-01 16:24:29  adam
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

#define HASH_BUCKET 63

struct CFile_hash_bucket {
    struct CFile_ph_bucket {
        int no[HASH_BUCKET];
        int vno[HASH_BUCKET];
        int this_bucket;
        int next_bucket;
    } ph;
    int dirty;
    struct CFile_hash_bucket *h_next, **h_prev;
    struct CFile_hash_bucket *lru_next, *lru_prev;
};

#define HASH_BSIZE sizeof(struct CFile_ph_bucket)

typedef struct CFile_struct
{
    struct CFile_head {
        int hash_size;
        int next_bucket;
        int next_block;
        int block_size;
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
} *CFile;

int cf_close (CFile cf);
CFile cf_open (MFile mf, MFile_area area, const char *fname, int block_size,
               int wflag, int *firstp);
int cf_read (CFile cf, int no, int offset, int num, void *buf);
int cf_write (CFile cf, int no, int offset, int num, const void *buf);
void cf_commit (CFile cf);

#endif
