/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: states.c,v $
 * Revision 1.1  1994-09-26 10:16:58  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <util.h>
#include <dfa.h>
#include "imalloc.h"

#define DFA_CHUNK 200
#define TRAN_CHUNK 800

int init_DFA_states (DFA_states **dfasp, SetType st, int hash)
{
    DFA_states *dfas;
    DFA_trans *tm;
    int i;

    dfas = (DFA_states *) imalloc( sizeof(DFA_states) );
    assert( dfas );
    dfas->hasharray = (DFA_state **) imalloc( sizeof(DFA_state*) * hash );
    assert( dfas->hasharray );
    *dfasp = dfas;
    dfas->freelist = dfas->unmarked = dfas->marked = NULL;
    dfas->statemem = NULL;
    dfas->hash = hash;
    dfas->st = st;
    dfas->no = 0;

    dfas->transmem = tm = (DFA_trans *) imalloc( sizeof(DFA_trans) );
    assert( tm );
    tm->next = NULL;
    tm->size = TRAN_CHUNK;
    tm->ptr = 0;
    tm->tran_block = (DFA_tran *) imalloc( sizeof(DFA_tran) * tm->size );
    assert( tm->tran_block );

    dfas->sortarray = NULL;
    for( i=0; i<dfas->hash; i++ )
        dfas->hasharray[i] = NULL;
    return 0;
}

int rm_DFA_states (DFA_states **dfasp)
{
    DFA_states *dfas = *dfasp;
    DFA_stateb *sm, *sm1;
    DFA_trans  *tm, *tm1;

    assert( dfas );
    ifree( dfas->hasharray );
    ifree( dfas->sortarray );

    for( tm=dfas->transmem; tm; tm=tm1 )
    {
        ifree( tm->tran_block );
        tm1 = tm->next;
        ifree( tm );
    }
    for( sm=dfas->statemem; sm; sm=sm1 )
    {
        ifree( sm->state_block );
        sm1 = sm->next;
        ifree( sm );
    }
    ifree( dfas );
    *dfasp = NULL;
    return 0;
}

int add_DFA_state (DFA_states *dfas, Set *s, DFA_state **sp)
{
    int i;
    DFA_state *si, **sip;
    DFA_stateb *sb;

    assert( dfas );
    assert( *s );
    sip = dfas->hasharray + (hash_Set( dfas->st, *s ) % dfas->hash);
    for( si = *sip; si; si=si->link )
        if( eq_Set( dfas->st, si->set, *s ) )
        {
            *sp = si;
            *s = rm_Set( dfas->st, *s );
            return 0;
        }
    if( !dfas->freelist )
    {
        sb = (DFA_stateb *) imalloc( sizeof(*sb) );
        sb->next = dfas->statemem;
        dfas->statemem = sb;
        sb->state_block = si = dfas->freelist = 
            (DFA_state *) imalloc( sizeof(DFA_state)*DFA_CHUNK);
        for( i = 0; i<DFA_CHUNK-1; i++, si++ )
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

void add_DFA_tran (DFA_states *dfas, DFA_state *s, int ch0, int ch1, int to)
{
    DFA_trans *tm;
    DFA_tran *t;

    tm = dfas->transmem;
    if( tm->ptr == tm->size )
    {
        tm = (DFA_trans *) imalloc( sizeof(DFA_trans) );
        assert( tm );
        tm->next = dfas->transmem;
        dfas->transmem = tm;
        tm->size = s->tran_no >= TRAN_CHUNK ? s->tran_no+8 : TRAN_CHUNK;
        tm->tran_block = (DFA_tran *) imalloc( sizeof(DFA_tran) * tm->size );
        assert( tm->tran_block );
        if( s->tran_no )
            memcpy( tm->tran_block, s->trans, s->tran_no*sizeof( DFA_tran ) );
        tm->ptr = s->tran_no;
        s->trans = tm->tran_block;
    }
    s->tran_no++;
    t = tm->tran_block + tm->ptr++;
    t->ch[0] = ch0;
    t->ch[1] = ch1;
    t->to = to;
}

DFA_state *get_DFA_state (DFA_states *dfas)
{
    DFA_state *si;
    assert( dfas );
    if( !(si = dfas->unmarked) )
        return NULL;
    dfas->unmarked = si->next;
    si->next = dfas->marked;
    dfas->marked = si;
    si->trans = dfas->transmem->tran_block + dfas->transmem->ptr;

    return si;
}

void sort_DFA_states (DFA_states *dfas)
{
    DFA_state *s;
    assert( dfas );
    dfas->sortarray = (DFA_state **) imalloc( sizeof(DFA_state *)*dfas->no );
    for( s = dfas->marked; s; s=s->next )
        dfas->sortarray[s->no] = s;
}
