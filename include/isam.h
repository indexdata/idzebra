/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isam.h,v $
 * Revision 1.10  1995-09-06 16:10:57  adam
 * More work on boolean sets.
 *
 * Revision 1.9  1994/09/28  16:58:26  quinn
 * Small mod.
 *
 * Revision 1.8  1994/09/28  12:56:09  quinn
 * Added access functions (ISPT)
 *
 * Revision 1.7  1994/09/28  11:56:13  quinn
 * Removed const from input to is_merge
 *
 * Revision 1.6  1994/09/28  11:29:28  quinn
 * Added cmp parameter.
 *
 * Revision 1.5  1994/09/27  20:03:36  quinn
 * Seems relatively bug-free.
 *
 * Revision 1.4  1994/09/26  17:05:54  quinn
 * Trivial.
 *
 * Revision 1.3  1994/09/26  16:08:42  quinn
 * Most of the functionality in place.
 *
 * Revision 1.2  1994/09/14  13:10:35  quinn
 * Small changes
 *
 * Revision 1.1  1994/09/12  08:02:07  quinn
 * Not functional yet
 *
 */

#ifndef ISAM_H
#define ISAM_H

#include <bfile.h>

#include "../isam/memory.h"
#include "../isam/physical.h"

#define IS_MAX_BLOCKTYPES 4
#define IS_MAX_RECORD 512
#define IS_DEF_REPACK_PERCENT "30" /* how much relative change before repack */

typedef unsigned int SYSNO; /* should be somewhere else */

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
ISAM is_open(const char *name, int (*cmp)(const void *p1, const void *p2),
    int writeflag, int keysize);

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

#endif
