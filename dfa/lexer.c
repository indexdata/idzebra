/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lexer.c,v $
 * Revision 1.11  1999-02-02 14:50:10  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.10  1996/10/29 13:57:27  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.9  1996/05/14 11:33:41  adam
 * MEMDEBUG turned off by default.
 *
 * Revision 1.8  1995/09/28  09:18:54  adam
 * Removed various preprocessor defines.
 *
 * Revision 1.7  1995/09/04  12:33:27  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.6  1995/01/25  11:30:51  adam
 * Simple error reporting when parsing regular expressions.
 * Memory usage reduced.
 *
 * Revision 1.5  1995/01/24  16:00:22  adam
 * Added -ansi to CFLAGS.
 * Some changes to the dfa module.
 *
 * Revision 1.4  1994/10/04  17:46:44  adam
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

#include <zebrautl.h>
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
    struct DFA *dfa;

    prog = *argv;
    dfa = dfa_init ();
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
            
            i = read_file (*argv, dfa);
            if (i)
                return i;
            dfa_mkstate (dfa);

#if MEMDEBUG
    imemstat();
#endif
        }
    dfa_delete (&dfa);
#if MEMDEBUG
    imemstat();
#endif
    if (!no)
    {
        fprintf (stderr, "%s: no files specified\n", prog);
        return 2;
    }
    return 0;
}

