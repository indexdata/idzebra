/* $Id$
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef ISAMD_H
#define ISAMD_H

#include <bfile.h>

YAZ_BEGIN_CDECL

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
    void (*log_item)(int logmask, const void *p, const char *txt);

#define ISAMD_DECODE 0
#define ISAMD_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);
    void (*code_reset)(void *p);

    int max_blocks_mem;
    int debug;
} ISAMD_M;

typedef struct ISAMD_I_s {  /* encapsulation of input data */
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} *ISAMD_I;

ISAMD_M *isamd_getmethod (ISAMD_M *me);

ISAMD isamd_open (BFiles bfs, const char *name, int writeflag, ISAMD_M *method);
int isamd_close (ISAMD is);
/*ISAMD_P isamd_append (ISAMD is, ISAMD_P pos, ISAMD_I data);*/
int isamd_append (ISAMD is, char *dictentry, int dictlen, ISAMD_I data);

  
  
/* Shortcut: If the isam is relatively short, we store the */
/* whole thing in the dictionary, and allocate no blocks at all! */
#define ISAMD_MAX_DICT_LEN 16

/*ISAMD_PP isamd_pp_open (ISAMD is, const char *dictbuf);*/
ISAMD_PP isamd_pp_open (ISAMD is, const char *dictbuf, int dictlen);
ISAMD_PP isamd_pp_create (ISAMD is, int cat);

void isamd_pp_close (ISAMD_PP pp);
int isamd_read_item (ISAMD_PP pp, char **dst);
int isamd_read_main_item (ISAMD_PP pp, char **dst);
int isamd_pp_read (ISAMD_PP pp, void *buf);
int isamd_pp_num (ISAMD_PP pp);

int isamd_block_used (ISAMD is, int type);
int isamd_block_size (ISAMD is, int type);


#define isamd_type(x) ((x) & 7)
#define isamd_block(x) ((x) >> 3)
#define isamd_addr(blk,typ) (((blk)<<3)+(typ))

void isamd_buildfirstblock(ISAMD_PP pp);
void isamd_buildlaterblock(ISAMD_PP pp);

YAZ_END_CDECL

#endif  /* ISAMD_H */


/*
 * $Log: isamd.h,v $
 * Revision 1.3  1999/08/18 08:33:41  heikki
 * Fixes
 *
 * Revision 1.2  1999/07/14 13:21:34  heikki
 * Added isam-d files. Compiles (almost) clean. Doesn't work at all
 *
 * Revision 1.1  1999/07/14 12:34:43  heikki
 * Copied from isamh, starting to change things...
 *
 *
 */
