/* $Id: bset.h,v 1.4 2002-08-02 19:26:55 adam Exp $
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


#ifndef BSET_H
#define BSET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short BSetWord;
typedef BSetWord *BSet;

typedef struct BSetHandle_ {
    unsigned size;        /* size of set in members */
    unsigned wsize;       /* size of individual set (in BSetWord)*/
    unsigned offset;      /* offset in current set block */
    unsigned chunk;       /* chunk, i.e. size of each block */
    struct BSetHandle_ *setchain;
    BSetWord setarray[1];
} BSetHandle;

BSetHandle *mk_BSetHandle (int size, int chunk);
void       rm_BSetHandle  (BSetHandle **shp);
int        inf_BSetHandle (BSetHandle *sh, long *used, long *alloc);
BSet       cp_BSet        (BSetHandle *sh, BSet dst, BSet src);
void       add_BSet       (BSetHandle *sh, BSet dst, unsigned member);
void       union_BSet     (BSetHandle *sh, BSet dst, BSet src);
BSet       mk_BSet        (BSetHandle **shp);
void       rm_BSet        (BSetHandle **shp);
void       res_BSet       (BSetHandle *sh, BSet dst);
void       com_BSet       (BSetHandle *sh, BSet dst);
void       pr_BSet        (BSetHandle *sh, BSet src);
unsigned   test_BSet      (BSetHandle *sh, BSet src, unsigned member);
int        trav_BSet      (BSetHandle *sh, BSet src, unsigned member);
int        travi_BSet     (BSetHandle *sh, BSet src, unsigned member);
unsigned   hash_BSet      (BSetHandle *sh, BSet src);
int        eq_BSet        (BSetHandle *sh, BSet dst, BSet src);
void       pr_charBSet    (BSetHandle *sh, BSet src, void (*f)(int));

#ifdef __cplusplus
}
#endif

#endif
