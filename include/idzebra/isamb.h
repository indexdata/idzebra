/* $Id: isamb.h,v 1.6 2006-05-10 08:13:20 adam Exp $
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
int isamb_pp_forward2(ISAMB_PP pp, void *buf, const void *untilbuf);

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

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

