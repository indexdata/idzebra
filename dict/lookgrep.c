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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dfa.h>
#include "dict-p.h"

typedef unsigned MatchWord;
#define WORD_BITS 32
#define MAX_LENGTH 1024

/* This code is based
 * Sun Wu and Udi Manber: Fast Text Searching Allowing Errors.
 *      Communications of the ACM, pp. 83-91, Vol. 35, No. 10, Oct. 1992, USA.
 *      PostScript version of the paper in its submitted form: agrep1.ps)
 *      recommended reading to understand AGREP !
 *
 * http://www.tgries.de/agrep/#AGREP1PS
 * http://www.tgries.de/agrep/doc/agrep1ps.zip
 */

typedef struct {
    int n;                 /* no of MatchWord needed */
    int range;             /* max no. of errors */
    int fact;              /* (range+1)*n */
    MatchWord *match_mask; /* match_mask */
} MatchContext;

#define INLINE

static INLINE void set_bit(MatchContext *mc, MatchWord *m, int ch, int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    m[mc->n * ch + wno] |= 1<<off;
}

static INLINE MatchWord get_bit(MatchContext *mc, MatchWord *m, int ch,
                                int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    return m[mc->n * ch + wno] & (1<<off);
}

static MatchContext *mk_MatchContext(struct DFA *dfa, int range)
{
    MatchContext *mc = (MatchContext *) xmalloc(sizeof(*mc));
    int s;

    mc->n = (dfa->no_states+WORD_BITS) / WORD_BITS;
    mc->range = range;
    mc->fact = (range+1)*mc->n;
    mc->match_mask = (MatchWord *) xcalloc(mc->n, sizeof(*mc->match_mask));

    for (s = 0; s<dfa->no_states; s++)
        if (dfa->states[s]->rule_no)
            set_bit(mc, mc->match_mask, 0, s);
    return mc;
}

static void rm_MatchContext(MatchContext **mc)
{
    xfree((*mc)->match_mask);
    xfree(*mc);
    *mc = NULL;
}

static void mask_shift(MatchContext *mc, MatchWord *Rdst, MatchWord *Rsrc,
                       struct DFA *dfa, int ch)
{
    int j, s = 0;
    MatchWord *Rsrc_p = Rsrc, mask;

