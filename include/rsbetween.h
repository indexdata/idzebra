/* $Id: rsbetween.h,v 1.6 2004-08-24 14:25:15 heikki Exp $
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



#ifndef RSET_BETWEEN_H
#define RSET_BETWEEN_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

RSET rsbetween_create( NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            RSET rset_l, RSET rset_m, RSET rset_r, RSET rset_attr,
            char *(*printer)(const void *p1, char *buf) );

extern const struct rset_control *rset_kind_between;

#ifdef __cplusplus
}
#endif

#endif
