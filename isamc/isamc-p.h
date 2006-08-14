/* $Id: isamc-p.h,v 1.9.2.1 2006-08-14 10:39:10 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

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
    ISAMC_M *method;
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

