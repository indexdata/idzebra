/* $Id: isamg.h,v 1.2 2002-08-02 19:26:55 adam Exp $
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



#ifndef ISAMG_H
#define ISAMG_H

/* #include <bfile.h> */

YAZ_BEGIN_CDECL

typedef struct ISAMG_s *ISAMG;
typedef int ISAMG_P;
typedef struct ISAMG_PP_s *ISAMG_PP;

#ifdef SKIPTHIS
/* File categories etc are no business of users of ISAM-G (or what?) */
typedef struct ISAMG_filecat_s {  /* filecategories, mostly block sizes */
    int bsize;         /* block size */
    int mblocks;       /* maximum keys before switching to larger sizes */
} *ISAMG_filecat;
#endif 


#ifdef SKIPTHIS
/* The (delta?)encoding will be encapsulated so deep in the isams that */
/* nobody needs to see it here... If different encodings needed, make */
/* new isam types! */

typedef struct ISAMG_M_s {
    ISAMG_filecat filecat;

    int (*compare_item)(const void *a, const void *b);

#define ISAMG_DECODE 0
#define ISAMG_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);
    void (*code_reset)(void *p);

    int max_blocks_mem;
    int debug;
} *ISAMG_M;
#endif

#ifdef SKIPTHIS
/* Is this needed? Shouldn't they all be using the same encapsulation */

typedef struct ISAMG_I_s {  /* encapsulation of input data */
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} *ISAMG_I;
#endif

#ifdef SKIPTHIS 
ISAMG_M isamg_getmethod (ISAMG_M me);
/* Does anyone really need this method thing ?? */
#endif

ISAMG isamg_open (BFiles bfs, int writeflag, char *isam_type_name, Res res);

int isamg_close (ISAMG is);

ISAMG_P isamg_append (ISAMG is, ISAMG_P pos, ISAMG_I data);
/* This may be difficult to generalize! */
  
  
ISAMG_PP isamg_pp_open (ISAMG is, ISAMG_P pos);
void isamg_pp_close (ISAMG_PP pp);
/* int isamg_read_item (ISAMG_PP pp, char **dst);      */
/* int isamg_read_main_item (ISAMG_PP pp, char **dst); */
int isamg_pp_read (ISAMG_PP pp, void *buf);
int isamg_pp_num (ISAMG_PP pp);

#ifdef SKIPTHIS
/* all internal stuff */
int isamg_block_used (ISAMG is, int type);
int isamg_block_size (ISAMG is, int type);


#define isamg_type(x) ((x) & 7)
#define isamg_block(x) ((x) >> 3)
#define isamg_addr(blk,typ) (((blk)<<3)+(typ))

void isamg_buildfirstblock(ISAMG_PP pp);
void isamg_buildlaterblock(ISAMG_PP pp);
#endif

YAZ_END_CDECL

#endif  /* ISAMG_H */


/*
 * $Log: isamg.h,v $
 * Revision 1.2  2002-08-02 19:26:55  adam
 * Towards GPL
 *
 * Revision 1.1  2001/01/16 19:05:11  heikki
 * Started to add isamg
 *
 *
 *
 */
