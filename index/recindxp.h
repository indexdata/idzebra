/*
 * Copyright (C) 1994-2000, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindxp.h,v $
 * Revision 1.9  2000-12-05 10:01:44  adam
 * Fixed bug regarding user-defined attribute sets.
 *
 * Revision 1.8  2000/04/05 09:49:35  adam
 * On Unix, zebra/z'mbol uses automake.
 *
 * Revision 1.7  1999/07/06 12:28:04  adam
 * Updated record index structure. Format includes version ID. Compression
 * algorithm ID is stored for each record block.
 *
 * Revision 1.6  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.5  1999/02/02 14:51:05  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.4  1998/03/05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.3  1995/12/11 11:45:55  adam
 * Removed commented code.
 *
 * Revision 1.2  1995/12/11  09:12:51  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.1  1995/12/06  12:41:25  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 */

#include "recindex.h"

#include <bfile.h>

YAZ_BEGIN_CDECL

#define REC_BLOCK_TYPES 2
#define REC_HEAD_MAGIC "recindex"
#define REC_VERSION 2

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
