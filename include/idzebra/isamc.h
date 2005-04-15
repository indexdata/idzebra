/* $Id: isamc.h,v 1.4 2005-04-15 10:47:48 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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

#include <idzebra/isam-codec.h>
#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

typedef zint ISAM_P;

typedef struct ISAMC_s *ISAMC;
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

    ISAM_CODEC codec;

    int max_blocks_mem;
    int debug;
} ISAMC_M;

typedef struct ISAMC_I_s {
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} ISAMC_I;

YAZ_EXPORT
void isamc_getmethod (ISAMC_M *m);

YAZ_EXPORT
ISAMC isamc_open (BFiles bfs, const char *name, int writeflag,
		  ISAMC_M *method);
YAZ_EXPORT
int isamc_close (ISAMC is);

YAZ_EXPORT
void isamc_merge (ISAMC is, ISAM_P *pos, ISAMC_I *data);

YAZ_EXPORT
ISAMC_PP isamc_pp_open (ISAMC is, ISAM_P pos);

YAZ_EXPORT
void isamc_pp_close (ISAMC_PP pp);

YAZ_EXPORT
int isamc_read_item (ISAMC_PP pp, char **dst);

YAZ_EXPORT
int isamc_pp_read (ISAMC_PP pp, void *buf);

YAZ_EXPORT
zint isamc_pp_num (ISAMC_PP pp);

YAZ_EXPORT
zint isamc_block_used (ISAMC is, int type);

YAZ_EXPORT
int isamc_block_size (ISAMC is, int type);

#define isamc_type(x) ((x) & 7)
#define isamc_block(x) ((x) >> 3)

YAZ_END_CDECL

#endif
