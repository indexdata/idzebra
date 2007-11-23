/* $Id: recindxp.h,v 1.22 2007-11-23 13:11:08 adam Exp $
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

#include "recindex.h"

#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

#define REC_BLOCK_TYPES 2
#define REC_HEAD_MAGIC "recindex"
#define REC_VERSION 5

struct recindex {
    char *index_fname;
    BFile index_BFile;
};

typedef struct recindex *recindex_t;

struct records_info {
    int rw;
    int compression_method;

    recindex_t recindex;

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
        zint block_size[REC_BLOCK_TYPES];
        zint block_free[REC_BLOCK_TYPES];
        zint block_last[REC_BLOCK_TYPES];
        zint block_used[REC_BLOCK_TYPES];
        zint block_move[REC_BLOCK_TYPES];

        zint total_bytes;
        zint index_last;
        zint index_free;
        zint no_records;

    } head;
};

enum recordCacheFlag { recordFlagNop, recordFlagWrite, recordFlagNew,
                       recordFlagDelete };

struct record_cache_entry {
    Record rec;
    enum recordCacheFlag flag;
};

struct record_index_entry {
    zint next;         /* first block of record info / next free entry */
    int size;          /* size of record or 0 if free entry */
};

Record rec_cp(Record rec);

YAZ_END_CDECL
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