    for (j = 0; j<mc->n; j++)
        Rdst[j] = 0;
    while (1)
    {
        mask = *Rsrc_p++;
        for (j = 0; j<WORD_BITS/4; j++)
        {
            if (mask & 15)
            {
                if (mask & 1)
                {
                    struct DFA_state *state = dfa->states[s];
                    int i = state->tran_no;
                    while (--i >= 0)
                        if (ch >= state->trans[i].ch[0] &&
                            ch <= state->trans[i].ch[1])
                            set_bit(mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 2)
                {
                    struct DFA_state *state = dfa->states[s+1];
                    int i = state->tran_no;
                    while (--i >= 0)
                        if (ch >= state->trans[i].ch[0] &&
                            ch <= state->trans[i].ch[1])
                            set_bit(mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 4)
                {
                    struct DFA_state *state = dfa->states[s+2];
                    int i = state->tran_no;
                    while (--i >= 0)
                        if (ch >= state->trans[i].ch[0] &&
                            ch <= state->trans[i].ch[1])
                            set_bit(mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 8)
                {
                    struct DFA_state *state = dfa->states[s+3];
                    int i = state->tran_no;
                    while (--i >= 0)
                        if (ch >= state->trans[i].ch[0] &&
                            ch <= state->trans[i].ch[1])
                            set_bit(mc, Rdst, 0, state->trans[i].to);
                }
            }
            s += 4;
            if (s >= dfa->no_states)
                return;
            mask >>= 4;
        }
    }
}

static void shift(MatchContext *mc, MatchWord *Rdst, MatchWord *Rsrc,
                  struct DFA *dfa)
{
    int j, s = 0;
    MatchWord *Rsrc_p = Rsrc, mask;
    for (j = 0; j<mc->n; j++)
        Rdst[j] = 0;
    while (1)
    {
        mask = *Rsrc_p++;
        for (j = 0; j<WORD_BITS/4; j++)
        {
            if (mask & 15)
            {
                if (mask & 1)
                {
                    struct DFA_state *state = dfa->states[s];
                    int i = state->tran_no;
                    while (--i >= 0)
                        set_bit(mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 2)
                {
                    struct DFA_state *state = dfa->states[s+1];
                    int i = state->tran_no;
                    while (--i >= 0)
                        set_bit(mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 4)
                {
                    struct DFA_state *state = dfa->states[s+2];
                    int i = state->tran_no;
                    while (--i >= 0)
                        set_bit(mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 8)
                {
                    struct DFA_state *state = dfa->states[s+3];
                    int i = state->tran_no;
                    while (--i >= 0)
                        set_bit(mc, Rdst, 0, state->trans[i].to);
                }
            }
            s += 4;
            if (s >= dfa->no_states)
                return;
            mask >>= 4;
        }
    }
}

static void or(MatchContext *mc, MatchWord *Rdst,
               MatchWord *Rsrc1, MatchWord *Rsrc2)
{
    int i;
    for (i = 0; i<mc->n; i++)
        Rdst[i] = Rsrc1[i] | Rsrc2[i];
}

static INLINE int move(MatchContext *mc, MatchWord *Rj1, MatchWord *Rj,
                       Dict_char ch, struct DFA *dfa, MatchWord *Rtmp,
                       int range)
{
    int d;
    MatchWord *Rtmp_2 = Rtmp + mc->n;

    mask_shift(mc, Rj1, Rj, dfa, ch);
    for (d = 1; d <= mc->range; d++)
    {
        or(mc, Rtmp, Rj, Rj1);                         /* 2,3 */

        shift(mc, Rtmp_2, Rtmp, dfa);

        mask_shift(mc, Rtmp, Rj+mc->n, dfa, ch);       /* 1 */

        or(mc, Rtmp, Rtmp_2, Rtmp);                    /* 1,2,3*/

        Rj1 += mc->n;

        or(mc, Rj1, Rtmp, Rj);                         /* 1,2,3,4 */

        Rj += mc->n;
    }
    return 1;

}


static int grep(Dict dict, Dict_ptr ptr, MatchContext *mc,
                MatchWord *Rj, int pos, void *client,
                int (*userfunc)(char *, const char *, void *),
                Dict_char *prefix, struct DFA *dfa,
                int *max_pos, int init_pos)
{
    int lo, hi, d;
    void *p;
    short *indxp;
    char *info;

    dict_bf_readp(dict->dbf, ptr, &p);
    lo = 0;
    hi = DICT_nodir(p)-1;
    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));

    while (lo <= hi)
    {
        if (indxp[-lo] > 0)
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */
            int j;
            int was_match = 0;
            info = (char*)p + indxp[-lo];
            for (j=0; ; j++)
            {
                Dict_char ch;
                MatchWord *Rj0 =    Rj + j    *mc->fact;
                MatchWord *Rj1 =    Rj + (j+1)*mc->fact;
                MatchWord *Rj_tmp = Rj + (j+2)*mc->fact;
                int range;

                memcpy(&ch, info+j*sizeof(Dict_char), sizeof(Dict_char));
                prefix[pos+j] = ch;
                if (pos+j > *max_pos)
                    *max_pos = pos+j;
                if (ch == DICT_EOS)
                {
                    if (was_match)
                    {
                        int ret = userfunc((char*) prefix,
                                           info+(j+1)*sizeof(Dict_char), client);
                        if (ret)
                            return ret;
                    }
                    break;
                }
                if (pos+j >= init_pos)
                    range = mc->range;
                else
                    range = 0;
                move(mc, Rj1, Rj0, ch, dfa, Rj_tmp, range);
                for (d = mc->n; --d >= 0; )
                    if (Rj1[range*mc->n + d])
                        break;
                if (d < 0)
                    break;
                was_match = 0;
                for (d = mc->n; --d >= 0; )
                    if (Rj1[range*mc->n + d] & mc->match_mask[d])
                    {
                        was_match = 1;
                        break;
                    }
            }
        }
        else
        {
            MatchWord *Rj1 =    Rj+  mc->fact;
            MatchWord *Rj_tmp = Rj+2*mc->fact;
            Dict_char ch;
            int range;

            /* Dict_ptr             subptr */
            /* Dict_char            sub char */
            /* unsigned char        length of information */
            /* char *               information */
            info = (char*)p - indxp[-lo];
            memcpy(&ch, info+sizeof(Dict_ptr), sizeof(Dict_char));
            prefix[pos] = ch;

            if (pos > *max_pos)
                *max_pos = pos;
            if (pos >= init_pos)
                range = mc->range;
            else
                range = 0;
            move(mc, Rj1, Rj, ch, dfa, Rj_tmp, range);
            for (d = mc->n; --d >= 0; )
                if (Rj1[range*mc->n + d])
                    break;
            if (d >= 0)
            {
                Dict_ptr subptr;
                if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
                {
                    for (d = mc->n; --d >= 0; )
                        if (Rj1[range*mc->n + d] & mc->match_mask[d])
                        {
                            int ret;
                            prefix[pos+1] = DICT_EOS;
                            ret = userfunc((char*) prefix,
                                           info+sizeof(Dict_ptr)+
                                           sizeof(Dict_char), client);
                            if (ret)
                                return ret;
                            break;
                        }
                }
                memcpy(&subptr, info, sizeof(Dict_ptr));
                if (subptr)
                {
                    int ret = grep(dict, subptr, mc, Rj1, pos+1,
                                   client, userfunc, prefix, dfa, max_pos,
                                   init_pos);
                    if (ret)
                        return ret;

                    dict_bf_readp(dict->dbf, ptr, &p);
                    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
                }
            }
        }
        lo++;
    }
    return 0;
}

int dict_lookup_grep(Dict dict, const char *pattern, int range, void *client,
                     int *max_pos, int init_pos,
                     int (*userfunc)(char *name, const char *info,
                                     void *client))
{
    MatchWord *Rj;
    Dict_char prefix[MAX_LENGTH+1];
    const char *this_pattern = pattern;
    MatchContext *mc;
    struct DFA *dfa = dfa_init();
    int i, d, ret = 0;

#if 0
    debug_dfa_trav = 1;
    debug_dfa_tran = 1;
    debug_dfa_followpos = 1;
    dfa_verbose = 1;
#endif

    dfa_anyset_includes_nl(dfa);

    yaz_log(YLOG_DEBUG, "dict_lookup_grep range=%d", range);
    for (i = 0; pattern[i]; i++)
    {
        yaz_log(YLOG_DEBUG, " %2d %3d  %c", i, pattern[i],
                (pattern[i] > ' ' && pattern[i] < 127) ? pattern[i] : '?');
    }

    dfa_set_cmap(dfa, dict->grep_cmap_data, dict->grep_cmap);

    i = dfa_parse(dfa, &this_pattern);
    if (i || *this_pattern)
    {
        yaz_log(YLOG_WARN, "dfa_parse fail=%d", i);
        dfa_delete(&dfa);
        return -1;
    }
    dfa_mkstate(dfa);

    mc = mk_MatchContext(dfa, range);

    Rj = (MatchWord *) xcalloc((MAX_LENGTH+2) * mc->fact, sizeof(*Rj));

    set_bit (mc, Rj, 0, 0);
    for (d = 1; d<=mc->range; d++)
    {
        int s;
        memcpy(Rj + mc->n * d, Rj + mc->n * (d-1), mc->n * sizeof(*Rj));
        for (s = 0; s < dfa->no_states; s++)
        {
            if (get_bit(mc, Rj, d-1, s))
            {
                struct DFA_state *state = dfa->states[s];
                int i = state->tran_no;
                while (--i >= 0)
                    set_bit(mc, Rj, d, state->trans[i].to);
            }
        }
    }
    *max_pos = 0;
    if (dict->head.root)
        ret = grep(dict, dict->head.root, mc, Rj, 0, client,
                   userfunc, prefix,
                   dfa, max_pos, init_pos);
    yaz_log(YLOG_DEBUG, "max_pos = %d", *max_pos);
    dfa_delete(&dfa);
    xfree(Rj);
    rm_MatchContext(&mc);
    return ret;
}

void dict_grep_cmap(Dict dict, void *vp,
                    const char **(*cmap)(void *vp, const char **from, int len))
{
    dict->grep_cmap = cmap;
    dict->grep_cmap_data = vp;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

