/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindxp.h,v $
 * Revision 1.1  1995-12-06 12:41:25  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 */

#include "recindex.h"

#include <bfile.h>

#define REC_BLOCK_TYPES 2
#define REC_HEAD_MAGIC "recindx"

struct records_info {
    int rw;

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

    struct records_head {
        char magic[8];
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
    union {
        struct {
            int next;
            int size;
        } used;
        struct {
            int next;
        } free;
    } u;
};

