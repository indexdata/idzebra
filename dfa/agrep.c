/* $Id: agrep.c,v 1.14 2005-01-15 19:38:18 adam Exp $
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


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WIN32

#include <io.h>

#else
#include <unistd.h>
#endif

#include <zebrautl.h>
#include <dfa.h>
#include "imalloc.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

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

static int show_lines = 0;

int agrep_options (argc, argv)
int argc;
char **argv;
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
                case 'v':
                    dfa_verbose = 1;
                    continue;
                case 'n':
                    show_lines = 1;
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
                    fprintf (stderr, "%s: unknown option `-%s'\n", prog, *argv);
                    return 1;
                }
                break;
            }
    return 0;
}

#define INF_BUF_SIZE  32768U
static char *inf_buf;
static char *inf_ptr, *inf_flsh;
static int inf_eof, line_no;

static int inf_flush (fd)
int fd;
{
    char *p;
    unsigned b, r;

    r = (unsigned) (inf_buf+INF_BUF_SIZE - inf_ptr);  /* no of `wrap' bytes */
    if (r)
        memcpy (inf_buf, inf_ptr, r);
    inf_ptr = p = inf_buf + r;
    b = INF_BUF_SIZE - r;
    do
        if ((r = read (fd, p, b)) == (unsigned) -1)
            return -1;
        else if (r)
            p +=  r;
        else
        {
            *p++ = '\n';
            inf_eof = 1;
            break;
        }
    while ((b -= r) > 0);
    while (p != inf_buf && *--p != '\n')
        ;
    while (p != inf_buf && *--p != '\n')
        ;
    inf_flsh = p+1;
    return 0;
}

static char *prline (p)
char *p;
{
    char *p0;

    --p;
    while (p != inf_buf && p[-1] != '\n')
        --p;
    p0 = p;
    while (*p++ != '\n')
        ;
    p[-1] = '\0';
    if (show_lines)
        printf ("%5d:\t%s\n", line_no, p0);
    else
        puts (p0);
    p[-1] = '\n';
    return p;
}

static int go (fd, dfaar)
int fd;
struct DFA_state **dfaar;
{
    struct DFA_state *s = dfaar[0];
    struct DFA_tran *t;
    char *p;
    int i;
    unsigned char c;
    int start_line = 1;

    while (1)
    {
        for (c = *inf_ptr++, t=s->trans, i=s->tran_no; --i >= 0; t++)
            if (c >= t->ch[0] && c <= t->ch[1])
            {
                p = inf_ptr;
                do
                {
                    if ((s = dfaar[t->to])->rule_no &&
                        (start_line || s->rule_nno))
                    {
                        inf_ptr = prline (inf_ptr);
                        c = '\n';
                        break;
                    }
                    for (t=s->trans, i=s->tran_no; --i >= 0; t++)
                        if ((unsigned) *p >= t->ch[0] 
                           && (unsigned) *p <= t->ch[1])
                            break;
                    p++;
                } while (i >= 0);
                s = dfaar[0];
                break;
            }
        if (c == '\n')
        {
            start_line = 1;
            ++line_no;
            if (inf_ptr == inf_flsh)
            {
                if (inf_eof)
                    break;
                ++line_no;
                if (inf_flush (fd))
                {
                    fprintf (stderr, "%s: read error\n", prog);
                    return -1;
                }
            }
        }
        else
            start_line = 0;
    }
    return 0;
}

int agrep (dfas, fd)
struct DFA_state **dfas;
int fd;
{
    inf_buf = imalloc (sizeof(char)*INF_BUF_SIZE);
    inf_eof = 0;
    inf_ptr = inf_buf+INF_BUF_SIZE;
    inf_flush (fd);
    line_no = 1;

    go (fd, dfas);

    ifree (inf_buf);
    return 0;
}


int main (argc, argv)
int argc;
char **argv;
{
    const char *pattern = NULL;
    char outbuf[BUFSIZ];
    int fd, i, no = 0;
    struct DFA *dfa = dfa_init();

    prog = *argv;
    if (argc < 2)
    {
        fprintf (stderr, "usage: agrep [options] pattern file..\n");
        fprintf (stderr, " -v   dfa verbose\n");
        fprintf (stderr, " -n   show lines\n");
        fprintf (stderr, " -d   debug\n");
        fprintf (stderr, " -V   show version\n");
        exit (1);
    }
    setbuf (stdout, outbuf);
    i = agrep_options (argc, argv);
    if (i)
        return i;
    while (--argc > 0)
        if (**++argv != '-' && **argv)
        {
            if (!pattern)
            {
                pattern = *argv;
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
                ++no;
                fd = open (*argv, O_RDONLY | O_BINARY);
                if (fd == -1)
                {
                    fprintf (stderr, "%s: couldn't open `%s'\n", prog, *argv);
                    return 1;
                }
                i = agrep (dfa->states, fd);
                close (fd);
                if (i)
                    return i;
            }
        }
    if (!no)
    {
        fprintf (stderr, "usage:\n "
                         " %s [-d] [-v] [-n] [-f] pattern file ..\n", prog);
        return 2;
    }
    fflush(stdout);
    dfa_delete (&dfa);
    return 0;
}
