/* This file is part of the Zebra server.
   Copyright (C) Index Data

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


#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <dfaset.h>
#include "imalloc.h"

static DFASet mk_DFASetElement (DFASetType st, int n);

DFASetType mk_DFASetType (int chunk)
{
    DFASetType st;

    assert (chunk > 8 && chunk < 8000);

    st = (DFASetType) imalloc (sizeof(*st));
    assert (st);

    st->alloclist = st->freelist = NULL;
    st->used = 0;
    st->chunk = chunk;
    return st;
}

int inf_DFASetType (DFASetType st, long *used, long *allocated)
{
    DFASet s;
    assert (st);
    *used = st->used;
    *allocated = 0;
    for (s = st->alloclist; s; s = s->next)
         *allocated += st->chunk;
    return sizeof (DFASetElement);
}

DFASetType rm_DFASetType (DFASetType st)
{
    DFASet s, s1;
    for (s = st->alloclist; (s1 = s);)
    {
        s = s->next;
        ifree (s1);
    }
    ifree (st);
    return NULL;
}

DFASet mk_DFASet (DFASetType st)
{
    assert (st);
    return NULL;
}

static DFASet mk_DFASetElement (DFASetType st, int n)
{
    DFASet s;
    int i;
    assert (st);

    assert (st->chunk > 8);
    if (! st->freelist)
    {
        s = (DFASet) imalloc (sizeof(*s) * (1+st->chunk));
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
static void rm_DFASetElement (DFASetType st, DFASetElement *p)
{
    assert (st);
    assert (st->used > 0);
    p->next = st->freelist;
    st->freelist = p;
    st->used--;
}
#endif

DFASet rm_DFASet (DFASetType st, DFASet s)
{
    DFASet s1 = s;
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

DFASet add_DFASet (DFASetType st, DFASet s, int n)
{
    DFASetElement dummy;
    DFASet p = &dummy, snew;
    p->next = s;
    while (p->next && p->next->value < n)
        p = p->next;
    assert (p);
    if (!(p->next && p->next->value == n))
    {
        snew = mk_DFASetElement (st, n);
        snew->next = p->next;
        p->next = snew;
    }
    return dummy.next;
}

DFASet union_DFASet (DFASetType st, DFASet s1, DFASet s2)
{
    DFASetElement dummy;
    DFASet p;
    assert (st);

    for (p = &dummy; s1 && s2;)
        if (s1->value < s2->value)
        {
            p = p->next = s1;
            s1 = s1->next;
        }
        else if (s1->value > s2->value)
        {
            p = p->next = mk_DFASetElement (st, s2->value);
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
            p = p->next = mk_DFASetElement (st, s2->value);
            s2 = s2->next;
        }
        p->next = NULL;
    }
    return dummy.next;
}

DFASet cp_DFASet (DFASetType st, DFASet s)
{
    return merge_DFASet (st, s, NULL);
}

DFASet merge_DFASet (DFASetType st, DFASet s1, DFASet s2)
{
    DFASetElement dummy;
    DFASet p;
    assert (st);
    for (p = &dummy; s1 && s2; p = p->next)
        if (s1->value < s2->value)
        {
            p->next = mk_DFASetElement (st, s1->value);
            s1 = s1->next;
        }
        else if (s1->value > s2->value)
        {
            p->next = mk_DFASetElement (st, s2->value);
            s2 = s2->next;
        }
        else
        {
            p->next = mk_DFASetElement (st, s1->value);
            s1 = s1->next;
            s2 = s2->next;
        }
    if (!s1)
        s1 = s2;
    while (s1)
    {
        p = p->next = mk_DFASetElement (st, s1->value);
        s1 = s1->next;
    }
    p->next = NULL;
    return dummy.next;
}

void pr_DFASet (DFASetType st, DFASet s)
{
    assert (st);
    while (s)
    {
        printf (" %d", s->value);
        s = s->next;
    }
    putchar ('\n');
}

unsigned hash_DFASet (DFASetType st, DFASet s)
{
    unsigned n = 0;
    while (s)
    {
        n += 11*s->value;
        s = s->next;
    }
    return n;
}

int eq_DFASet (DFASetType st, DFASet s1, DFASet s2)
{
    for (; s1 && s2; s1=s1->next, s2=s2->next)
        if (s1->value != s2->value)
            return 0;
    return s1 == s2;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

