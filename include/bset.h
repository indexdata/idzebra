/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bset.h,v $
 * Revision 1.1  1994-09-26 10:17:42  adam
 * Dfa-module header files.
 *
 */
#ifndef BSET_H
#define BSET_H

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

#endif
