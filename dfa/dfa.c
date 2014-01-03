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
#include <ctype.h>

#include <idzebra/util.h>
#include "dfap.h"
#include "imalloc.h"

#define CAT     16000
#define OR      16001
#define STAR    16002
#define PLUS    16003
#define EPSILON 16004

struct Tnode {
    union  {
        struct Tnode *p[2];   /* CAT,OR,STAR,PLUS (left,right) */
        short ch[2];          /* if ch[0] >= 0 then this Tnode represents */
                              /*   a character in range [ch[0]..ch[1]]    */
                              /* otherwise ch[0] represents */
                              /*   accepting no (-acceptno) */
    } u;
    unsigned pos : 15;        /* CAT/OR/STAR/EPSILON or non-neg. position */
    unsigned nullable : 1;
    DFASet firstpos;             /* first positions */
    DFASet lastpos;              /* last positions */
};

struct Tblock {               /* Tnode bucket (block) */
    struct Tblock *next;      /* pointer to next bucket */
    struct Tnode *tarray;     /* array of nodes in bucket */
};

#define TADD 64
#define STATE_HASH 199
#define POSET_CHUNK 100

int debug_dfa_trav  = 0;
int debug_dfa_tran  = 0;
int debug_dfa_followpos = 0;
int dfa_verbose = 0;

static struct Tnode *mk_Tnode      (struct DFA_parse *parse_info);
static struct Tnode *mk_Tnode_cset (struct DFA_parse *parse_info,
				    BSet charset);
static void        term_Tnode      (struct DFA_parse *parse_info);

static void
    del_followpos  (struct DFA_parse *parse_info),
    init_pos       (struct DFA_parse *parse_info),
    del_pos        (struct DFA_parse *parse_info),
    mk_dfa_tran    (struct DFA_parse *parse_info, struct DFA_states *dfas),
    add_follow     (struct DFA_parse *parse_info, DFASet lastpos, DFASet firstpos),
    dfa_trav       (struct DFA_parse *parse_info, struct Tnode *n),
    init_followpos (struct DFA_parse *parse_info),
    pr_tran        (struct DFA_parse *parse_info, struct DFA_states *dfas),
    pr_verbose     (struct DFA_parse *parse_info, struct DFA_states *dfas),
    pr_followpos   (struct DFA_parse *parse_info),
    out_char       (int c),
    lex            (struct DFA_parse *parse_info);

static int
    nextchar       (struct DFA_parse *parse_info, int *esc),
    read_charset   (struct DFA_parse *parse_info);

static const char
    *str_char      (unsigned c);

#define L_LP 1
#define L_RP 2
#define L_CHAR 3
#define L_CHARS 4
#define L_ANY 5
#define L_ALT 6
#define L_ANYZ 7
#define L_WILD 8
#define L_QUEST 9
#define L_CLOS1 10
#define L_CLOS0 11
#define L_END 12
#define L_START 13


static struct Tnode *expr_1 (struct DFA_parse *parse_info),
                    *expr_2 (struct DFA_parse *parse_info),
                    *expr_3 (struct DFA_parse *parse_info),
                    *expr_4 (struct DFA_parse *parse_info);

static struct Tnode *expr_1 (struct DFA_parse *parse_info)
{
    struct Tnode *t1, *t2, *tn;

    if (!(t1 = expr_2 (parse_info)))
        return t1;
    while (parse_info->lookahead == L_ALT)
    {
        lex (parse_info);
        if (!(t2 = expr_2 (parse_info)))
            return t2;

        tn = mk_Tnode (parse_info);
        tn->pos = OR;
        tn->u.p[0] = t1;
        tn->u.p[1] = t2;
        t1 = tn;
    }
    return t1;
}

static struct Tnode *expr_2 (struct DFA_parse *parse_info)
{
    struct Tnode *t1, *t2, *tn;

    if (!(t1 = expr_3 (parse_info)))
        return t1;
    while (parse_info->lookahead == L_WILD ||
           parse_info->lookahead == L_ANYZ ||
           parse_info->lookahead == L_ANY ||
           parse_info->lookahead == L_LP ||
           parse_info->lookahead == L_CHAR ||
           parse_info->lookahead == L_CHARS)
    {
        if (!(t2 = expr_3 (parse_info)))
            return t2;

        tn = mk_Tnode (parse_info);
        tn->pos = CAT;
        tn->u.p[0] = t1;
        tn->u.p[1] = t2;
        t1 = tn;
    }
    return t1;
}

static struct Tnode *expr_3 (struct DFA_parse *parse_info)
{
    struct Tnode *t1, *tn;

