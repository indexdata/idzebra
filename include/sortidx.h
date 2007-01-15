/* $Id: sortidx.h,v 1.13 2007-01-15 20:08:24 adam Exp $
   Copyright (C) 1995-2007
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef SORTIDX_H
#define SORTIDX_H

#include <yaz/yconfig.h>
#include <idzebra/version.h>
#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

#define SORT_IDX_ENTRYSIZE 64

typedef struct zebra_sort_index *zebra_sort_index_t;

#define ZEBRA_SORT_TYPE_FLAT 1
#define ZEBRA_SORT_TYPE_ISAMB 2

zebra_sort_index_t zebra_sort_open(BFiles bfs, int write_flag, int sort_type);
void zebra_sort_close(zebra_sort_index_t si);
int zebra_sort_type(zebra_sort_index_t si, int type);
void zebra_sort_sysno(zebra_sort_index_t si, zint sysno);
void zebra_sort_add(zebra_sort_index_t si, const char *buf, int len);
void zebra_sort_delete(zebra_sort_index_t si);
void zebra_sort_read(zebra_sort_index_t si, char *buf);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

