/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: set.c,v $
 * Revision 1.2  1995-01-24 16:00:22  adam
 * Added -ansi to CFLAGS.
 * Some changes to the dfa module.
 *
 * Revision 1.1  1994/09/26  10:16:57  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <util.h>
#include <set.h>
#include "imalloc.h"


static void rm_SetElement (SetType st, SetElement *p);
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

static void rm_SetElement (SetType st, SetElement *p)
{
    assert (st);
    assert (st->used > 0);
    p->next = st->freelist;
    st->freelist = p;
    st->used--;
}

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
    Set p = &dummy, new;
    p->next = s;
    while (p->next && p->next->value < n)
        p = p->next;
    assert (p);
    if (!(p->next && p->next->value == n))
    {
        new = mk_SetElement (st, n);
        new->next = p->next;
        p->next = new;
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
