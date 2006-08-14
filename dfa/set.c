/* $Id: set.c,v 1.8.2.1 2006-08-14 10:38:53 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <set.h>
#include "imalloc.h"


static Set mk_SetElement (SetType st, int n);

SetType mk_SetType (int chunk)
{
    SetType st;

    assert (chunk > 8 && chunk < 8000);

    st = (SetType) imalloc (sizeof(*st));
    assert (st);

    st->alloclist = st->freelist = NULL;
    st->used = 0;
    st->chunk = chunk;
    return st;
}

int inf_SetType (SetType st, long *used, long *allocated)
{
    Set s;
    assert (st);
    *used = st->used;
    *allocated = 0;
    for (s = st->alloclist; s; s = s->next)
         *allocated += st->chunk;
    return sizeof (SetElement);
}

SetType rm_SetType (SetType st)
{
    Set s, s1;
    for (s = st->alloclist; (s1 = s);)
    {
        s = s->next;
        ifree (s1);
    }
    ifree (st);
    return NULL;
}

Set mk_Set (SetType st)
{
    assert (st);
    return NULL;
}

static Set mk_SetElement (SetType st, int n)
{
    Set s;
    int i;
    assert (st);

    assert (st->chunk > 8);
    if (! st->freelist)
    {
        s = (Set) imalloc (sizeof(*s) * (1+st->chunk));
        assert (s);
        s->next = st->alloclist;
        st->alloclist = s;
        st->freelist = ++s;
        for (i=st->chunk; --i > 0; s++)
            s->next = s+1;
        s->next = NULL;
    }
    st->used++;
    s = st->freelist;
    st->freelist = s->next;
    s->value = n;
    return s;
}

#if 0
static void rm_SetElement (SetType st, SetElement *p)
{
    assert (st);
    assert (st->used > 0);
    p->next = st->freelist;
    st->freelist = p;
    st->used--;
}
#endif

Set rm_Set (SetType st, Set s)
{
    Set s1 = s;
    int i = 1;

    if (s)
    {
        while (s->next)
        {
            s = s->next;
            ++i;
        }
        s->next = st->freelist;
        st->freelist = s1;
        st->used -= i;
        assert (st->used >= 0);
    }
    return NULL;
}

Set add_Set (SetType st, Set s, int n)
{
    SetElement dummy;
    Set p = &dummy, snew;
    p->next = s;
    while (p->next && p->next->value < n)
        p = p->next;
    assert (p);
    if (!(p->next && p->next->value == n))
    {
        snew = mk_SetElement (st, n);
        snew->next = p->next;
        p->next = snew;
    }
    return dummy.next;
}

Set union_Set (SetType st, Set s1, Set s2)
{
    SetElement dummy;
    Set p;
    assert (st);

    for (p = &dummy; s1 && s2;)
        if (s1->value < s2->value)
        {
            p = p->next = s1;
            s1 = s1->next;
        }
        else if (s1->value > s2->value)
        {
            p = p->next = mk_SetElement (st, s2->value);
            s2 = s2->next;
        }
        else
        {
            p = p->next = s1;
            s1 = s1->next;
            s2 = s2->next;
        }
    if (s1)
        p->next = s1;
    else
    {
        while (s2)
        {
            p = p->next = mk_SetElement (st, s2->value);
            s2 = s2->next;
        }
        p->next = NULL;
    }
    return dummy.next;
}

Set cp_Set (SetType st, Set s)
{
    return merge_Set (st, s, NULL);
}

Set merge_Set (SetType st, Set s1, Set s2)
{
    SetElement dummy;
    Set p;
    assert (st);
    for (p = &dummy; s1 && s2; p = p->next)
        if (s1->value < s2->value)
        {
            p->next = mk_SetElement (st, s1->value);
            s1 = s1->next;
        }
        else if (s1->value > s2->value)
        {
            p->next = mk_SetElement (st, s2->value);
            s2 = s2->next;
        }
        else
        {
            p->next = mk_SetElement (st, s1->value);
            s1 = s1->next;
            s2 = s2->next;
        }
    if (!s1)
        s1 = s2;
    while (s1)
    {
        p = p->next = mk_SetElement (st, s1->value);
        s1 = s1->next;
    }
    p->next = NULL;
    return dummy.next;
}

void pr_Set (SetType st, Set s)
{
    assert (st);
    while (s)
    {
        printf (" %d", s->value);
        s = s->next;
    }
    putchar ('\n');
}

unsigned hash_Set (SetType st, Set s)
{
    unsigned n = 0;
    while (s)
    {
        n += 11*s->value;
        s = s->next;
    }
    return n;
}

int eq_Set (SetType st, Set s1, Set s2)
{
    for (; s1 && s2; s1=s1->next, s2=s2->next)
        if (s1->value != s2->value)
            return 0;
    return s1 == s2;
}
