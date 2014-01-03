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
#include <ctype.h>
#include <assert.h>

#include <idzebra/util.h>
#include <yaz/yaz-util.h>
#include <dfa.h>
#include "imalloc.h"

char *prog;
static int show_line = 0;

typedef unsigned MatchWord;
#define WORD_BITS 32

typedef struct {
    int n;           /* no of MatchWord needed */
    int range;       /* max no. of errors */
    MatchWord *Sc;   /* Mask Sc */
} MatchContext;

#define INFBUF_SIZE 16384

#define INLINE

static INLINE void set_bit (MatchContext *mc, MatchWord *m, int ch, int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    m[mc->n * ch + wno] |= 1<<off;
}

static INLINE void reset_bit (MatchContext *mc, MatchWord *m, int ch,
                              int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    m[mc->n * ch + wno] &= ~(1<<off);
}

static INLINE MatchWord get_bit (MatchContext *mc, MatchWord *m, int ch,
                                 int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    return m[mc->n * ch + wno] & (1<<off);
}

static MatchContext *mk_MatchContext (struct DFA *dfa, int range)
{
    MatchContext *mc = imalloc (sizeof(*mc));
    int i;

    mc->n = (dfa->no_states+WORD_BITS) / WORD_BITS;
    mc->range = range;
    mc->Sc = icalloc (sizeof(*mc->Sc) * 256 * mc->n);

    for (i=0; i<dfa->no_states; i++)
    {
        int j;
        struct DFA_state *state = dfa->states[i];

        for (j=0; j<state->tran_no; j++)
        {
            int ch;
            int ch0 = state->trans[j].ch[0];
            int ch1 = state->trans[j].ch[1];
            assert (ch0 >= 0 && ch1 >= 0);

            for (ch = ch0; ch <= ch1; ch++)
                set_bit (mc, mc->Sc, ch, i);
        }
    }
    return mc;
}


static void mask_shift (MatchContext *mc, MatchWord *Rdst, MatchWord *Rsrc,
                        struct DFA *dfa, int ch)
{
    int j, s = 0;
    MatchWord *Rsrc_p = Rsrc, mask;

    Rdst[0] = 1;
    for (j = 1; j<mc->n; j++)
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
                            set_bit (mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 2)
                {
                    struct DFA_state *state = dfa->states[s+1];
                    int i = state->tran_no;
                    while (--i >= 0)
                        if (ch >= state->trans[i].ch[0] &&
                            ch <= state->trans[i].ch[1])
                            set_bit (mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 4)
                {
                    struct DFA_state *state = dfa->states[s+2];
                    int i = state->tran_no;
                    while (--i >= 0)
                        if (ch >= state->trans[i].ch[0] &&
                            ch <= state->trans[i].ch[1])
                            set_bit (mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 8)
                {
                    struct DFA_state *state = dfa->states[s+3];
                    int i = state->tran_no;
                    while (--i >= 0)
                        if (ch >= state->trans[i].ch[0] &&
                            ch <= state->trans[i].ch[1])
                            set_bit (mc, Rdst, 0, state->trans[i].to);
                }
            }
            s += 4;
            if (s >= dfa->no_states)
                return;
            mask >>= 4;
        }
    }
}

static void shift (MatchContext *mc, MatchWord *Rdst, MatchWord *Rsrc,
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
                        set_bit (mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 2)
                {
                    struct DFA_state *state = dfa->states[s+1];
                    int i = state->tran_no;
                    while (--i >= 0)
                        set_bit (mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 4)
                {
                    struct DFA_state *state = dfa->states[s+2];
                    int i = state->tran_no;
                    while (--i >= 0)
                        set_bit (mc, Rdst, 0, state->trans[i].to);
                }
                if (mask & 8)
                {
                    struct DFA_state *state = dfa->states[s+3];
                    int i = state->tran_no;
                    while (--i >= 0)
                        set_bit (mc, Rdst, 0, state->trans[i].to);
                }
            }
            s += 4;
            if (s >= dfa->no_states)
                return;
            mask >>= 4;
        }
    }
}

static void or (MatchContext *mc, MatchWord *Rdst,
                MatchWord *Rsrc1, MatchWord *Rsrc2)
{
    int i;
    for (i = 0; i<mc->n; i++)
        Rdst[i] = Rsrc1[i] | Rsrc2[i];
}


