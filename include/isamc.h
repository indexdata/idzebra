/* $Id: isamc.h,v 1.12 2004-06-01 12:56:38 adam Exp $
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



#ifndef ISAMC_H
#define ISAMC_H

#include <bfile.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ISAMC_s *ISAMC;
typedef int ISAMC_P;
typedef struct ISAMC_PP_s *ISAMC_PP;

typedef struct ISAMC_filecat_s {
    int bsize;         /* block size */
    int ifill;         /* initial fill */
    int mfill;         /* minimum fill */
    int mblocks;       /* maximum blocks */
} *ISAMC_filecat;

typedef struct ISAMC_M_s {
    ISAMC_filecat filecat;

    int (*compare_item)(const void *a, const void *b);
    void (*log_item)(int logmask, const void *p, const char *txt);

#define ISAMC_DECODE 0
#define ISAMC_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);
    void (*code_reset)(void *p);

    int max_blocks_mem;
    int debug;
} ISAMC_M;

typedef struct ISAMC_I_s {
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} ISAMC_I;

void isc_getmethod (ISAMC_M *m);

ISAMC isc_open (BFiles bfs, const char *name, int writeflag, ISAMC_M *method);
int isc_close (ISAMC is);
ISAMC_P isc_merge (ISAMC is, ISAMC_P pos, ISAMC_I *data);

ISAMC_PP isc_pp_open (ISAMC is, ISAMC_P pos);
void isc_pp_close (ISAMC_PP pp);
int isc_read_item (ISAMC_PP pp, char **dst);
int isc_pp_read (ISAMC_PP pp, void *buf);
int isc_pp_num (ISAMC_PP pp);

int isc_block_used (ISAMC is, int type);
int isc_block_size (ISAMC is, int type);

#define isc_type(x) ((x) & 7)
#define isc_block(x) ((x) >> 3)

#ifdef __cplusplus
}
#endif

#endif
