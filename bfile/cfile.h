/* $Id: cfile.h,v 1.14 2002-08-02 19:26:55 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

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



#ifndef CFILE_H
#define CFILE_H

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

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
    Zebra_mutex mutex;
} *CFile;

int cf_close (CFile cf);
CFile cf_open (MFile mf, MFile_area area, const char *fname, int block_size,
               int wflag, int *firstp);
int cf_read (CFile cf, int no, int offset, int nbytes, void *buf);
int cf_write (CFile cf, int no, int offset, int nbytes, const void *buf);
void cf_unlink (CFile cf);
void cf_commit (CFile cf);

YAZ_END_CDECL

#endif
