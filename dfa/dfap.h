/* $Id: dfap.h,v 1.15 2007-01-15 20:08:23 adam Exp $
   Copyright (C) 1995-2007
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#ifndef DFAP_H
#define DFAP_H

#include <dfa.h>

#ifdef __cplusplus
extern "C" {
#endif

struct DFA_parse {
    struct Tnode *root;       /* root of regular syntax tree */
    int position;             /* no of positions so far */
    int rule;                 /* no of rules so far */
    BSetHandle *charset;      /* character set type */
    BSet anyset;              /* character recognized by `.' */
    int use_Tnode;            /* used Tnodes */
    int max_Tnode;            /* allocated Tnodes */
    struct Tblock *start;     /* start block of Tnodes */
    struct Tblock *end;       /* end block of Tnodes */
    int *charMap;
    int charMapSize;
    void *cmap_data;

    unsigned look_ch;
    int lookahead;
    BSet look_chars;
    int err_code;
    int inside_string;
    const unsigned char *expr_ptr;

    struct Tnode **posar;

    DFASetType poset;
    DFASet *followpos;

    const char **(*cmap)(void *vp, const char **from, int len);
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
    DFASetType st;                   /* Position set type */
    int hash;                     /* no hash entries in hasharray */
    struct DFA_state **hasharray; /* hash pointers */
    struct DFA_state **sortarray; /* sorted DFA states */
    struct DFA_trans *transmem;   /* transition memory */
};

int         init_DFA_states (struct DFA_states **dfasp, DFASetType st, int hash);
int         rm_DFA_states   (struct DFA_states **dfasp);
int         add_DFA_state   (struct DFA_states *dfas, DFASet *s,
                             struct DFA_state **sp);
struct DFA_state *get_DFA_state  (struct DFA_states *dfas);
void        sort_DFA_states (struct DFA_states *dfas);
void        add_DFA_tran    (struct DFA_states *, struct DFA_state *,
                             int, int, int);

#ifdef __cplusplus
}
#endif
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

