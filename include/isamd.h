/*
 * Copyright (c) 1995-1997, Index Data.
 * See the file LICENSE for details.
 *
 * IsamH is a simple ISAM that can only append to the end of the list.
 * It will need a clean-up process occasionally...  Code stolen from
 * isamc...
 * 
 * Heikki Levanto
 *
 * Detailed log at the end of the file
 *
 */

#ifndef ISAMD_H
#define ISAMD_H

#include <bfile.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ISAMD_s *ISAMD;
typedef int ISAMD_P;
typedef struct ISAMD_PP_s *ISAMD_PP;

typedef struct ISAMD_filecat_s {  /* filecategories, mostly block sizes */
    int bsize;         /* block size */
    int mblocks;       /* maximum keys before switching to larger sizes */
} *ISAMD_filecat;

typedef struct ISAMD_M_s {
    ISAMD_filecat filecat;

    int (*compare_item)(const void *a, const void *b);

#define ISAMD_DECODE 0
#define ISAMD_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);
    void (*code_reset)(void *p);

    int max_blocks_mem;
    int debug;
} *ISAMD_M;

typedef struct ISAMD_I_s {  /* encapsulation of input data */
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} *ISAMD_I;

ISAMD_M isamd_getmethod (ISAMD_M me);

ISAMD isamd_open (BFiles bfs, const char *name, int writeflag, ISAMD_M method);
int isamd_close (ISAMD is);
ISAMD_P isamd_append (ISAMD is, ISAMD_P pos, ISAMD_I data);
  /* corresponds to isc_merge */
  
  
ISAMD_PP isamd_pp_open (ISAMD is, ISAMD_P pos);
void isamd_pp_close (ISAMD_PP pp);
int isamd_read_item (ISAMD_PP pp, char **dst);
int isamd_pp_read (ISAMD_PP pp, void *buf);
int isamd_pp_num (ISAMD_PP pp);

int isamd_block_used (ISAMD is, int type);
int isamd_block_size (ISAMD is, int type);


#define isamd_type(x) ((x) & 7)
#define isamd_block(x) ((x) >> 3)
#define isamd_addr(blk,typ) (((blk)<<3)+(typ))

void isamd_buildfirstblock(ISAMD_PP pp);
void isamd_buildlaterblock(ISAMD_PP pp);

#ifdef __cplusplus
}
#endif

#endif  /* ISAMD_H */


/*
 * $Log: isamd.h,v $
 * Revision 1.1  1999/07/14 12:34:43  heikki
 * Copied from isamh, starting to change things...
 *
 *
 */