    if (!(t1 = expr_4 (parse_info)))
        return t1;
    if (parse_info->lookahead == L_CLOS0)
    {
        lex (parse_info);
        tn = mk_Tnode (parse_info);
        tn->pos = STAR;
        tn->u.p[0] = t1;
        t1 = tn;
    }
    else if (parse_info->lookahead == L_CLOS1)
    {
        lex (parse_info);
        tn = mk_Tnode (parse_info);
        tn->pos = PLUS;
        tn->u.p[0] = t1;
        t1 = tn;
    }
    else if (parse_info->lookahead == L_QUEST)
    {
        lex (parse_info);
        tn = mk_Tnode(parse_info);
        tn->pos = OR;
        tn->u.p[0] = t1;
        tn->u.p[1] = mk_Tnode(parse_info);
        tn->u.p[1]->pos = EPSILON;
        t1 = tn;
    }
    return t1;
}

static struct Tnode *expr_4 (struct DFA_parse *parse_info)
{
    struct Tnode *t1;

    switch (parse_info->lookahead)
    {
    case L_LP:
        lex (parse_info);
        if (!(t1 = expr_1 (parse_info)))
            return t1;
        if (parse_info->lookahead == L_RP)
            lex (parse_info);
        else
            return NULL;
        break;
    case L_CHAR:
        t1 = mk_Tnode(parse_info);
        t1->pos = ++parse_info->position;
        t1->u.ch[1] = t1->u.ch[0] = parse_info->look_ch;
        lex (parse_info);
        break;
    case L_CHARS:
        t1 = mk_Tnode_cset (parse_info, parse_info->look_chars);
        lex (parse_info);
        break;
    case L_ANY:
        t1 = mk_Tnode_cset (parse_info, parse_info->anyset);
        lex (parse_info);
        break;
    case L_ANYZ:
        t1 = mk_Tnode(parse_info);
        t1->pos = OR;
        t1->u.p[0] = mk_Tnode_cset (parse_info, parse_info->anyset);
        t1->u.p[1] = mk_Tnode(parse_info);
        t1->u.p[1]->pos = EPSILON;
        lex (parse_info);
        break;
    case L_WILD:
        t1 = mk_Tnode(parse_info);
        t1->pos = STAR;
        t1->u.p[0] = mk_Tnode_cset (parse_info, parse_info->anyset);
        lex (parse_info);
    default:
        t1 = NULL;
    }
    return t1;
}

static void do_parse (struct DFA_parse *parse_info, const char **s,
                      struct Tnode **tnp)
{
    int start_anchor_flag = 0;
    struct Tnode *t1, *t2, *tn;

    parse_info->err_code = 0;
    parse_info->expr_ptr =  * (const unsigned char **) s;

    parse_info->inside_string = 0;
    lex (parse_info);
    if (parse_info->lookahead == L_START)
    {
        start_anchor_flag = 1;
        lex (parse_info);
    }
    if (parse_info->lookahead == L_END)
    {
        t1 = mk_Tnode (parse_info);
        t1->pos = ++parse_info->position;
        t1->u.ch[1] = t1->u.ch[0] = '\n';
        lex (parse_info);
    }
    else
    {
        t1 = expr_1 (parse_info);
        if (t1 && parse_info->lookahead == L_END)
        {
            t2 = mk_Tnode (parse_info);
            t2->pos = ++parse_info->position;
            t2->u.ch[1] = t2->u.ch[0] = '\n';

            tn = mk_Tnode (parse_info);
            tn->pos = CAT;
            tn->u.p[0] = t1;
            tn->u.p[1] = t2;
            t1 = tn;

            lex (parse_info);
        }
    }
    if (t1 && parse_info->lookahead == 0)
    {
        t2 = mk_Tnode(parse_info);
        t2->pos = ++parse_info->position;
        t2->u.ch[0] = -(++parse_info->rule);
        t2->u.ch[1] = start_anchor_flag ? 0 : -(parse_info->rule);

        *tnp = mk_Tnode(parse_info);
        (*tnp)->pos = CAT;
        (*tnp)->u.p[0] = t1;
        (*tnp)->u.p[1] = t2;
    }
    else
    {
        if (!parse_info->err_code)
        {
            if (parse_info->lookahead == L_RP)
                parse_info->err_code = DFA_ERR_RP;
            else if (parse_info->lookahead == L_LP)
                parse_info->err_code = DFA_ERR_LP;
            else
                parse_info->err_code = DFA_ERR_SYNTAX;
        }
    }
    *s = (const char *) parse_info->expr_ptr;
}

