/* $Id: rsisamb.h,v 1.2.2.1 2005-01-14 14:32:25 adam Exp $
   Copyright (C) 1995-2005
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



#ifndef RSET_ISAMB_H
#define RSET_ISAMB_H

#include <rset.h>
#include <isamb.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_isamb;
extern const struct rset_control *rset_kind_isamb_forward;

typedef struct rset_isamb_parms
{
    int (*cmp)(const void *p1, const void *p2);
    int key_size;
    ISAMB is;
    ISAMB_P pos;
    RSET_TERM rset_term;
} rset_isamb_parms;

#ifdef __cplusplus
}
#endif

#endif
