/* $Id: set.h,v 1.5 2005-01-15 19:38:24 adam Exp $
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


#ifndef SET_H
#define SET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SetElement_  {
    struct SetElement_ *next;
    int value;
} SetElement, *Set;

typedef struct {
    Set  alloclist;
    Set  freelist;
    long used;
    int  chunk;
} *SetType;

SetType  mk_SetType   (int chunk);
int      inf_SetType  (SetType st, long *used, long *allocated);
SetType  rm_SetType   (SetType st);
Set      mk_Set       (SetType st);
Set      add_Set      (SetType st, Set s, int value);
Set      merge_Set    (SetType st, Set s1, Set s2);
Set      union_Set    (SetType st, Set s1, Set s2);
Set      rm_Set       (SetType st, Set s);
Set      cp_Set       (SetType st, Set s);
void     pr_Set       (SetType st, Set s);
unsigned hash_Set     (SetType st, Set s);
int      eq_Set       (SetType s, Set s1, Set s2);

#ifdef __cplusplus
}
#endif

#endif