static int nextchar (struct DFA_parse *parse_info, int *esc)
{
    *esc = 0;
    if (*parse_info->expr_ptr == '\0')
        return 0;
    else if (*parse_info->expr_ptr != '\\')
        return *parse_info->expr_ptr++;
    *esc = 1;
    switch (*++parse_info->expr_ptr)
    {
    case '\r':
    case '\n':
    case '\0':
        return '\\';
    case '\t':
        ++parse_info->expr_ptr;
        return ' ';
    case 'n':
        ++parse_info->expr_ptr;
        return '\n';
    case 't':
        ++parse_info->expr_ptr;
        return '\t';
    case 'r':
        ++parse_info->expr_ptr;
        return '\r';
    case 'f':
        ++parse_info->expr_ptr;
        return '\f';
    default:
        return *parse_info->expr_ptr++;
    }
}

static int nextchar_set (struct DFA_parse *parse_info, int *esc)
{
    if (*parse_info->expr_ptr == ' ')
    {
        *esc = 0;
        return *parse_info->expr_ptr++;
    }
    return nextchar (parse_info, esc);
}

static int read_charset (struct DFA_parse *parse_info)
{
    int i, ch0, esc0, cc = 0;
    parse_info->look_chars = mk_BSet (&parse_info->charset);
    res_BSet (parse_info->charset, parse_info->look_chars);

    ch0 = nextchar_set (parse_info, &esc0);
    if (!esc0 && ch0 == '^')
    {
        cc = 1;
        ch0 = nextchar_set (parse_info, &esc0);
    }
    /**
       ch0 is last met character
       ch1 is "next" char
    */
    while (ch0 != 0)
    {
        int ch1, esc1;
        if (!esc0 && ch0 == ']')
            break;
	if (!esc0 && ch0 == '-')
	{
	    ch1 = ch0;
	    esc1 = esc0;
	    ch0 = 1;
	    add_BSet (parse_info->charset, parse_info->look_chars, ch0);
	}
	else
	{
            if (ch0 == 1)
            {
                ch0 = nextchar(parse_info, &esc0);
            }
            else
            {
                if (parse_info->cmap)
                {
                    const char **mapto;
                    char mapfrom[2];
                    const char *mcp = mapfrom;
                    mapfrom[0] = ch0;
                    mapto = parse_info->cmap(parse_info->cmap_data, &mcp, 1);
                    assert (mapto);
                    ch0 = mapto[0][0];
                }
            }
	    add_BSet (parse_info->charset, parse_info->look_chars, ch0);
	    ch1 = nextchar_set (parse_info, &esc1);
	}
        if (!esc1 && ch1 == '-')
        {
            int open_range = 0;
            if ((ch1 = nextchar_set (parse_info, &esc1)) == 0)
                break;
            if (!esc1 && ch1 == ']')
            {
                ch1 = 255;
                open_range = 1;
            }
            else if (ch1 == 1)
            {
                ch1 = nextchar(parse_info, &esc1);
            }
            else if (parse_info->cmap)
            {
                const char **mapto;
		char mapfrom[2];
                const char *mcp = mapfrom;
                mapfrom[0] = ch1;
                mapto = (*parse_info->cmap) (parse_info->cmap_data, &mcp, 1);
                assert (mapto);
                ch1 = mapto[0][0];
            }
            for (i = ch0; ++i <= ch1;)
                add_BSet (parse_info->charset, parse_info->look_chars, i);

            if (open_range)
                break;
            ch0 = nextchar_set (parse_info, &esc0);
        }
        else
        {
            esc0 = esc1;
            ch0 = ch1;
        }
    }
    if (cc)
        com_BSet (parse_info->charset, parse_info->look_chars);
    return L_CHARS;
}

static int map_l_char (struct DFA_parse *parse_info)
{
    const char **mapto;
    const char *cp0 = (const char *) (parse_info->expr_ptr-1);
    int i = 0, len = strlen(cp0);

    if (cp0[0] == 1 && cp0[1])
    {
        parse_info->expr_ptr++;
        parse_info->look_ch = ((unsigned char *) cp0)[1];
        return L_CHAR;
    }
    if (!parse_info->cmap)
        return L_CHAR;

    mapto = (*parse_info->cmap) (parse_info->cmap_data, &cp0, len);
    assert (mapto);

    parse_info->expr_ptr = (const unsigned char *) cp0;
    parse_info->look_ch = ((unsigned char **) mapto)[i][0];
    yaz_log (YLOG_DEBUG, "map from %c to %d", parse_info->expr_ptr[-1], parse_info->look_ch);
    return L_CHAR;
}

