/* $Id: rsprox.h,v 1.1.2.1 2006-08-14 10:38:56 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

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
