/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.h,v $
 * Revision 1.15  1997-09-17 12:19:07  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.14  1997/09/05 15:29:58  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.13  1996/10/29 13:43:07  adam
 * Added a few comments.
 *
 * Revision 1.12  1996/03/26 16:00:44  adam
 * The directory of the shadow table can be specified by the new
 * bf_lockDir call.
 *
 * Revision 1.11  1995/12/08  16:20:39  adam
 * New commit utilities - used for 'save' update.
 *
 * Revision 1.10  1995/12/01  16:24:33  adam
 * Commit files use separate meta file area.
 *
 * Revision 1.9  1995/12/01  11:37:46  adam
 * Cached/commit files implemented as meta-files.
 *
 * Revision 1.8  1995/11/30  08:33:29  adam
 * Started work on commit facility.
 *
 * Revision 1.7  1995/09/04  12:33:35  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.6  1994/09/14  13:10:35  quinn
 * Small changes
 *
 * Revision 1.5  1994/08/24  08:45:52  quinn
 * Using mfile.
 *
 * Revision 1.4  1994/08/17  15:38:28  adam
 * Include of util.h.
 *
 * Revision 1.3  1994/08/17  14:09:47  quinn
 * Small changes
 *
 */

#ifndef BFILE_H
#define BFILE_H

#include <mfile.h>

#ifdef __cplusplus
extern "C" {
#endif

#define bf_blocksize(bf) mf_blocksize(bf->mf)

typedef struct BFiles_struct *BFiles;

BFiles bfs_create (const char *spec);
void bfs_destroy (BFiles bfiles);

typedef struct BFile_struct
{
    MFile mf;
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
   reads 'num' bytes (or whole block if 0) from offset 'offset' from
   block 'no'. stores contents in buffer 'buf'.
   returns 1 if whole block could be read; 0 otherwise.
 */
int bf_read (BFile bf, int no, int offset, int num, void *buf);

/* bf_write: writes bytes to bfile 'bf'.
   writes 'num' bytes (or whole block if 0) at offset 'offset' to
   block 'no'. retrieves contents from buffer 'buf'.
   returns 0 if successful; non-zero otherwise.
 */
int bf_write (BFile bf, int no, int offset, int num, const void *buf);

/* bf_cache: enables bfile cache if spec is not NULL */
void bf_cache (BFiles bfs, const char *spec);

/* bf_lockDir: specifies locking directory for the cache system */
void bf_lockDir (BFiles bfs, const char *lockDir);

/* bf_commitExists: returns 1 if commit is pending; 0 otherwise */
int bf_commitExists (BFiles bfs);

/* bf_commitExec: executes commit */
void bf_commitExec (BFiles bfs);

/* bf_commitClean: cleans commit files, etc */
void bf_commitClean (BFiles bfs, const char *spec);

#ifdef __cplusplus
}
#endif

#endif
