/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: bfile.h,v 1.20 2002-04-04 14:14:13 adam Exp $
 */

#ifndef BFILE_H
#define BFILE_H

#include <mfile.h>

#ifdef __cplusplus
extern "C" {
#endif

#define bf_blocksize(bf) mf_blocksize(bf->mf)

typedef struct BFiles_struct *BFiles;

BFiles bfs_create (const char *spec, const char *base);
void bfs_destroy (BFiles bfiles);

typedef struct BFile_struct
{
    MFile mf;
    Zebra_lock_rdwr rdwr_lock;
    struct CFile_struct *cf;
} *BFile, BFile_struct;

/* bf_close: closes bfile.
   returns 0 if successful; non-zero otherwise 
 */
int bf_close (BFile);

/* bf_open: opens bfile.
   opens bfile with name 'name' and with 'block_size' as block size.
   returns bfile handle is successful; NULL otherwise 
 */
BFile bf_open (BFiles bfs, const char *name, int block_size, int wflag);

/* bf_read: reads bytes from bfile 'bf'.
   reads 'nbytes' bytes (or whole block if 0) from offset 'offset' from
   block 'no'. stores contents in buffer 'buf'.
   returns 1 if whole block could be read; 0 otherwise.
 */
int bf_read (BFile bf, int no, int offset, int nbytes, void *buf);

/* bf_write: writes bytes to bfile 'bf'.
   writes 'nbytes' bytes (or whole block if 0) at offset 'offset' to
   block 'no'. retrieves contents from buffer 'buf'.
   returns 0 if successful; non-zero otherwise.
 */
int bf_write (BFile bf, int no, int offset, int nbytes, const void *buf);

/* bf_cache: enables bfile cache if spec is not NULL */
void bf_cache (BFiles bfs, const char *spec);

/* bf_commitExists: returns 1 if commit is pending; 0 otherwise */
int bf_commitExists (BFiles bfs);

/* bf_commitExec: executes commit */
void bf_commitExec (BFiles bfs);

/* bf_commitClean: cleans commit files, etc */
void bf_commitClean (BFiles bfs, const char *spec);

/* bf_reset: delete register and shadow completely */
void bf_reset (BFiles bfs);

#ifdef __cplusplus
}
#endif

#endif
