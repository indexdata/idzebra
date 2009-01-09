/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include "dfap.h"
#include "imalloc.h"

#define DFA_CHUNK 40
#define TRAN_CHUNK 100

int init_DFA_states (struct DFA_states **dfasp, DFASetType st, int hash)
{
    struct DFA_states *dfas;
    struct DFA_trans *tm;
    int i;
    
    dfas = (struct DFA_states *) imalloc (sizeof(struct DFA_states));
    assert (dfas);
    dfas->hasharray = (struct DFA_state **)
        imalloc (sizeof(struct DFA_state*) * hash);
    assert (dfas->hasharray);
    *dfasp = dfas;
    dfas->freelist = dfas->unmarked = dfas->marked = NULL;
    dfas->statemem = NULL;
    dfas->hash = hash;
    dfas->st = st;
    dfas->no = 0;

    dfas->transmem = tm = (struct DFA_trans *)
        imalloc (sizeof(struct DFA_trans));
    assert (tm);
    tm->next = NULL;
    tm->size = TRAN_CHUNK;
    tm->ptr = 0;
    tm->tran_block = (struct DFA_tran *)
        imalloc (sizeof(struct DFA_tran) * tm->size);
    assert (tm->tran_block);

    dfas->sortarray = NULL;
    for (i=0; i<dfas->hash; i++)
        dfas->hasharray[i] = NULL;
    return 0;
}

int rm_DFA_states (struct DFA_states **dfasp)
{
    struct DFA_states *dfas = *dfasp;
    DFA_stateb *sm, *sm1;
    struct DFA_trans  *tm, *tm1;

    assert (dfas);
    if (dfas->hasharray)
        ifree (dfas->hasharray);
    ifree (dfas->sortarray);

    for (tm=dfas->transmem; tm; tm=tm1)
    {
        ifree (tm->tran_block);
        tm1 = tm->next;
        ifree (tm);
    }
    for (sm=dfas->statemem; sm; sm=sm1)
    {
        ifree (sm->state_block);
        sm1 = sm->next;
        ifree (sm);
    }
    ifree (dfas);
    *dfasp = NULL;
    return 0;
}

int add_DFA_state (struct DFA_states *dfas, DFASet *s, struct DFA_state **sp)
{
    int i;
    struct DFA_state *si, **sip;
    DFA_stateb *sb;

    assert (dfas);
    assert (*s);
    assert (dfas->hasharray);
    sip = dfas->hasharray + (hash_DFASet (dfas->st, *s) % dfas->hash);
    for (si = *sip; si; si=si->link)
        if (eq_DFASet (dfas->st, si->set, *s))
        {
            *sp = si;
            *s = rm_DFASet (dfas->st, *s);
            return 0;
        }
    if (!dfas->freelist)
    {
        sb = (DFA_stateb *) imalloc (sizeof(*sb));
        sb->next = dfas->statemem;
        dfas->statemem = sb;
        sb->state_block = si = dfas->freelist = 
            (struct DFA_state *) imalloc (sizeof(struct DFA_state)*DFA_CHUNK);
        for (i = 0; i<DFA_CHUNK-1; i++, si++)
            si->next = si+1;
        si->next = NULL;
    }

    si = dfas->freelist;
    dfas->freelist = si->next;

    si->next = dfas->unmarked;
    dfas->unmarked = si;

    si->link = *sip;
    *sip = si;

    si->no = (dfas->no)++;
    si->tran_no = 0;
    si->set = *s;
    *s = NULL;
    *sp = si;
    return 1;
}

void add_DFA_tran (struct DFA_states *dfas, struct DFA_state *s,
                   int ch0, int ch1, int to)
{
    struct DFA_trans *tm;
    struct DFA_tran *t;

    tm = dfas->transmem;
    if (tm->ptr == tm->size)
    {
        tm = (struct DFA_trans *) imalloc (sizeof(struct DFA_trans));
        assert (tm);
        tm->next = dfas->transmem;
        dfas->transmem = tm;
        tm->size = s->tran_no >= TRAN_CHUNK ? s->tran_no+8 : TRAN_CHUNK;
        tm->tran_block = (struct DFA_tran *)
            imalloc (sizeof(struct DFA_tran) * tm->size);
        assert (tm->tran_block);
        if (s->tran_no)
            memcpy (tm->tran_block, s->trans,
                    s->tran_no*sizeof (struct DFA_tran));
        tm->ptr = s->tran_no;
        s->trans = tm->tran_block;
    }
    s->tran_no++;
    t = tm->tran_block + tm->ptr++;
    t->ch[0] = ch0;
    t->ch[1] = ch1;
    t->to = to;
}

struct DFA_state *get_DFA_state (struct DFA_states *dfas)
{
    struct DFA_state *si;
    assert (dfas);
    if (!(si = dfas->unmarked))
        return NULL;
    dfas->unmarked = si->next;
    si->next = dfas->marked;
    dfas->marked = si;
    si->trans = dfas->transmem->tran_block + dfas->transmem->ptr;

    return si;
}

void sort_DFA_states (struct DFA_states *dfas)
{
    struct DFA_state *s;
    assert (dfas);
    dfas->sortarray = (struct DFA_state **)
        imalloc (sizeof(struct DFA_state *)*dfas->no);
    for (s = dfas->marked; s; s=s->next)
        dfas->sortarray[s->no] = s;
    ifree (dfas->hasharray);
    dfas->hasharray = NULL;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

