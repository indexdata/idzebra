/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lexer.c,v $
 * Revision 1.4  1994-10-04 17:46:44  adam
 * Function options now returns arg with error option.
 *
 * Revision 1.3  1994/10/03  17:22:19  adam
 * Optimization of grepper.
 *
 * Revision 1.2  1994/09/27  16:31:20  adam
 * First version of grepper: grep with error correction.
 *
 * Revision 1.1  1994/09/26  10:16:55  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 *
 * Adam Dickmeiss.      1992-1993
 * This module is actually very old...
 */
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <util.h>
#include <dfa.h>
#include "imalloc.h"
#include "lexer.h"

static char *prog;


void error (const char *format, ...)
{
    va_list argptr;
    va_start (argptr, format);
    fprintf (stderr, "%s error: ", prog);
    (void) vfprintf (stderr, format, argptr);
    putc ('\n', stderr);
    exit (1);
}

#ifdef YACC
extern int yydebug;
#else
extern int alexdebug;
#endif
int ccluse = 0;

static int lexer_options (int argc, char **argv)
{
    while (--argc > 0)
        if (**++argv == '-')
            while (*++*argv)
            {
                switch (**argv)
                {
                case 'V':
                    fprintf (stderr, "%s: %s %s\n", prog, __DATE__, __TIME__);
                    continue;
                case 's':
                    dfa_verbose = 1;
                    continue;
                case 't':
#ifdef YACC
                    yydebug = 1;
#else
                    alexdebug = 1;
#endif
                    continue;
	        case 'c':
                    ccluse = 1;
                    continue;
                case 'd':
                    switch (*++*argv)
                    {
                    case 's':
                        debug_dfa_tran = 1;
                        break;
                    case 't':
                        debug_dfa_trav = 1;
                        break;
                    case 'f':
                        debug_dfa_followpos = 1;
                        break;
                    default:
                        --*argv;
                        debug_dfa_tran = 1;
                        debug_dfa_followpos = 1;
                        debug_dfa_trav = 1;
                    }
                    continue;
                default:
                    fprintf (stderr, "%s: unknown option `-%s'\n",
                             prog, *argv);
                    return 1;
                }
                break;
            }
    return 0;
}

int main (int argc, char **argv)
{
    int i, no = 0;
    DFA *dfa;
    DFA_states *dfas;

    prog = *argv;
#ifdef YACC
    yydebug = 0;
#else
    alexdebug = 0;
#endif
    i = lexer_options (argc, argv);
    if (i)
        return i;

    if (argc < 2)
    {
        fprintf (stderr, "usage\n  %s [-c] [-V] [-s] [-t] [-d[stf]] file\n",
                 prog);
        return 1;
    }
    else while (--argc > 0)
        if (**++argv != '-' && **argv)
        {
            ++no;
            i = read_file (*argv, &dfa);
            if (i)
                return i;
            dfas = mk_dfas (dfa, 2000);
            rm_dfa (&dfa);
            rm_dfas (&dfas);
        }
#ifdef MEMDEBUG
    imemstat();
#endif
    if (!no)
    {
        fprintf (stderr, "%s: no files specified\n", prog);
        return 2;
    }
    return 0;
}

