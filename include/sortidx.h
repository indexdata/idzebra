/* $Id: sortidx.h,v 1.8 2005-01-15 21:45:42 adam Exp $
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

#ifndef SORTIDX_H
#define SORTIDX_H

#include <yaz/yconfig.h>
#include <idzebra/version.h>
#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

#define SORT_IDX_ENTRYSIZE 64

typedef struct sortIdx *SortIdx;

SortIdx sortIdx_open (BFiles bfs, int write_flag);
void sortIdx_close (SortIdx si);
int sortIdx_type (SortIdx si, int type);
void sortIdx_sysno (SortIdx si, SYSNO sysno);
void sortIdx_add (SortIdx si, const char *buf, int len);
void sortIdx_read (SortIdx si, char *buf);

YAZ_END_CDECL

#endif