static int go (MatchContext *mc, struct DFA *dfa, FILE *inf)
{
    MatchWord *Rj, *Rj1, *Rj_a, *Rj_b, *Rj_c;
    int s, d, ch;
    int lineno = 1;
    char *infbuf;
    int inf_ptr = 1;
    int no_match = 0;

    infbuf = imalloc (INFBUF_SIZE);
    infbuf[0] = '\n';
    Rj = icalloc (mc->n * (mc->range+1) * sizeof(*Rj));
    Rj1 = icalloc (mc->n * (mc->range+1) * sizeof(*Rj));
    Rj_a = icalloc (mc->n * sizeof(*Rj));
    Rj_b = icalloc (mc->n * sizeof(*Rj));
    Rj_c = icalloc (mc->n * sizeof(*Rj));

    set_bit (mc, Rj, 0, 0);
    for (d = 1; d<=mc->range; d++)
    {
        int s;
        memcpy (Rj + mc->n * d, Rj + mc->n * (d-1), mc->n * sizeof(*Rj));
        for (s = 0; s<dfa->no_states; s++)
        {
            if (get_bit (mc, Rj, d-1, s))
            {
                struct DFA_state *state = dfa->states[s];
                int i = state->tran_no;
                while (--i >= 0)
                    set_bit (mc, Rj, d, state->trans[i].to);
            }
        }
    }
    while ((ch = getc (inf)) != EOF)
    {
        MatchWord *Rj_t;

        infbuf[inf_ptr] = ch;
        if (ch == '\n')
        {
            if (no_match)
            {
                int i = inf_ptr;
                if (show_line)
                    printf ("%5d:", lineno);
                do
                {
                    if (--i < 0)
                        i = INFBUF_SIZE-1;
                } while (infbuf[i] != '\n');
                do
                {
                    if (++i == INFBUF_SIZE)
                        i = 0;
                    putchar (infbuf[i]);
                } while (infbuf[i] != '\n');
                no_match = 0;
            }
            lineno++;
        }
        if (++inf_ptr == INFBUF_SIZE)
            inf_ptr = 0;
        mask_shift (mc, Rj1, Rj, dfa, ch);
        for (d = 1; d <= mc->range; d++)
        {
            mask_shift (mc, Rj_b, Rj+d*mc->n, dfa, ch);    /* 1 */

            or (mc, Rj_a, Rj+(d-1)*mc->n, Rj1+(d-1)*mc->n); /* 2,3 */

            shift (mc, Rj_c, Rj_a, dfa);

            or (mc, Rj_a, Rj_b, Rj_c);                      /* 1,2,3*/

            or (mc, Rj1+d*mc->n, Rj_a, Rj+(d-1)*mc->n);     /* 1,2,3,4 */
        }
        for (s = 0; s<dfa->no_states; s++)
        {
            if (dfa->states[s]->rule_no)
                if (get_bit (mc, Rj1+mc->range*mc->n, 0, s))
                    no_match++;
        }
        for (d = 0; d <= mc->range; d++)
            reset_bit (mc, Rj1+d*mc->n, 0, dfa->no_states);
        Rj_t = Rj1;
        Rj1 = Rj;
        Rj = Rj_t;
    }
    ifree (Rj);
    ifree (Rj1);
    ifree (Rj_a);
    ifree (Rj_b);
    ifree (Rj_c);
    ifree (infbuf);
    return 0;
}

static int grep_file (struct DFA *dfa, const char *fname, int range)
{
    FILE *inf;
    MatchContext *mc;

    if (fname)
    {
        inf = fopen (fname, "r");
        if (!inf)
        {
            yaz_log (YLOG_FATAL|YLOG_ERRNO, "cannot open `%s'", fname);
            exit (1);
        }
    }
    else
        inf = stdin;

    mc = mk_MatchContext (dfa, range);

    go (mc, dfa, inf);

    if (fname)
        fclose (inf);
    return 0;
}

int main (int argc, char **argv)
{
    int ret;
    int range = 0;
    char *arg;
    const char *pattern = NULL;
    int no_files = 0;
    struct DFA *dfa = dfa_init();

    prog = argv[0];
    while ((ret = options ("nr:dsv:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!pattern)
            {
                int i;
                pattern = arg;
                i = dfa_parse (dfa, &pattern);
                if (i || *pattern)
                {
                    fprintf (stderr, "%s: illegal pattern\n", prog);
                    return 1;
                }
                dfa_mkstate (dfa);
            }
            else
            {
                no_files++;
                grep_file (dfa, arg, range);
            }
        }
        else if (ret == 'v')
        {
            yaz_log_init (yaz_log_mask_str(arg), prog, NULL);
        }
        else if (ret == 's')
        {
            dfa_verbose = 1;
        }
        else if (ret == 'd')
        {
            debug_dfa_tran = 1;
            debug_dfa_followpos = 1;
            debug_dfa_trav = 1;
        }
        else if (ret == 'r')
        {
            range = atoi (arg);
        }
        else if (ret == 'n')
        {
            show_line = 1;
        }
        else
        {
            yaz_log (YLOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!pattern)
    {
        fprintf (stderr, "usage:\n "
                 " %s [-d] [-n] [-r n] [-s] [-v n] pattern file ..\n", prog);
        exit (1);
    }
    else if (no_files == 0)
    {
        grep_file (dfa, NULL, range);
    }
    dfa_delete (&dfa);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

