/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dfa.h,v $
 * Revision 1.9  1999-02-02 14:50:31  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.8  1997/09/18 08:59:18  adam
 * Extra generic handle for the character mapping routines.
 *
 * Revision 1.7  1997/09/05 15:29:59  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.6  1996/06/04 10:20:10  adam
 * Added support for character mapping.
 *
 * Revision 1.5  1996/01/08  09:09:48  adam
 * Function dfa_parse got 'const' string argument.
 *
 * Revision 1.4  1995/01/25  11:31:04  adam
 * Simple error reporting when parsing regular expressions.
 *
 * Revision 1.3  1995/01/24  16:01:30  adam
 * Added -ansi to CFLAGS.
 * New functions and change of data structures.
 *
 * Revision 1.2  1994/09/26  16:31:23  adam
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

#ifdef __cplusplus
extern "C" {
#endif

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
    Set set;                  /* set of positions (important nfa states) */
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

#ifdef __cplusplus
}
#endif

#endif
