/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: agrep.c,v $
 * Revision 1.10  1997-09-09 13:37:57  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.9  1996/10/29 13:57:18  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.8  1996/01/08 09:09:16  adam
 * Function dfa_parse got 'const' string argument.
 * New functions to define char mappings made public.
 *
 * Revision 1.7  1995/10/16  09:31:24  adam
 * Bug fix.
 *
 * Revision 1.6  1995/09/28  09:18:51  adam
 * Removed various preprocessor defines.
 *
 * Revision 1.5  1995/09/04  12:33:25  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.4  1995/01/24  16:00:21  adam
 * Added -ansi to CFLAGS.
 * Some changes to the dfa module.
 *
 * Revision 1.3  1994/09/27  16:31:18  adam
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
#ifdef WINDOWS
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
