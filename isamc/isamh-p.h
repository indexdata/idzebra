/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 *
 *
 */

#include <bfile.h>
#include <isamh.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int lastblock;
    int freelist;
} ISAMH_head;

typedef unsigned ISAMH_BLOCK_SIZE;

typedef struct ISAMH_file_s {
    ISAMH_head head;
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
} *ISAMH_file;

struct ISAMH_s {
    int no_files;
    int max_cat;
  //  char *merge_buf;
    char *startblock; /* start of the chain, update lastptr and numKeys here */
    char *lastblock;  /* end of the chain, append here */
    ISAMH_M method;
    ISAMH_file files;
}; 

struct ISAMH_PP_s {
    char *buf;
    ISAMH_BLOCK_SIZE offset;
    ISAMH_BLOCK_SIZE size;
    int cat;  /* category of this block */
    int pos;  /* block number of this block */
    int next; /* number of the next block */
    ISAMH is;
    void *decodeClientData;
    int deleteFlag;
    int numKeys;
    ISAMH_BLOCK_SIZE lastblock;  /* last block in chain */
};

#define ISAMH_BLOCK_OFFSET_N (sizeof(int) +  \
                              sizeof(ISAMH_BLOCK_SIZE)) 
/* == 8 */
#define ISAMH_BLOCK_OFFSET_1 (sizeof(int) + \
                              sizeof(ISAMH_BLOCK_SIZE) + \
                              sizeof(int) + \
                              sizeof(ISAMH_BLOCK_SIZE)) 
/* == 16 */
int isamh_alloc_block (ISAMH is, int cat);
void isamh_release_block (ISAMH is, int cat, int pos);
int isamh_read_block (ISAMH is, int cat, int pos, char *dst);
int isamh_write_block (ISAMH is, int cat, int pos, char *src);

#ifdef __cplusplus
}
#endif



/*
 * $Log: isamh-p.h,v $
 * Revision 1.3  1999-07-07 09:36:04  heikki
 * Fixed an assertion in isamh
 *
 * Revision 1.2  1999/07/06 09:37:05  heikki
 * Working on isamh - not ready yet.
 *
 * Revision 1.1  1999/06/30 15:05:45  heikki
 * opied from isamc.p.h, starting to simplify
 *
 */