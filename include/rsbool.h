/* $Id: rsbool.h,v 1.8 2002-08-02 19:26:55 adam Exp $
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



#ifndef RSET_BOOL_H
#define RSET_BOOL_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_and;
extern const struct rset_control *rset_kind_or;
extern const struct rset_control *rset_kind_not;

typedef struct rset_bool_parms
{
    int     key_size;
    RSET    rset_l;
    RSET    rset_r;
    int (*cmp)(const void *p1, const void *p2);
} rset_bool_parms;

#ifdef __cplusplus
}
#endif

#endif