static int lex_sub(struct DFA_parse *parse_info)
{
    int esc;
    while ((parse_info->look_ch = nextchar (parse_info, &esc)) != 0)
        if (parse_info->look_ch == '\"')
        {
            if (esc)
                return map_l_char (parse_info);
            parse_info->inside_string = !parse_info->inside_string;
        }
        else if (esc || parse_info->inside_string)
            return map_l_char (parse_info);
        else if (parse_info->look_ch == '[')
            return read_charset(parse_info);
        else
        {
            const int *cc;
            for (cc = parse_info->charMap; *cc; cc += 2)
                if (*cc == (int) (parse_info->look_ch))
                {
                    if (!cc[1])
                        --parse_info->expr_ptr;
                    return cc[1];
                }
            return map_l_char (parse_info);
        }
    return 0;
}

static void lex (struct DFA_parse *parse_info)
{
    parse_info->lookahead = lex_sub (parse_info);
}

static const char *str_char (unsigned c)
{
    static char s[6];
    s[0] = '\\';
    if (c < 32 || c >= 127)
        switch (c)
        {
        case '\r':
            strcpy (s+1, "r");
            break;
        case '\n':
            strcpy (s+1, "n");
            break;
        case '\t':
            strcpy (s+1, "t");
            break;
        default:
            sprintf (s+1, "x%02x", c);
            break;
        }
    else
        switch (c)
        {
        case '\"':
            strcpy (s+1, "\"");
            break;
        case '\'':
            strcpy (s+1, "\'");
            break;
        case '\\':
            strcpy (s+1, "\\");
            break;
        default:
            s[0] = c;
            s[1] = '\0';
        }
    return s;
}

static void out_char (int c)
{
    printf ("%s", str_char (c));
}

static void term_Tnode (struct DFA_parse *parse_info)
{
    struct Tblock *t, *t1;
    for (t = parse_info->start; (t1 = t);)
    {
        t=t->next;
        ifree (t1->tarray);
        ifree (t1);
    }
}

static struct Tnode *mk_Tnode (struct DFA_parse *parse_info)
{
    struct Tblock *tnew;

    if (parse_info->use_Tnode == parse_info->max_Tnode)
    {
        tnew = (struct Tblock *) imalloc (sizeof(struct Tblock));
        tnew->tarray = (struct Tnode *) imalloc (TADD*sizeof(struct Tnode));
        if (!tnew->tarray)
            return NULL;
        if (parse_info->use_Tnode == 0)
            parse_info->start = tnew;
        else
            parse_info->end->next = tnew;
        parse_info->end = tnew;
        tnew->next = NULL;
        parse_info->max_Tnode += TADD;
    }
    return parse_info->end->tarray+(parse_info->use_Tnode++ % TADD);
}

struct Tnode *mk_Tnode_cset (struct DFA_parse *parse_info, BSet charset)
{
    struct Tnode *tn1, *tn0 = mk_Tnode(parse_info);
    int ch1, ch0 = trav_BSet (parse_info->charset, charset, 0);
    if (ch0 == -1)
        tn0->pos = EPSILON;
    else
    {
        tn0->u.ch[0] = ch0;
        tn0->pos = ++parse_info->position;
        ch1 = travi_BSet (parse_info->charset, charset, ch0+1) ;
        if (ch1 == -1)
            tn0->u.ch[1] = parse_info->charset->size;
        else
        {
            tn0->u.ch[1] = ch1-1;
            while ((ch0=trav_BSet (parse_info->charset, charset, ch1)) != -1)
            {
                tn1 = mk_Tnode(parse_info);
                tn1->pos = OR;
                tn1->u.p[0] = tn0;
                tn0 = tn1;
                tn1 = tn0->u.p[1] = mk_Tnode(parse_info);
                tn1->u.ch[0] = ch0;
                tn1->pos = ++(parse_info->position);
                if ((ch1 = travi_BSet (parse_info->charset, charset,
                                       ch0+1)) == -1)
                {
                    tn1->u.ch[1] = parse_info->charset->size;
                    break;
                }
                tn1->u.ch[1] = ch1-1;
            }
        }
    }
    return tn0;
}

static void del_followpos (struct DFA_parse *parse_info)
{
    ifree (parse_info->followpos);
}

static void init_pos (struct DFA_parse *parse_info)
{
    parse_info->posar = (struct Tnode **) imalloc (sizeof(struct Tnode*)
                                       * (1+parse_info->position));
}

static void del_pos (struct DFA_parse *parse_info)
{
    ifree (parse_info->posar);
}

