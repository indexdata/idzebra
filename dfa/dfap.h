/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dfap.h,v $
 * Revision 1.8  1999-02-02 14:50:06  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.7  1997/09/29 09:05:17  adam
 * Thread safe DFA module. We simply had to put a few static vars to
 * the DFA_parse structure.
 *
 * Revision 1.6  1997/09/18 08:59:17  adam
 * Extra generic handle for the character mapping routines.
 *
 * Revision 1.5  1997/09/05 15:29:58  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.4  1996/06/04 10:20:03  adam
 * Added support for character mapping.
 *
 * Revision 1.3  1996/01/08  09:09:19  adam
 * Function dfa_parse got 'const' string argument.
 * New functions to define char mappings made public.
 *
 * Revision 1.2  1995/01/25  11:30:50  adam
 * Simple error reporting when parsing regular expressions.
 * Memory usage reduced.
 *
 * Revision 1.1  1995/01/24  16:02:53  adam
 * New private header file in dfa module (dfap.h).
 * Module no longer uses yacc to parse regular expressions.
 *
 */

#ifndef DFAP_H
#define DFAP_H

#include <dfa.h>

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

    SetType poset;
    Set *followpos;

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
