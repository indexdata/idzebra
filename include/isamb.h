/* $Id: isamb.h,v 1.9 2004-06-02 12:30:32 adam Exp $
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

#ifndef ISAMB_H
#define ISAMB_H

#include <bfile.h>
#include <isamc.h>

typedef struct ISAMB_s *ISAMB;
typedef struct ISAMB_PP_s *ISAMB_PP;
typedef ISAMC_P ISAMB_P;

ISAMB isamb_open (BFiles bfs, const char *name, int writeflag, ISAMC_M *method,
                  int cache);
void isamb_close (ISAMB isamb);

ISAMB_P isamb_merge (ISAMB b, ISAMB_P pos, ISAMC_I *data);

ISAMB_PP isamb_pp_open (ISAMB isamb, ISAMB_P pos);

int isamb_pp_read (ISAMB_PP pp, void *buf);

int isamb_pp_forward (ISAMB_PP pp, void *buf, const void *untilbuf);

void isamb_pp_close (ISAMB_PP pp);

int isamb_unlink (ISAMB b, ISAMC_P pos);

int isamb_pp_num (ISAMB_PP pp);

ISAMB_PP isamb_pp_open_x (ISAMB isamb, ISAMB_P pos, int *level);
void isamb_pp_close_x (ISAMB_PP pp, int *size, int *blocks);

int isamb_block_info (ISAMB isamb, int cat);

void isamb_dump (ISAMB b, ISAMB_P pos, void (*pr)(const char *str));

#endif
