/* $Id: recindxp.h,v 1.15 2004-08-09 09:06:13 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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

#include "recindex.h"

#include <bfile.h>

YAZ_BEGIN_CDECL

#define REC_BLOCK_TYPES 2
#define REC_HEAD_MAGIC "recindex"
#define REC_VERSION 5

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

YAZ_END_CDECL
