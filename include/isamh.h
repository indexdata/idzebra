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

#ifndef ISAMH_H
#define ISAMH_H

#include <bfile.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ISAMH_s *ISAMH;
typedef int ISAMH_P;
typedef struct ISAMH_PP_s *ISAMH_PP;

typedef struct ISAMH_filecat_s {  /* filecategories, mostly block sizes */
    int bsize;         /* block size */
    int mblocks;       /* maximum blocks */
} *ISAMH_filecat;

typedef struct ISAMH_M_s {
    ISAMH_filecat filecat;

    int (*compare_item)(const void *a, const void *b);

#define ISAMH_DECODE 0
#define ISAMH_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);
    void (*code_reset)(void *p);

    int max_blocks_mem;
    int debug;
} *ISAMH_M;

typedef struct ISAMH_I_s {  /* encapsulation of input data */
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} *ISAMH_I;

ISAMH_M isamh_getmethod (void);

ISAMH isamh_open (BFiles bfs, const char *name, int writeflag, ISAMH_M method);
int isamh_close (ISAMH is);
ISAMH_P isamh_append (ISAMH is, ISAMH_P pos, ISAMH_I data);
  /* corresponds to isc_merge */
  
  
ISAMH_PP isamh_pp_open (ISAMH is, ISAMH_P pos);
void isamh_pp_close (ISAMH_PP pp);
int isamh_read_item (ISAMH_PP pp, char **dst);
int isamh_pp_read (ISAMH_PP pp, void *buf);
int isamh_pp_num (ISAMH_PP pp);

int isamh_block_used (ISAMH is, int type);
int isamh_block_size (ISAMH is, int type);

#define isamh_type(x) ((x) & 7)
#define isamh_block(x) ((x) >> 3)


#ifdef __cplusplus
}
#endif

#endif  /* ISAMH_H */


/*
 * $Log: isamh.h,v $
 * Revision 1.1  1999-06-30 15:06:28  heikki
 * copied from isamc.h, simplifying
 *
 */
