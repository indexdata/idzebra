/* $Id: isamd-p.h,v 1.11 2002-08-02 19:26:56 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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

    int no_forward;  /* stats from pp_read, for isam-c compatibility */
    int no_backward;
    int sum_forward;
    int sum_backward;
    int no_next;
    int no_prev;

    int no_op_diffonly;/* number of opens without mainblock */
    int no_op_main;    /* number of opens with a main block */

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
    int last_pos;   /* last read/write position for seek stats */
    int last_cat;   /* same for category */
    int no_read;    /* blocks read (in all categories) */
    int no_write;   /* blocks written (in all categories) */
    int no_op_single;/* singleton "blocks" opened */
    int no_read_keys;/* number of keys read (occurences of words) */
    int no_read_main;/* number of main keys read (not diffs) */
    int no_read_eof; /* number of key sequence ends read (no of words read) */
    int no_seek_nxt; /* seeks to the next record (fast) */
    int no_seek_sam; /* seeks to same record (fast) */
    int no_seek_fwd; /* seeks forward */
    int no_seek_prv; /* seeks to previous */
    int no_seek_bak; /* seeks backwards */
    int no_seek_cat; /* seeks to different category (expensive) */
    int no_op_new;  /* "open"s for new blocks */
    int no_fbuilds;    /* number of first-time builds */
    int no_appds;      /* number of appends */
    int no_merges;     /* number of merges done */
    int no_non;     /* merges without any work */
    int no_singles; /* new items resulting in singletons */
}; 


typedef struct ISAMD_DIFF_s *ISAMD_DIFF;

struct ISAMD_PP_s {
    char *buf;   /* buffer for read/write operations */
    ISAMD_BLOCK_SIZE offset; /* position for next read/write */
    ISAMD_BLOCK_SIZE size;   /* size of actual data */
    int cat;  /* category of this block */
    int pos;  /* block number of this block */
    int next; /* number of the next block */
    int diffs; /* not used in the modern isam-d, but kept for stats compatibility */
               /* never stored on disk, though */
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
                              sizeof(ISAMD_BLOCK_SIZE)) 
/* == 12  (was 16) */
//                              sizeof(int) + 


int isamd_alloc_block (ISAMD is, int cat);
void isamd_release_block (ISAMD is, int cat, int pos);
int isamd_read_block (ISAMD is, int cat, int pos, char *dst);
int isamd_write_block (ISAMD is, int cat, int pos, char *src);
void isamd_free_diffs(ISAMD_PP pp);

int is_singleton(ISAMD_P ipos);
void singleton_decode (int code, struct it_key *k);
int singleton_encode(struct it_key *k);


#ifdef __cplusplus
}
#endif



/*
 * $Log: isamd-p.h,v $
 * Revision 1.11  2002-08-02 19:26:56  adam
 * Towards GPL
 *
 * Revision 1.10  2002/04/29 18:10:24  adam
 * Newline at end of file
 *
 * Revision 1.9  1999/10/05 09:57:40  heikki
 * Tuning the isam-d (and fixed a small "detail")
 *
 * Revision 1.8  1999/09/23 18:01:18  heikki
 * singleton optimising
 *
 * Revision 1.7  1999/09/20 15:48:06  heikki
 * Small changes
 *
 * Revision 1.6  1999/08/25 18:09:23  heikki
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

