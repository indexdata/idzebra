/* $Id: isam.h,v 1.15 2002-08-02 19:26:55 adam Exp $
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



#ifndef ISAM_H
#define ISAM_H

#include <res.h>
#include <bfile.h>

#include "../isam/memory.h"
#include "../isam/physical.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IS_MAX_BLOCKTYPES 4
#define IS_MAX_RECORD 512
#define IS_DEF_REPACK_PERCENT "30" /* how much relative change before repack */

/*
 * Description of a blocktype (part of an isam file)
 */
typedef struct isam_blocktype
{
    BFile bf;                    /* blocked file */
    int blocksize;
    int first_block;             /* position of first data block */
    int max_keys_block;          /* max num of keys per block */
    int max_keys_block0;         /* max num of keys in first block */
    int nice_keys_block;         /* nice number of keys per block */
    int max_keys;                /* max number of keys per table */
    int freelist;                /* first free block */
    int top;                     /* first unused block */
    int index;                   /* placeholder. Always 0. */
    char *dbuf;                  /* buffer for use in I/O operations */
} isam_blocktype;

/*
 * Handle to an open isam complex.
 */
typedef struct isam_struct
{
    isam_blocktype types[IS_MAX_BLOCKTYPES]; /* block_types used in this file */
    int num_types;                /* number of block types used */
    int writeflag;
    int keysize;                  /* size of the keys (records) used */
    int repack;                   /* how many percent to grow before repack */
    int (*cmp)(const void *k1, const void *k2); /* compare function */
} isam_struct;

typedef struct ispt_struct
{
    struct is_mtable tab;
    struct ispt_struct *next;      /* freelist */
} ispt_struct, *ISPT; 

#define is_type(x) ((x) & 3)      /* type part of position */
#define is_block(x) ((x) >> 2)     /* block # part of position */

#define is_keysize(is) ((is)->keysize)

/*
 * Public Prototypes.
 *******************************************************************
 */

/*
 * Open isam file.
 */
ISAM is_open(BFiles bfs, const char *name,
	     int (*cmp)(const void *p1, const void *p2),
	     int writeflag, int keysize, Res res);

/*
 * Close isam file.
 */
int is_close(ISAM is);

/*
 * Locate a table of keys in an isam file. The ISPT is an individual
 * position marker for that table.
 */
ISPT is_position(ISAM is, ISAM_P pos);

/*
 * Release ISPT.
 */
void is_pt_free(ISPT ip);

/*
 * Read a key from a table.
 */
int is_readkey(ISPT ip, void *buf);

int is_writekey(ISPT ip, const void *buf);

int is_numkeys(ISPT ip);

void is_rewind(ISPT ip);

ISAM_P is_merge(ISAM is, ISAM_P pos, int num, char *data);

#ifdef __cplusplus
}
#endif

#endif
