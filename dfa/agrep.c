/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: agrep.c,v $
 * Revision 1.3  1994-09-27 16:31:18  adam
 * First version of grepper: grep with error correction.
 *
 * Revision 1.2  1994/09/26  16:30:56  adam
 * Minor changes. imalloc uses xmalloc now.
 *
 * Revision 1.1  1994/09/26  10:16:52  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include <util.h>
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

#ifdef YYDEBUG
#ifdef YACC
extern int yydebug;
#else
extern int alexdebug;
#endif
#endif

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
#ifdef YYDEBUG
                case 't':
#ifdef YACC
                    yydebug = 1;
#else
                    alexdebug = 1;
#endif
                    continue;
#endif
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
DFA_state **dfaar;
{
    DFA_state *s = dfaar[0];
    DFA_tran *t;
    char *p;
    int i;
    unsigned char c;

    while (1)
    {
        for (c = *inf_ptr++, t=s->trans, i=s->tran_no; --i >= 0; t++)
            if (c >= t->ch[0] && c <= t->ch[1])
            {
                p = inf_ptr;
                do
                {
                    if ((s = dfaar[t->to])->rule_no)
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
    }
    return 0;
}

int agrep (dfas, fd)
DFA_states *dfas;
int fd;
{
    inf_buf = imalloc (sizeof(char)*INF_BUF_SIZE);
    inf_eof = 0;
    inf_ptr = inf_buf+INF_BUF_SIZE;
    inf_flush (fd);
    line_no = 1;

    go (fd, dfas->sortarray);

    ifree (inf_buf);
    return 0;
}


int main (argc, argv)
int argc;
char **argv;
{
    char *pattern = NULL;
    char outbuf[BUFSIZ];
    int fd, i, no = 0;
    DFA *dfa = init_dfa();
    DFA_states *dfas;

    prog = *argv;
#ifdef YYDEBUG
#ifdef YACC
    yydebug = 0;
#else
    alexdebug = 0;
#endif
#endif
    setbuf (stdout, outbuf);
    i = agrep_options (argc, argv);
    if (i)
        return i;
    while (--argc > 0)
        if (**++argv != '-' && **argv)
            if (!pattern)
            {
                pattern = *argv;
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
                ++no;
                fd = open (*argv, O_RDONLY | O_BINARY);
                if (fd == -1)
                {
                    fprintf (stderr, "%s: couldn't open `%s'\n", prog, *argv);
                    return 1;
                }
                i = agrep (dfas, fd);
                close (fd);
                if (i)
                    return i;
            }
    if (!no)
    {
        fprintf (stderr, "%s: no files specified\n", prog);
        return 2;
    }
    fflush(stdout);
    return 0;
}
