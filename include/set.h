/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: set.h,v $
 * Revision 1.3  1999-02-02 14:50:44  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.2  1997/09/05 15:30:05  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.1  1994/09/26 10:17:44  adam
 * Dfa-module header files.
 *
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