static void add_follow (struct DFA_parse *parse_info,
			DFASet lastpos, DFASet firstpos)
{
    while (lastpos)
    {
        parse_info->followpos[lastpos->value] =
	    union_DFASet (parse_info->poset,
		       parse_info->followpos[lastpos->value], firstpos);
        lastpos = lastpos->next;
    }
}

static void dfa_trav (struct DFA_parse *parse_info, struct Tnode *n)
{
    struct Tnode **posar = parse_info->posar;
    DFASetType poset = parse_info->poset;

    switch (n->pos)
    {
    case CAT:
        dfa_trav (parse_info, n->u.p[0]);
        dfa_trav (parse_info, n->u.p[1]);
        n->nullable = n->u.p[0]->nullable & n->u.p[1]->nullable;
        n->firstpos = mk_DFASet (poset);
        n->firstpos = union_DFASet (poset, n->firstpos, n->u.p[0]->firstpos);
        if (n->u.p[0]->nullable)
            n->firstpos = union_DFASet (poset, n->firstpos, n->u.p[1]->firstpos);
        n->lastpos = mk_DFASet (poset);
        n->lastpos = union_DFASet (poset, n->lastpos, n->u.p[1]->lastpos);
        if (n->u.p[1]->nullable)
            n->lastpos = union_DFASet (poset, n->lastpos, n->u.p[0]->lastpos);

        add_follow (parse_info, n->u.p[0]->lastpos, n->u.p[1]->firstpos);

        n->u.p[0]->firstpos = rm_DFASet (poset, n->u.p[0]->firstpos);
        n->u.p[0]->lastpos = rm_DFASet (poset, n->u.p[0]->lastpos);
        n->u.p[1]->firstpos = rm_DFASet (poset, n->u.p[1]->firstpos);
        n->u.p[1]->lastpos = rm_DFASet (poset, n->u.p[1]->lastpos);
        if (debug_dfa_trav)
            printf ("CAT");
        break;
    case OR:
        dfa_trav (parse_info, n->u.p[0]);
        dfa_trav (parse_info, n->u.p[1]);
        n->nullable = n->u.p[0]->nullable | n->u.p[1]->nullable;

        n->firstpos = merge_DFASet (poset, n->u.p[0]->firstpos,
                                 n->u.p[1]->firstpos);
        n->lastpos = merge_DFASet (poset, n->u.p[0]->lastpos,
                                n->u.p[1]->lastpos);
        n->u.p[0]->firstpos = rm_DFASet (poset, n->u.p[0]->firstpos);
        n->u.p[0]->lastpos = rm_DFASet (poset, n->u.p[0]->lastpos);
        n->u.p[1]->firstpos = rm_DFASet (poset, n->u.p[1]->firstpos);
        n->u.p[1]->lastpos = rm_DFASet (poset, n->u.p[1]->lastpos);
        if (debug_dfa_trav)
            printf ("OR");
        break;
    case PLUS:
        dfa_trav (parse_info, n->u.p[0]);
        n->nullable = n->u.p[0]->nullable;
        n->firstpos = n->u.p[0]->firstpos;
        n->lastpos = n->u.p[0]->lastpos;
        add_follow (parse_info, n->lastpos, n->firstpos);
        if (debug_dfa_trav)
            printf ("PLUS");
        break;
    case STAR:
        dfa_trav (parse_info, n->u.p[0]);
        n->nullable = 1;
        n->firstpos = n->u.p[0]->firstpos;
        n->lastpos = n->u.p[0]->lastpos;
        add_follow (parse_info, n->lastpos, n->firstpos);
        if (debug_dfa_trav)
            printf ("STAR");
        break;
    case EPSILON:
        n->nullable = 1;
        n->lastpos = mk_DFASet (poset);
        n->firstpos = mk_DFASet (poset);
        if (debug_dfa_trav)
            printf ("EPSILON");
        break;
    default:
        posar[n->pos] = n;
        n->nullable = 0;
        n->firstpos = mk_DFASet (poset);
        n->firstpos = add_DFASet (poset, n->firstpos, n->pos);
        n->lastpos = mk_DFASet (poset);
        n->lastpos = add_DFASet (poset, n->lastpos, n->pos);
        if (debug_dfa_trav)
	{
            if (n->u.ch[0] < 0)
                printf ("#%d (n#%d)", -n->u.ch[0], -n->u.ch[1]);
            else if (n->u.ch[1] > n->u.ch[0])
            {
                putchar ('[');
                out_char (n->u.ch[0]);
                if (n->u.ch[1] > n->u.ch[0]+1)
                    putchar ('-');
                out_char (n->u.ch[1]);
                putchar (']');
            }
            else
                out_char (n->u.ch[0]);
	}
    }
    if (debug_dfa_trav)
    {
        printf ("\n nullable : %c\n", n->nullable ? '1' : '0');
        printf (" firstpos :");
        pr_DFASet (poset, n->firstpos);
        printf (" lastpos  :");
        pr_DFASet (poset, n->lastpos);
    }
}

