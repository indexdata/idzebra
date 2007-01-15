/* $Id: cfile.h,v 1.22 2007-01-15 20:08:23 adam Exp $
   Copyright (C) 1995-2007
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



#ifndef CFILE_H
#define CFILE_H

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

#define HASH_BUCKET 15

struct CFile_ph_bucket {     /* structure on disc */
    zint no[HASH_BUCKET];    /* block number in original file */
    zint vno[HASH_BUCKET];   /* block number in shadow file */
    zint this_bucket;        /* this bucket number */
    zint next_bucket;        /* next bucket number */
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
        zint next_block;          /* next free block / last block */
        int block_size;          /* mfile/bfile block size */
        int hash_size;           /* no of chains in hash table */
        zint first_bucket;       /* first hash bucket */
        zint next_bucket;        /* last hash bucket + 1 = first flat bucket */
        zint flat_bucket;        /* last flat bucket + 1 */
    } head;
    MFile block_mf;
    MFile hash_mf;
    zint *array;
    struct CFile_hash_bucket **parray;
    struct CFile_hash_bucket *bucket_lru_front, *bucket_lru_back;
    int dirty;
    zint bucket_in_memory;
    zint max_bucket_in_memory;
    char *iobuf;
    MFile rmf;
    int  no_hits;
    int  no_miss;
    Zebra_mutex mutex;
} *CFile;

int cf_close (CFile cf);
CFile cf_open (MFile mf, MFile_area area, const char *fname, int block_size,
               int wflag, int *firstp);
int cf_read (CFile cf, zint no, int offset, int nbytes, void *buf);
int cf_write (CFile cf, zint no, int offset, int nbytes, const void *buf);
int cf_commit (CFile cf) ZEBRA_GCC_ATTR((warn_unused_result));

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

