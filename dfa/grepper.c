/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: grepper.c,v $
 * Revision 1.1  1994-09-27 16:31:18  adam
 * First version of grepper: grep with error correction.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include <util.h>
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

static inline void set_bit (MatchContext *mc, MatchWord *m, int ch, int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    m[mc->n * ch + wno] |= 1<<off;
}

static inline void reset_bit (MatchContext *mc, MatchWord *m, int ch, int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    m[mc->n * ch + wno] &= ~(1<<off);
}

static inline MatchWord get_bit (MatchContext *mc, MatchWord *m, int ch, int state)
{
    int off = state & (WORD_BITS-1);
    int wno = state / WORD_BITS;

    return m[mc->n * ch + wno] & (1<<off);
}

static MatchContext *mk_MatchContext (DFA_states *dfas, int range)
{
    MatchContext *mc = imalloc (sizeof(*mc));
    int i;

    mc->n = (dfas->no+WORD_BITS) / WORD_BITS;
    mc->range = range;
    mc->Sc = icalloc (sizeof(*mc->Sc) * 256 * mc->n);
    
    for (i=0; i<dfas->no; i++)
    {
        int j;
        DFA_state *state = dfas->sortarray[i];

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
                   DFA_states *dfas, int ch)
{
    int i, s;
    Rdst[0] = 1;
    for (i = 1; i<mc->n; i++)
        Rdst[i] = 0;
    for (s = 0; s<dfas->no; s++)
        if (get_bit (mc, Rsrc, 0, s))
        {
            DFA_state *state = dfas->sortarray[s];
            int i = state->tran_no;
            while (--i >= 0)
                if (ch >= state->trans[i].ch[0] && ch <= state->trans[i].ch[1])
                    set_bit (mc, Rdst, 0, state->trans[i].to);
        }
}

static void shift (MatchContext *mc, MatchWord *Rdst, MatchWord *Rsrc,
                   DFA_states *dfas)
{
    int i, s;
    for (i = 0; i<mc->n; i++)
        Rdst[i] = 0;
    for (s = 0; s<dfas->no; s++)
        if (get_bit (mc, Rsrc, 0, s))
        {
            DFA_state *state = dfas->sortarray[s];
            int i = state->tran_no;
            while (--i >= 0)
                set_bit (mc, Rdst, 0, state->trans[i].to);
        }
}

static void or (MatchContext *mc, MatchWord *Rdst,
                MatchWord *Rsrc1, MatchWord *Rsrc2)
{
    int i;
    for (i = 0; i<mc->n; i++)
        Rdst[i] = Rsrc1[i] | Rsrc2[i];
}


static int go (MatchContext *mc, DFA_states *dfas, FILE *inf)
{
    MatchWord *Rj, *Rj1, *Rj_a, *Rj_b;
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

    set_bit (mc, Rj, 0, 0);
    for (d = 1; d<=mc->range; d++)
    {
        int s;
        memcpy (Rj + mc->n * d, Rj + mc->n * (d-1), mc->n * sizeof(*Rj));
        for (s = 0; s<dfas->no; s++)
        {
            if (get_bit (mc, Rj, d-1, s))
            {
                DFA_state *state = dfas->sortarray[s];
                int i = state->tran_no;
                while (--i >= 0)
                    set_bit (mc, Rj, d, state->trans[i].to);
            }
        }
    }
    while ((ch = getc (inf)) != EOF)
    {
        MatchWord *Rj_t;
        if (ch == '\n')
        {
            if (no_match)
            {
                int i = inf_ptr;
                if (show_line)
                    printf ("%5d:", lineno);
                while (infbuf[i] != '\n')
                {
                    if (--i < 0)
                        i = INFBUF_SIZE-1;
                }
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
        infbuf[inf_ptr++] = ch;
        if (inf_ptr == INFBUF_SIZE)
            inf_ptr = 0;
        mask_shift (mc, Rj1, Rj, dfas, ch);
        for (d = 1; d <= mc->range; d++)
        {
            mask_shift (mc, Rj_b, Rj+d*mc->n, dfas, ch);/* 1 */

            shift (mc, Rj_a, Rj+(d-1)*mc->n, dfas);     /* 2 */

            or (mc, Rj_a, Rj_a, Rj_b);                  /* 1,2 */

            shift (mc, Rj_b, Rj1+(d-1)*mc->n, dfas);    /* 3 */

            or (mc, Rj_a, Rj_a, Rj_b);                  /* 1,2,3 */

            or (mc, Rj1+d*mc->n, Rj_a, Rj+(d-1)*mc->n); /* 1,2,3,4 */
        }
        for (s = 0; s<dfas->no; s++)
        {
            if (dfas->sortarray[s]->rule_no)
                if (get_bit (mc, Rj1+mc->range*mc->n, 0, s))
                    no_match++;
        }
        for (d = 0; d <= mc->range; d++)
            reset_bit (mc, Rj1+d*mc->n, 0, dfas->no);
        Rj_t = Rj1;
        Rj1 = Rj;
        Rj = Rj_t;
    }
    ifree (Rj);
    ifree (Rj1);
    ifree (Rj_a);
    ifree (Rj_b);
    ifree (infbuf);
    return 0;
}

static int grep_file (DFA_states *dfas, const char *fname, int range)
{
    FILE *inf;
    MatchContext *mc;

    if (fname)
    {
        inf = fopen (fname, "r");
        if (!inf)
        {
            log (LOG_FATAL|LOG_ERRNO, "cannot open `%s'", fname);
            exit (1);
        }
    }
    else
        inf = stdin;
     
    mc = mk_MatchContext (dfas, range);

    go (mc, dfas, inf);

    if (fname)
        fclose (inf);
    return 0;
}

int main (int argc, char **argv)
{
    int ret;
    int range = 0;
    char *arg;
    char *pattern = NULL;
    DFA_states *dfas = NULL;
    int no_files = 0;

    prog = argv[0];
    while ((ret = options ("nr:dsv:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!pattern)
            {
                int i;
                DFA *dfa = init_dfa();
                pattern = arg;
                i = parse_dfa (dfa, &pattern, dfa_thompson_chars);
                if (i || *pattern)
                {
                    fprintf (stderr, "%s: illegal pattern\n", prog);
                    return 1;
                }
                dfa->root = dfa->top;
                dfas = mk_dfas (dfa, 200);
                rm_dfa (&dfa);
            }
            else
            {
                no_files++;
                grep_file (dfas, arg, range);
            }
        }
        else if (ret == 'v')
        {
            log_init (atoi(arg), prog, NULL);
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
            log (LOG_FATAL, "unknown option");
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
        grep_file (dfas, NULL, range);
    }
    return 0;
}
