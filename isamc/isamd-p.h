/* $Id: isamd-p.h,v 1.6 1999-08-25 18:09:23 heikki Exp $
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 *
 * log at the end
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
    
    int no_writes;  /* statistics, to be used properly... */
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

    int no_op_nodiff; /* existing blocks opened for reading without diffs */
    int no_op_intdiff; /* - with internal diffs */
    int no_op_extdiff; /* with separate diff blocks */
    int no_fbuilds;    /* number of first-time builds */
    int no_appds;      /* number of appends */
    int no_merges;     /* number of merges done */
    int no_remerges;   /* number of times more than one merge needed */

    char *alloc_buf;    /* free-list handling (?) */
    int alloc_entries_num;
    int alloc_entries_max;

    int fc_max;
    int *fc_list;
} *ISAMD_file;

struct ISAMD_s {
    int no_files;
    int max_cat;
    ISAMD_M method;
    ISAMD_file files;
}; 


typedef struct ISAMD_DIFF_s *ISAMD_DIFF;

struct ISAMD_PP_s {
    char *buf;   /* buffer for read/write operations */
    ISAMD_BLOCK_SIZE offset; /* position for next read/write */
    ISAMD_BLOCK_SIZE size;   /* size of actual data */
    int cat;  /* category of this block */
    int pos;  /* block number of this block */
    int next; /* number of the next block */
    int diffs; /* either block or offset (in head) of start of diffs */
               /* will not be used in the improved version! */
    ISAMD is;
    void *decodeClientData;  /* delta-encoder's own data */
    ISAMD_DIFF diffinfo;
    char *diffbuf; /* buffer for the diff block */
    int numKeys;
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
void isamd_free_diffs(ISAMD_PP pp);

#ifdef __cplusplus
}
#endif



/*
 * $Log: isamd-p.h,v $
 * Revision 1.6  1999-08-25 18:09:23  heikki
 * Starting to optimize
 *
 * Revision 1.5  1999/08/20 12:25:58  heikki
 * Statistics in isamd
 *
 * Revision 1.4  1999/07/21 14:24:50  heikki
 * isamd write and read functions ok, except when diff block full.
 * (merge not yet done)
 *
 * Revision 1.3  1999/07/14 15:05:30  heikki
 * slow start on isam-d
 *
 * Revision 1.1  1999/07/14 12:34:43  heikki
 * Copied from isamh, starting to change things...
 *
 *
 */