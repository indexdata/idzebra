/* $Id: rsprox.h,v 1.1 2004-06-09 12:15:25 adam Exp $
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

#ifndef RSET_PROX_H
#define RSET_PROX_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_prox;

typedef struct rset_prox_parms
{
    RSET *rset;
    int rset_no;
    int ordered;
    int exclusion;
    int relation;
    int distance;
    int key_size;
    int (*cmp)(const void *p1, const void *p2);
    int (*getseq)(const void *p);
    void (*log_item)(int logmask, const void *a, const char *txt);
} rset_prox_parms;

#ifdef __cplusplus
}
#endif

#endif