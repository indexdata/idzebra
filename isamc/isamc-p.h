/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isamc-p.h,v $
 * Revision 1.7  1999-05-26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.6  1998/03/18 09:23:55  adam
 * Blocks are stored in chunks on free list - up to factor 2 in speed.
 * Fixed bug that could occur in block category rearrangemen.
 *
 * Revision 1.5  1998/03/16 10:37:24  adam
 * Added more statistics.
 *
 * Revision 1.4  1996/11/08 11:15:28  adam
 * Number of keys in chain are stored in first block and the function
 * to retrieve this information, isc_pp_num is implemented.
 *
 * Revision 1.3  1996/11/04 14:08:55  adam
 * Optimized free block usage.
 *
 * Revision 1.2  1996/11/01 08:59:13  adam
 * First version of isc_merge that supports update/delete.
 *
 * Revision 1.1  1996/10/29 13:40:47  adam
 * First work.
 *
 */

#include <bfile.h>
#include <isamc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int lastblock;
    int freelist;
} ISAMC_head;

typedef unsigned ISAMC_BLOCK_SIZE;

typedef struct ISAMC_file_s {
    ISAMC_head head;
    BFile bf;
    int head_is_dirty;
    
    int no_writes;
    int no_reads;
    int no_skip_writes;
    int no_allocated;
    int no_released;
    int no_remap;

    int no_forward;
    int no_backward;
    int sum_forward;
    int sum_backward;
    int no_next;
    int no_prev;

    char *alloc_buf;
    int alloc_entries_num;
    int alloc_entries_max;

    int fc_max;
    int *fc_list;
} *ISAMC_file;

struct ISAMC_s {
    int no_files;
    int max_cat;
    char *merge_buf;
    ISAMC_M method;
    ISAMC_file files;
}; 

struct ISAMC_PP_s {
    char *buf;
    ISAMC_BLOCK_SIZE offset;
    ISAMC_BLOCK_SIZE size;
    int cat;
    int pos;
    int next;
    ISAMC is;
    void *decodeClientData;
    int deleteFlag;
    int numKeys;
};

#define ISAMC_BLOCK_OFFSET_N (sizeof(int)+sizeof(ISAMC_BLOCK_SIZE)) 
#define ISAMC_BLOCK_OFFSET_1 (sizeof(int)+sizeof(ISAMC_BLOCK_SIZE)+sizeof(int)) 
int isc_alloc_block (ISAMC is, int cat);
void isc_release_block (ISAMC is, int cat, int pos);
int isc_read_block (ISAMC is, int cat, int pos, char *dst);
int isc_write_block (ISAMC is, int cat, int pos, char *src);

#ifdef __cplusplus
}
#endif

