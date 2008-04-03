/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

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

#ifndef ISAMB_H
#define ISAMB_H

#include <idzebra/bfile.h>
#include <idzebra/isamc.h>

YAZ_BEGIN_CDECL

typedef struct ISAMB_s *ISAMB;
typedef struct ISAMB_PP_s *ISAMB_PP;

YAZ_EXPORT
ISAMB isamb_open(BFiles bfs, const char *name, int writeflag, ISAMC_M *method,
                  int cache);

YAZ_EXPORT
ISAMB isamb_open2(BFiles bfs, const char *name, int writeflag, ISAMC_M *method,
                  int cache, int no_cat, int *sizes, int use_root_ptr);

YAZ_EXPORT
void isamb_close(ISAMB isamb);

YAZ_EXPORT 
void isamb_merge(ISAMB b, ISAM_P *pos, ISAMC_I *data);

YAZ_EXPORT
ISAMB_PP isamb_pp_open(ISAMB isamb, ISAM_P pos, int scope);

YAZ_EXPORT
int isamb_pp_read(ISAMB_PP pp, void *buf);

YAZ_EXPORT
int isamb_pp_forward(ISAMB_PP pp, void *buf, const void *untilbuf);

YAZ_EXPORT
void isamb_pp_pos(ISAMB_PP pp, double *current, double *total);

YAZ_EXPORT
void isamb_pp_close(ISAMB_PP pp);

YAZ_EXPORT
int isamb_unlink(ISAMB b, ISAM_P pos);

YAZ_EXPORT
ISAMB_PP isamb_pp_open_x(ISAMB isamb, ISAM_P pos, int *level, int scope);
YAZ_EXPORT
void isamb_pp_close_x(ISAMB_PP pp, zint *size, zint *blocks);

YAZ_EXPORT
int isamb_block_info(ISAMB isamb, int cat);

YAZ_EXPORT
void isamb_dump(ISAMB b, ISAM_P pos, void (*pr)(const char *str));

YAZ_EXPORT
zint isamb_get_int_splits(ISAMB b);

YAZ_EXPORT
zint isamb_get_leaf_splits(ISAMB b);

YAZ_EXPORT
void isamb_set_int_count(ISAMB b, int v);

YAZ_EXPORT
void isamb_set_cache_size(ISAMB b, int sz);

YAZ_EXPORT
zint isamb_get_root_ptr(ISAMB b);

YAZ_EXPORT
void isamb_set_root_ptr(ISAMB b, zint root_ptr);


YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

