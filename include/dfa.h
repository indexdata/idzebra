/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dfa.h,v $
 * Revision 1.2  1994-09-26 16:31:23  adam
 * Minor changes. xmalloc declares xcalloc now.
 *
 * Revision 1.1  1994/09/26  10:17:43  adam
 * Dfa-module header files.
 *
 */

#ifndef DFA_H
#define DFA_H

#include <bset.h>
#include <set.h>
#define CAT     16000
#define OR      16001
#define STAR    16002
#define PLUS    16003
#define EPSILON 16004

typedef struct Tnode_ {
    union  {
        struct Tnode_ *p[2];  /* CAT,OR,STAR,PLUS (left,right) */
        short ch[2];          /* if ch[0] >= 0 then this Tnode represents */
                              /*   a character in range [ch[0]..ch[1]]    */
                              /* otherwise ch[0] represents */
                              /*   accepting no (-acceptno) */
    } u;
    unsigned pos : 15;        /* CAT/OR/STAR/EPSILON or non-neg. position */
    unsigned nullable : 1;
    Set firstpos;             /* first positions */
    Set lastpos;              /* last positions */
} Tnode;

typedef struct Tblock_ {      /* Tnode bucket (block) */
    struct Tblock_ *next;     /* pointer to next bucket */
    Tnode *tarray;            /* array of nodes in bucket */
} Tblock;

typedef struct  {
    Tnode *root;              /* root of regular syntax tree */
    Tnode *top;               /* regular tree top (returned by parse_dfa) */
    int position;             /* no of positions so far */
    int rule;                 /* no of rules so far */
    BSetHandle *charset;      /* character set type */
    BSet anyset;              /* character recognized by `.' */
    int use_Tnode;            /* used Tnodes */
    int max_Tnode;            /* allocated Tnodes */
    Tblock *start;            /* start block of Tnodes */
    Tblock *end;              /* end block of Tnodes */
} DFA;

typedef struct {
    unsigned char ch[2];      /* transition on ch[0] <= c <= ch[1] to */
    unsigned short to;        /* this state */
} DFA_tran;

typedef struct DFA_trans_ {
    struct DFA_trans_ *next;  /* next DFA transition block */
    DFA_tran *tran_block;     /* pointer to transitions */
    int  ptr;                 /* index of next transition in tran_block */
    int  size;                /* allocated size of tran_block */
} DFA_trans;

typedef struct DFA_state_ {
    struct DFA_state_ *next;  /* next entry in free/unmarked/marked list */
    struct DFA_state_ *link;  /* link to next entry in hash chain */
    DFA_tran *trans;          /* transition list */
    Set set;                  /* set of positions (important nfa states) */
    short no;                 /* no of this state */
    short tran_no;            /* no of transitions to other states */
    short rule_no;            /* if non-zero, this holds accept rule no */
} DFA_state;

typedef struct DFA_stateb_ {
    struct DFA_stateb_ *next;
    DFA_state *state_block;
} DFA_stateb;

typedef struct  {
    DFA_state *freelist;      /* chain of unused (but allocated) DFA states */
    DFA_state *unmarked;      /* chain of unmarked DFA states */
    DFA_state *marked;        /* chain of marked DFA states */
    DFA_stateb *statemem;     /* state memory */
    int no;                   /* no of states (unmarked+marked) */
    SetType st;               /* Position set type */
    int hash;                 /* no hash entries in hasharray */
    DFA_state **hasharray;    /* hash pointers */
    DFA_state **sortarray;    /* sorted DFA states */
    DFA_trans *transmem;      /* transition memory */
} DFA_states;
           
DFA *       init_dfa        (void);
void        rm_dfa          (DFA **dfap);
int         parse_dfa       (DFA *, char **, const unsigned short *);
DFA_states* mk_dfas         (DFA *, int poset_chunk);
void        rm_dfas         (DFA_states **dfas);

Tnode *     mk_Tnode        (void);
Tnode *     mk_Tnode_cset   (BSet charset);
void        term_Tnode      (void);

int         init_DFA_states (DFA_states **dfasp, SetType st, int hash);
int         rm_DFA_states   (DFA_states **dfasp);
int         add_DFA_state   (DFA_states *dfas, Set *s, DFA_state **sp);
DFA_state   *get_DFA_state  (DFA_states *dfas);
void        sort_DFA_states (DFA_states *dfas);
void        add_DFA_tran    (DFA_states *, DFA_state *, int, int, int);

extern int  debug_dfa_trav;
extern int  debug_dfa_tran;
extern int  debug_dfa_followpos;
extern int  dfa_verbose;

extern unsigned short
        dfa_thompson_chars[],
        dfa_ccl_chars[];
#endif
