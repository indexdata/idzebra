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
    char *merge_buf;
    ISAMH_M method;
    ISAMH_file files;
}; 

struct ISAMH_PP_s {
    char *buf;
    ISAMH_BLOCK_SIZE offset;
    ISAMH_BLOCK_SIZE size;
    int cat;
    int pos;
    int next;
    ISAMH is;
    void *decodeClientData;
    int deleteFlag;
    int numKeys;
};

#define ISAMH_BLOCK_OFFSET_N (sizeof(int)+sizeof(ISAMH_BLOCK_SIZE)) 
#define ISAMH_BLOCK_OFFSET_1 (sizeof(int)+sizeof(ISAMH_BLOCK_SIZE)+sizeof(int)) 
int isamh_alloc_block (ISAMH is, int cat);
void isamh_release_block (ISAMH is, int cat, int pos);
int isamh_read_block (ISAMH is, int cat, int pos, char *dst);
int isamh_write_block (ISAMH is, int cat, int pos, char *src);

#ifdef __cplusplus
}
#endif



/*
 * $Log: isamh-p.h,v $
 * Revision 1.1  1999-06-30 15:05:45  heikki
 * opied from isamc.p.h, starting to simplify
 *
 */