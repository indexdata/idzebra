/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dfap.h,v $
 * Revision 1.1  1995-01-24 16:02:53  adam
 * New private header file in dfa module (dfap.h).
 * Module no longer uses yacc to parse regular expressions.
 *
 */

#ifndef DFAP_H
#define DFAP_H

#include <dfa.h>

struct DFA_parse {
    struct Tnode *root;       /* root of regular syntax tree */
    struct Tnode *top;        /* regular tree top (returned by parse_dfa) */
    int position;             /* no of positions so far */
    int rule;                 /* no of rules so far */
    BSetHandle *charset;      /* character set type */
    BSet anyset;              /* character recognized by `.' */
    int use_Tnode;            /* used Tnodes */
    int max_Tnode;            /* allocated Tnodes */
    struct Tblock *start;     /* start block of Tnodes */
    struct Tblock *end;       /* end block of Tnodes */
};

typedef struct DFA_stateb_ {
    struct DFA_stateb_ *next;
    struct DFA_state *state_block;
} DFA_stateb;

struct DFA_states {
    struct DFA_state *freelist;   /* chain of unused (but allocated) states */
    struct DFA_state *unmarked;   /* chain of unmarked DFA states */
    struct DFA_state *marked;     /* chain of marked DFA states */
    DFA_stateb *statemem;         /* state memory */
    int no;                       /* no of states (unmarked+marked) */
    SetType st;                   /* Position set type */
    int hash;                     /* no hash entries in hasharray */
    struct DFA_state **hasharray; /* hash pointers */
    struct DFA_state **sortarray; /* sorted DFA states */
    struct DFA_trans *transmem;   /* transition memory */
};

int         init_DFA_states (struct DFA_states **dfasp, SetType st, int hash);
int         rm_DFA_states   (struct DFA_states **dfasp);
int         add_DFA_state   (struct DFA_states *dfas, Set *s,
                             struct DFA_state **sp);
struct DFA_state *get_DFA_state  (struct DFA_states *dfas);
void        sort_DFA_states (struct DFA_states *dfas);
void        add_DFA_tran    (struct DFA_states *, struct DFA_state *,
                             int, int, int);

#endif
