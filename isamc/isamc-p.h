/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isamc-p.h,v $
 * Revision 1.4  1996-11-08 11:15:28  adam
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

typedef struct {
    int lastblock;
    int freelist;
} ISAMC_head;

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
    unsigned offset;
    unsigned short size;
    int cat;
    int pos;
    int next;
    ISAMC is;
    void *decodeClientData;
    int deleteFlag;
    int numKeys;
};

#define ISAMC_BLOCK_OFFSET_N (sizeof(int)+sizeof(short)) 
#define ISAMC_BLOCK_OFFSET_1 (sizeof(int)+sizeof(short)+sizeof(int)) 

int isc_alloc_block (ISAMC is, int cat);
void isc_release_block (ISAMC is, int cat, int pos);
int isc_read_block (ISAMC is, int cat, int pos, char *dst);
int isc_write_block (ISAMC is, int cat, int pos, char *src);
