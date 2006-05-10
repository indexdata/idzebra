/* $Id: dfaset.h,v 1.2 2006-05-10 08:13:18 adam Exp $
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

#ifndef DFASET_H
#define DFASET_H

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

typedef struct DFASetElement_  {
    struct DFASetElement_ *next;
    int value;
} DFASetElement, *DFASet;

typedef struct {
    DFASet  alloclist;
    DFASet  freelist;
    long used;
    int  chunk;
} *DFASetType;

DFASetType  mk_DFASetType (int chunk);
int         inf_DFASetType(DFASetType st, long *used, long *allocated);
DFASetType  rm_DFASetType (DFASetType st);
DFASet      mk_DFASet     (DFASetType st);
DFASet      add_DFASet    (DFASetType st, DFASet s, int value);
DFASet      merge_DFASet  (DFASetType st, DFASet s1, DFASet s2);
DFASet      union_DFASet  (DFASetType st, DFASet s1, DFASet s2);
DFASet      rm_DFASet     (DFASetType st, DFASet s);
DFASet      cp_DFASet     (DFASetType st, DFASet s);
void        pr_DFASet     (DFASetType st, DFASet s);
unsigned    hash_DFASet   (DFASetType st, DFASet s);
int         eq_DFASet     (DFASetType s, DFASet s1, DFASet s2);

YAZ_END_CDECL
#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