static void init_followpos (struct DFA_parse *parse_info)
{
    DFASet *fa;
    int i;
    parse_info->followpos = fa =
	(DFASet *) imalloc (sizeof(DFASet) * (1+parse_info->position));
    for (i = parse_info->position+1; --i >= 0; fa++)
        *fa = mk_DFASet (parse_info->poset);
}

static void mk_dfa_tran (struct DFA_parse *parse_info, struct DFA_states *dfas)
{
    int i, j, c;
    int max_char;
    short *pos, *pos_i;
    DFASet tran_set;
    int char_0, char_1;
    struct DFA_state *dfa_from, *dfa_to;
    struct Tnode **posar;
    DFASetType poset;
    DFASet *followpos;

    assert (parse_info);

    posar = parse_info->posar;
    max_char = parse_info->charset->size;
    pos = (short *) imalloc (sizeof(*pos) * (parse_info->position+1));

    tran_set = cp_DFASet (parse_info->poset, parse_info->root->firstpos);
    i = add_DFA_state (dfas, &tran_set, &dfa_from);
    assert (i);
    dfa_from->rule_no = 0;
    poset = parse_info->poset;
    followpos = parse_info->followpos;
    while ((dfa_from = get_DFA_state (dfas)))
    {
        pos_i = pos;
        j = i = 0;
        for (tran_set = dfa_from->set; tran_set; tran_set = tran_set->next)
            if ((c = posar[tran_set->value]->u.ch[0]) >= 0 && c <= max_char)
                *pos_i++ = tran_set->value;
            else if (c < 0)
            {
                if (i == 0 || c > i)
                    i = c;
                c = posar[tran_set->value]->u.ch[1];
                if (j == 0 || c > j)
                    j = c;
            }
        *pos_i = -1;
        dfa_from->rule_no = -i;
        dfa_from->rule_nno = -j;

        for (char_1 = 0; char_1 <= max_char; char_1++)
        {
            char_0 = max_char+1;
            for (pos_i = pos; (i = *pos_i) != -1; ++pos_i)
                if (posar[i]->u.ch[1] >= char_1
                    && (c=posar[i]->u.ch[0]) < char_0)
		{
                    if (c < char_1)
                        char_0 = char_1;
                    else
                        char_0 = c;
		}

            if (char_0 > max_char)
                break;

            char_1 = max_char;

            tran_set = mk_DFASet (poset);
            for (pos_i = pos; (i = *pos_i) != -1; ++pos_i)
            {
                if ((c=posar[i]->u.ch[0]) > char_0 && c <= char_1)
                    char_1 = c - 1;                /* forward chunk */
                else if ((c=posar[i]->u.ch[1]) >= char_0 && c < char_1)
                    char_1 = c;                    /* backward chunk */
                if (posar[i]->u.ch[1] >= char_0 && posar[i]->u.ch[0] <= char_0)
                    tran_set = union_DFASet (poset, tran_set, followpos[i]);
            }
            if (tran_set)
            {
                add_DFA_state (dfas, &tran_set, &dfa_to);
                add_DFA_tran (dfas, dfa_from, char_0, char_1, dfa_to->no);
            }
        }
    }
    ifree (pos);
    sort_DFA_states (dfas);
}

static void pr_tran (struct DFA_parse *parse_info, struct DFA_states *dfas)
{
    struct DFA_state *s;
    struct DFA_tran *tran;
    int prev_no, i, c, no;

    for (no=0; no < dfas->no; ++no)
    {
        s = dfas->sortarray[no];
        assert (s->no == no);
        printf ("DFA state %d", no);
        if (s->rule_no)
            printf ("#%d", s->rule_no);
        if (s->rule_nno)
            printf (" #%d", s->rule_nno);

        putchar (':');
        pr_DFASet (parse_info->poset, s->set);
        prev_no = -1;
        for (i=s->tran_no, tran=s->trans; --i >= 0; tran++)
        {
            if (prev_no != tran->to)
            {
                if (prev_no != -1)
                    printf ("]\n");
                printf (" goto %d on [", tran->to);
                prev_no = tran->to;
            }
            for (c = tran->ch[0]; c <= tran->ch[1]; c++)
                printf ("%s", str_char(c));
        }
        if (prev_no != -1)
            printf ("]\n");
        putchar ('\n');
    }
}

