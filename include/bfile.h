/* $Id: bfile.h,v 1.22 2004-08-04 08:35:23 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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

#ifndef BFILE_H
#define BFILE_H

#include <mfile.h>

YAZ_BEGIN_CDECL

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
int bf_read (BFile bf, zint no, int offset, int nbytes, void *buf);

/* bf_write: writes bytes to bfile 'bf'.
   writes 'nbytes' bytes (or whole block if 0) at offset 'offset' to
   block 'no'. retrieves contents from buffer 'buf'.
   returns 0 if successful; non-zero otherwise.
 */
int bf_write (BFile bf, zint no, int offset, int nbytes, const void *buf);

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

YAZ_END_CDECL

#endif
