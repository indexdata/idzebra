/* $Id: dfa.h,v 1.13 2006-05-10 08:13:18 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#ifndef DFA_H
#define DFA_H

#include <bset.h>
#include <dfaset.h>

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

struct DFA_tran {
    unsigned char ch[2];      /* transition on ch[0] <= c <= ch[1] to */
    unsigned short to;        /* this state */
};

struct DFA_trans {
    struct DFA_trans *next;   /* next DFA transition block */
    struct DFA_tran *tran_block; /* pointer to transitions */
    int  ptr;                 /* index of next transition in tran_block */
    int  size;                /* allocated size of tran_block */
};

struct DFA_state {
    struct DFA_state *next;   /* next entry in free/unmarked/marked list */
    struct DFA_state *link;   /* link to next entry in hash chain */
    struct DFA_tran *trans;   /* transition list */
    DFASet set;               /* set of positions (important nfa states) */
    short no;                 /* no of this state */
    short tran_no;            /* no of transitions to other states */
    short rule_no;            /* if non-zero, this holds accept rule no */
    short rule_nno;           /* accept rule no - except start rules */
};

struct DFA {
    int no_states;
    struct DFA_state  **states;
    struct DFA_states *state_info;
    struct DFA_parse  *parse_info;
};

struct DFA *dfa_init (void);
void dfa_set_cmap (struct DFA *dfa, void *vp,
                   const char **(*cmap)(void *vp, const char **from, int len));
int dfa_parse (struct DFA *, const char **);
void dfa_mkstate (struct DFA *);
void dfa_delete (struct DFA **);

void dfa_parse_cmap_clean (struct DFA *d);
void dfa_parse_cmap_new (struct DFA *d, const int *cmap);
void dfa_parse_cmap_del (struct DFA *d, int from);
void dfa_parse_cmap_add (struct DFA *d, int from, int to);

extern int  debug_dfa_trav;
extern int  debug_dfa_tran;
extern int  debug_dfa_followpos;
extern int  dfa_verbose;

extern unsigned short
        dfa_thompson_chars[],
        dfa_ccl_chars[];

#define L_LP 1
#define L_RP 2
#define L_CHAR 3
#define L_CHARS 4
#define L_ANY 5
#define L_ALT 6
#define L_ANYZ 7
#define L_WILD 8
#define L_QUEST 9
#define L_CLOS1 10
#define L_CLOS0 11
#define L_END 12
#define L_START 13

#define DFA_ERR_SYNTAX 1
#define DFA_ERR_LP     2
#define DFA_ERR_RP     3

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

