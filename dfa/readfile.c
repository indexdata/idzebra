/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: readfile.c,v $
 * Revision 1.4  1995-01-25 11:30:51  adam
 * Simple error reporting when parsing regular expressions.
 * Memory usage reduced.
 *
 * Revision 1.3  1995/01/24  16:00:22  adam
 * Added -ansi to CFLAGS.
 * Some changes to the dfa module.
 *
 * Revision 1.2  1994/09/26  16:30:57  adam
 * Minor changes. imalloc uses xmalloc now.
 *
 * Revision 1.1  1994/09/26  10:16:56  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <util.h>
#include <dfa.h>
#include "lexer.h"

#define MAXLINE 512

static FILE *inf;
static FILE *outf;
static const char *inf_name;
static int line_no;
static int err_no;

static void
    prep        (char **s),
    read_defs   (void),
    read_rules  (struct DFA *dfap),
    read_tail   (void);

static char
    *read_line  (void);

static void prep (char **s)
{
    static char expr_buf[MAXLINE+1];
    char *dst = expr_buf;
    const char *src = *s;
    int c;

    while ((c = *src++))
        *dst++ = c;

    *dst = '\0';
    *s = expr_buf;
}

static char *read_line (void)
{
    static char linebuf[MAXLINE+1];
    ++line_no;
    return fgets (linebuf, MAXLINE, inf);
}

static void read_defs (void)
{
    const char *s;
    while ((s=read_line()))
    {
        if (*s == '%' && s[1] == '%')
            return;
        else if (*s == '\0' || isspace (*s))
            fputs (s, outf);
    }
    error ("missing rule section");
}

static void read_rules (struct DFA *dfa)
{
    char *s;
    int i;
    int no = 0;

    fputs ("\n#ifndef YY_BREAK\n#define YY_BREAK break;\n#endif\n", outf);
    fputs ("void lexact (int no)\n{\n", outf);
    fputs (  "\tswitch (no)\n\t{\n", outf);
    while ((s=read_line()))
    {
        if (*s == '%' && s[1] == '%')
            break;
        else if (*s == '\0' || isspace (*s))
            /* copy rest of line to output */
            fputs (s, outf);
        else
        { 
            /* preprocess regular expression */
            prep (&s);                   
            /* now parse regular expression */
            i = dfa_parse (dfa, &s);
            if (i)
            {
                fprintf (stderr, "%s #%d: regular expression syntax error\n",
                        inf_name, line_no);
                assert (0);
                err_no++;
            }
            else
            {
                if (no)
                    fputs ("\t\tYY_BREAK\n", outf);
                no++;
                fprintf (outf, "\tcase %d:\n#line %d\n\t\t", no, line_no);
            }
            while (*s == '\t' || *s == ' ')
                s++;
            fputs (s, outf);
        }
    }
    fputs ("\tYY_BREAK\n\t}\n}\n", outf);
    if (!no)
        error ("no regular expressions in rule section");
}

static void read_tail (void)
{
    const char *s;
    while ((s=read_line()))
        fputs (s, outf);
}

int read_file (const char *s, struct DFA *dfa)
{
    inf_name = s;
    if (!(inf=fopen (s,"r")))
    {
        error ("cannot open `%s'", s);
        return -1;
    }

    if (!(outf=fopen ("lex.yy.c", "w")))
    {
        error ("cannot open `%s'", "lex.yy.c");
        return -2;
    }

    line_no = 0;
    err_no = 0;

    read_defs ();
    read_rules (dfa);
    read_tail ();

    fclose (outf);
    fclose (inf);
    return err_no;
}
