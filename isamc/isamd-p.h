/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 *
 *
 */

#include <bfile.h>
#include <isamd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int lastblock;
    int freelist;
} ISAMD_head;

typedef unsigned ISAMD_BLOCK_SIZE;

typedef struct ISAMD_file_s {
    ISAMD_head head;
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
} *ISAMD_file;

struct ISAMD_s {
    int no_files;
    int max_cat;
  //  char *merge_buf;
    char *startblock; /* start of the chain, update lastptr and numKeys here */
    char *lastblock;  /* end of the chain, append here */
    ISAMD_M method;
    ISAMD_file files;
}; 

struct ISAMD_PP_s {
    char *buf;
    ISAMD_BLOCK_SIZE offset;
    ISAMD_BLOCK_SIZE size;
    int cat;  /* category of this block */
    int pos;  /* block number of this block */
    int next; /* number of the next block */
    ISAMD is;
    void *decodeClientData;
    int deleteFlag;
    int numKeys;
    ISAMD_BLOCK_SIZE lastblock;  /* last block in chain */
};

#define ISAMD_BLOCK_OFFSET_N (sizeof(int) +  \
                              sizeof(ISAMD_BLOCK_SIZE)) 
/* == 8 */
#define ISAMD_BLOCK_OFFSET_1 (sizeof(int) + \
                              sizeof(ISAMD_BLOCK_SIZE) + \
                              sizeof(int) + \
                              sizeof(ISAMD_BLOCK_SIZE)) 
/* == 16 */
int isamd_alloc_block (ISAMD is, int cat);
void isamd_release_block (ISAMD is, int cat, int pos);
int isamd_read_block (ISAMD is, int cat, int pos, char *dst);
int isamd_write_block (ISAMD is, int cat, int pos, char *src);

#ifdef __cplusplus
}
#endif



/*
 * $Log: isamd-p.h,v $
 * Revision 1.2  1999-07-14 13:21:34  heikki
 * Added isam-d files. Compiles (almost) clean. Doesn't work at all
 *
 * Revision 1.1  1999/07/14 12:34:43  heikki
 * Copied from isamh, starting to change things...
 *
 *
 */