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
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <idzebra/util.h>
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

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

