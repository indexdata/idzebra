/* $Id: recindxp.h,v 1.12.2.1 2006-08-14 10:38:59 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#include "recindex.h"

#include <bfile.h>

YAZ_BEGIN_CDECL

#define REC_BLOCK_TYPES 2
#define REC_HEAD_MAGIC "recindex"
#define REC_VERSION 4

struct records_info {
    int rw;
    int compression_method;

    char *index_fname;
    BFile index_BFile;

    char *data_fname[REC_BLOCK_TYPES];
    BFile data_BFile[REC_BLOCK_TYPES];

    char *tmp_buf;
    int tmp_size;

    struct record_cache_entry *record_cache;
    int cache_size;
    int cache_cur;
    int cache_max;

    Zebra_mutex mutex;

    struct records_head {
        char magic[8];
	char version[4];
        int block_size[REC_BLOCK_TYPES];
        int block_free[REC_BLOCK_TYPES];
        int block_last[REC_BLOCK_TYPES];
        int block_used[REC_BLOCK_TYPES];
        int block_move[REC_BLOCK_TYPES];

        int total_bytes;
        int index_last;
        int index_free;
        int no_records;

    } head;
};

enum recordCacheFlag { recordFlagNop, recordFlagWrite, recordFlagNew,
                       recordFlagDelete };

struct record_cache_entry {
    Record rec;
    enum recordCacheFlag flag;
};

struct record_index_entry {
    int next;         /* first block of record info / next free entry */
    int size;         /* size of record or 0 if free entry */
};

YAZ_END_CDECL