static void pr_verbose (struct DFA_parse *parse_info, struct DFA_states *dfas)
{
    long i, j;
    int k;
    printf ("%d/%d tree nodes used, %ld bytes each\n",
        parse_info->use_Tnode, parse_info->max_Tnode, (long) sizeof (struct Tnode));
    k = inf_BSetHandle (parse_info->charset, &i, &j);
    printf ("%ld/%ld character sets, %ld bytes each\n",
            i/k, j/k, (long) k*sizeof(BSetWord));
    k = inf_DFASetType (parse_info->poset, &i, &j);
    printf ("%ld/%ld poset items, %d bytes each\n", i, j, k);
    printf ("%d DFA states\n", dfas->no);
}

static void pr_followpos (struct DFA_parse *parse_info)
{
    struct Tnode **posar = parse_info->posar;
    int i;
    printf ("\nfollowsets:\n");
    for (i=1; i <= parse_info->position; i++)
    {
        printf ("%3d:", i);
        pr_DFASet (parse_info->poset, parse_info->followpos[i]);
        putchar ('\t');

        if (posar[i]->u.ch[0] < 0)
            printf ("#%d", -posar[i]->u.ch[0]);
        else if (posar[i]->u.ch[1] > posar[i]->u.ch[0])
        {
            putchar ('[');
            out_char (posar[i]->u.ch[0]);
            if (posar[i]->u.ch[1] > posar[i]->u.ch[0]+1)
                putchar ('-');
            out_char (posar[i]->u.ch[1]);
            putchar (']');
        }
        else
            out_char (posar[i]->u.ch[0]);
        putchar ('\n');
    }
    putchar ('\n');
}

void dfa_parse_cmap_clean (struct DFA *d)
{
    struct DFA_parse *dfa = d->parse_info;

    assert (dfa);
    if (!dfa->charMap)
    {
        dfa->charMapSize = 7;
        dfa->charMap = (int *)
	    imalloc (dfa->charMapSize * sizeof(*dfa->charMap));
    }
    dfa->charMap[0] = 0;
}

void dfa_parse_cmap_new (struct DFA *d, const int *cmap)
{
    struct DFA_parse *dfa = d->parse_info;
    const int *cp;
    int size;

    assert (dfa);
    for (cp = cmap; *cp; cp += 2)
        ;
    size = cp - cmap + 1;
    if (size > dfa->charMapSize)
    {
        if (dfa->charMap)
            ifree (dfa->charMap);
        dfa->charMapSize = size;
        dfa->charMap = (int *) imalloc (size * sizeof(*dfa->charMap));
    }
    memcpy (dfa->charMap, cmap, size * sizeof(*dfa->charMap));
}

void dfa_parse_cmap_del (struct DFA *d, int from)
{
    struct DFA_parse *dfa = d->parse_info;
    int *cc;

    assert (dfa);
    for (cc = dfa->charMap; *cc; cc += 2)
        if (*cc == from)
        {
            while ((cc[0] = cc[2]))
            {
                cc[1] = cc[3];
                cc += 2;
            }
            break;
        }
}

void dfa_parse_cmap_add (struct DFA *d, int from, int to)
{
    struct DFA_parse *dfa = d->parse_info;
    int *cc;
    int indx, size;

    assert (dfa);
    for (cc = dfa->charMap; *cc; cc += 2)
        if (*cc == from)
        {
            cc[1] = to;
            return ;
        }
    indx = cc - dfa->charMap;
    size = dfa->charMapSize;
    if (indx >= size)
    {
        int *cn = (int *) imalloc ((size+16) * sizeof(*dfa->charMap));
        memcpy (cn, dfa->charMap, indx*sizeof(*dfa->charMap));
        ifree (dfa->charMap);
        dfa->charMap = cn;
        dfa->charMapSize = size+16;
    }
    dfa->charMap[indx] = from;
    dfa->charMap[indx+1] = to;
    dfa->charMap[indx+2] = 0;
}

void dfa_parse_cmap_thompson (struct DFA *d)
{
    static int thompson_chars[] =
    {
        '*',  L_CLOS0,
        '+',  L_CLOS1,
        '|',  L_ALT,
        '^',  L_START,
        '$',  L_END,
        '?',  L_QUEST,
        '.',  L_ANY,
        '(',  L_LP,
        ')',  L_RP,
        ' ',  0,
        '\t', 0,
        '\n', 0,
        0
    };
    dfa_parse_cmap_new (d, thompson_chars);
}

static struct DFA_parse *dfa_parse_init (void)
{
    struct DFA_parse *parse_info =
	(struct DFA_parse *) imalloc (sizeof (struct DFA_parse));

    parse_info->charset = mk_BSetHandle (255, 20);
    parse_info->position = 0;
    parse_info->rule = 0;
    parse_info->root = NULL;

    /* initialize the anyset which by default does not include \n */
    parse_info->anyset = mk_BSet (&parse_info->charset);
    res_BSet (parse_info->charset, parse_info->anyset);
    add_BSet (parse_info->charset, parse_info->anyset, '\n');
    com_BSet (parse_info->charset, parse_info->anyset);

    parse_info->use_Tnode = parse_info->max_Tnode = 0;
    parse_info->start = parse_info->end = NULL;
    parse_info->charMap = NULL;
    parse_info->charMapSize = 0;
    parse_info->cmap = NULL;
    return parse_info;
}

static void rm_dfa_parse (struct DFA_parse **dfap)
{
    struct DFA_parse *parse_info = *dfap;
    assert (parse_info);
    term_Tnode(parse_info);
    rm_BSetHandle (&parse_info->charset);
    ifree (parse_info->charMap);
    ifree (parse_info);
    *dfap = NULL;
}

static struct DFA_states *mk_dfas (struct DFA_parse *dfap, int poset_chunk)
{
    struct DFA_states *dfas;
    struct DFA_parse *parse_info = dfap;

    assert (poset_chunk > 10);
    assert (dfap);

    parse_info->poset = mk_DFASetType (poset_chunk);
    init_pos(parse_info);
    init_followpos(parse_info);
    assert (parse_info->root);
    dfa_trav (parse_info, parse_info->root);

    if (debug_dfa_followpos)
        pr_followpos(parse_info);
    init_DFA_states (&dfas, parse_info->poset, (int) (STATE_HASH));
    mk_dfa_tran (parse_info, dfas);
    if (debug_dfa_tran)
    {
        pr_tran (parse_info, dfas);
    }
    if (dfa_verbose)
        pr_verbose (parse_info, dfas);
    del_pos(parse_info);
    del_followpos(parse_info);
    rm_DFASetType (parse_info->poset);
    return dfas;
}

struct DFA *dfa_init (void)
{
    struct DFA *dfa;

    dfa = (struct DFA *) imalloc (sizeof(*dfa));
    dfa->parse_info = dfa_parse_init ();
    dfa->state_info = NULL;
    dfa->states = NULL;
    dfa_parse_cmap_thompson (dfa);
    return dfa;
}

void dfa_anyset_includes_nl(struct DFA *dfa)
{
    add_BSet (dfa->parse_info->charset, dfa->parse_info->anyset, '\n');
}

void dfa_set_cmap (struct DFA *dfa, void *vp,
		   const char **(*cmap)(void *vp, const char **from, int len))
{
    dfa->parse_info->cmap = cmap;
    dfa->parse_info->cmap_data = vp;
}

int dfa_get_last_rule (struct DFA *dfa)
{
    return dfa->parse_info->rule;
}

int dfa_parse (struct DFA *dfa, const char **pattern)
{
    struct Tnode *top;
    struct DFA_parse *parse_info;

    assert (dfa);
    assert (dfa->parse_info);
    parse_info = dfa->parse_info;

    do_parse (parse_info, pattern, &top);
    if (parse_info->err_code)
        return parse_info->err_code;
    if ( !dfa->parse_info->root )
        dfa->parse_info->root = top;
    else
    {
        struct Tnode *n;

        n = mk_Tnode (parse_info);
        n->pos = OR;
        n->u.p[0] = dfa->parse_info->root;
        n->u.p[1] = top;
        dfa->parse_info->root = n;
    }
    return 0;
}

void dfa_mkstate (struct DFA *dfa)
{
    assert (dfa);

    dfa->state_info = mk_dfas (dfa->parse_info, POSET_CHUNK);
    dfa->no_states = dfa->state_info->no;
    dfa->states = dfa->state_info->sortarray;
    rm_dfa_parse (&dfa->parse_info);
}

void dfa_delete (struct DFA **dfap)
{
    assert (dfap);
    assert (*dfap);
    if ((*dfap)->parse_info)
        rm_dfa_parse (&(*dfap)->parse_info);
    if ((*dfap)->state_info)
        rm_DFA_states (&(*dfap)->state_info);
    ifree (*dfap);
    *dfap = NULL;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

