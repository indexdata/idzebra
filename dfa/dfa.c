/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dfa.c,v $
 * Revision 1.10  1996-01-08 09:09:17  adam
 * Function dfa_parse got 'const' string argument.
 * New functions to define char mappings made public.
 *
 * Revision 1.9  1995/12/06  12:24:58  adam
 * Removed verbatim mode code.
 *
 * Revision 1.8  1995/12/06  09:09:58  adam
 * Work on left and right anchors.
 *
 * Revision 1.7  1995/11/27  09:23:02  adam
 * New berbatim hook in regular expressions. "[]n ..".
 *
 * Revision 1.6  1995/10/16  09:31:25  adam
 * Bug fix.
 *
 * Revision 1.5  1995/10/02  15:17:58  adam
 * Bug fix in dfa_delete.
 *
 * Revision 1.4  1995/09/28  09:18:52  adam
 * Removed various preprocessor defines.
 *
 * Revision 1.3  1995/09/04  12:33:26  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.2  1995/01/25  11:30:50  adam
 * Simple error reporting when parsing regular expressions.
 * Memory usage reduced.
 *
 * Revision 1.1  1995/01/24  16:02:52  adam
 * New private header file in dfa module (dfap.h).
 * Module no longer uses yacc to parse regular expressions.
 *
 */
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <alexutil.h>
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
    Set firstpos;             /* first positions */
    Set lastpos;              /* last positions */
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

static struct DFA_parse *parse_info = NULL;

static int err_code;
static int inside_string;
static const unsigned char *expr_ptr;
static struct Tnode **posar;

static SetType poset;
static Set *followpos;

static struct Tnode *mk_Tnode      (void);
static struct Tnode *mk_Tnode_cset (BSet charset);
static void        term_Tnode      (void);

static void 
    del_followpos  (void), 
    init_pos       (void), 
    del_pos        (void),
    mk_dfa_tran    (struct DFA_states *dfas),
    add_follow     (Set lastpos, Set firstpos),
    dfa_trav       (struct Tnode *n),
    init_followpos (void),
    pr_tran        (struct DFA_states *dfas),
    pr_verbose     (struct DFA_states *dfas),
    pr_followpos   (void),
    out_char       (int c),
    lex            (void);

static int
    nextchar       (int *esc),
    read_charset   (void);

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

static int lookahead;
static unsigned look_ch;
static BSet look_chars;

static struct Tnode *expr_1 (void),
                    *expr_2 (void),
                    *expr_3 (void),
                    *expr_4 (void);

static struct Tnode *expr_1 (void)
{ 
    struct Tnode *t1, *t2, *tn;

    if (!(t1 = expr_2 ()))
        return t1;
    while (lookahead == L_ALT)
    {
        lex ();
        if (!(t2 = expr_2 ()))
            return t2;
        
        tn = mk_Tnode ();
        tn->pos = OR;
        tn->u.p[0] = t1;
        tn->u.p[1] = t2;
        t1 = tn;
    }
    return t1;
}

static struct Tnode *expr_2 (void)
{
    struct Tnode *t1, *t2, *tn;
    
    if (!(t1 = expr_3 ()))
        return t1;
    while (lookahead == L_WILD ||
           lookahead == L_ANYZ ||
           lookahead == L_ANY ||
           lookahead == L_LP ||
           lookahead == L_CHAR ||
           lookahead == L_CHARS)
    {
        if (!(t2 = expr_3 ()))
            return t2;
        
        tn = mk_Tnode ();
        tn->pos = CAT;
        tn->u.p[0] = t1;
        tn->u.p[1] = t2;
        t1 = tn;
    }
    return t1;
}

static struct Tnode *expr_3 (void)
{
    struct Tnode *t1, *tn;

    if (!(t1 = expr_4 ()))
        return t1;
    if (lookahead == L_CLOS0)
    {
        lex ();
        tn = mk_Tnode ();
        tn->pos = STAR;
        tn->u.p[0] = t1;
        t1 = tn;
    }
    else if (lookahead == L_CLOS1)
    {
        lex ();
        tn = mk_Tnode ();
        tn->pos = PLUS;
        tn->u.p[0] = t1;
        t1 = tn;
    }
    else if (lookahead == L_QUEST)
    {
        lex ();
        tn = mk_Tnode();
        tn->pos = OR;
        tn->u.p[0] = t1;
        tn->u.p[1] = mk_Tnode();
        tn->u.p[1]->pos = EPSILON;
        t1 = tn;
    }
    return t1;
}

static struct Tnode *expr_4 (void)
{
    struct Tnode *t1;

    switch (lookahead)
    {
    case L_LP:
        lex ();
        if (!(t1 = expr_1 ()))
            return t1;
        if (lookahead == L_RP)
            lex ();
        else
            return NULL;
        break;
    case L_CHAR:
        t1 = mk_Tnode();
        t1->pos = ++parse_info->position;
        t1->u.ch[1] = t1->u.ch[0] = look_ch;
        lex ();
        break;
    case L_CHARS:
        t1 = mk_Tnode_cset (look_chars);
        lex ();
        break;
    case L_ANY:
        t1 = mk_Tnode_cset (parse_info->anyset);
        lex ();
        break;
    case L_ANYZ:
        t1 = mk_Tnode();
        t1->pos = OR;
        t1->u.p[0] = mk_Tnode_cset (parse_info->anyset);
        t1->u.p[1] = mk_Tnode();
        t1->u.p[1]->pos = EPSILON;
        lex ();
        break;
    case L_WILD:
        t1 = mk_Tnode();
        t1->pos = STAR;
        t1->u.p[0] = mk_Tnode_cset (parse_info->anyset);
        lex ();
    default:
        t1 = NULL;
    }
    return t1;
}

static void do_parse (struct DFA_parse *dfap, const char **s, struct Tnode **tnp)
{
    int anchor_flag = 0;
    int start_anchor_flag = 0;
    struct Tnode *t1, *t2, *tn;

    parse_info = dfap;
    err_code = 0;
    expr_ptr = (unsigned char *) *s;

    inside_string = 0;
    lex ();
    if (lookahead == L_START)
    {
        start_anchor_flag = 1;
        lex ();
    }
    t1 = expr_1 ();
    if (anchor_flag)
    {
        tn = mk_Tnode ();
        tn->pos = CAT;
        tn->u.p[0] = t2;
        tn->u.p[1] = t1;
        t1 = tn;
    }
    if (lookahead == L_END && t1)
    {
        t2 = mk_Tnode ();
        t2->pos = ++parse_info->position;
        t2->u.ch[1] = t2->u.ch[0] = '\n';

        tn = mk_Tnode ();
        tn->pos = CAT;
        tn->u.p[0] = t1;
        tn->u.p[1] = t2;
        t1 = tn;
        
        anchor_flag |= 2;
        lex ();
    }
    if (lookahead == 0 && t1)
    {
        t2 = mk_Tnode();
        t2->pos = ++parse_info->position;
        t2->u.ch[0] = -(++parse_info->rule);
        t2->u.ch[1] = start_anchor_flag ? 0 : -(parse_info->rule);
        
        *tnp = mk_Tnode();
        (*tnp)->pos = CAT;
        (*tnp)->u.p[0] = t1;
        (*tnp)->u.p[1] = t2;
    }
    else
    {
        if (!err_code)
        {
            if (lookahead == L_RP)
                err_code = DFA_ERR_RP;
            else if (lookahead == L_LP)
                err_code = DFA_ERR_LP;
            else
                err_code = DFA_ERR_SYNTAX;
        }
    }
    *s = (char *) expr_ptr;
}

static int nextchar (int *esc)
{
    *esc = 0;
    if (*expr_ptr == '\0')
        return 0;
    else if (*expr_ptr != '\\')
        return *expr_ptr++;
    *esc = 1;
    switch (*++expr_ptr)
    {
    case '\r':
    case '\n':
    case '\0':
        return '\\';
    case '\t':
        ++expr_ptr;
        return ' ';
    case 'n':
        ++expr_ptr;
        return '\n';
    case 't':
        ++expr_ptr;
        return '\t';
    case 'r':
        ++expr_ptr;
        return '\r';
    case 'f':
        ++expr_ptr;
        return '\f';
    default:
        return *expr_ptr++;
    }
}

static int nextchar_set (int *esc)
{
    if (*expr_ptr == ' ')
    {
        *esc = 0;
        return *expr_ptr++;
    }
    return nextchar (esc);
}

static int read_charset (void)
{
    int i, ch0, ch1, esc0, esc1, cc = 0;
    look_chars = mk_BSet (&parse_info->charset);
    res_BSet (parse_info->charset, look_chars);

    ch0 = nextchar_set (&esc0);
    if (!esc0 && ch0 == '^')
    {
        cc = 1;
        ch0 = nextchar_set (&esc0);
    }
    while (ch0 != 0)
    {
        if (!esc0 && ch0 == ']')
            break;
        add_BSet (parse_info->charset, look_chars, ch0);
        ch1 = nextchar_set (&esc1);
        if (!esc1 && ch1 == '-')
        {
            if ((ch1 = nextchar_set (&esc1)) == 0)
                break;
            if (!esc1 && ch1 == ']')
            {
                add_BSet (parse_info->charset, look_chars, '-');
                break;
            }
            for (i=ch0; ++i<=ch1;)
                add_BSet (parse_info->charset, look_chars, i);
            ch0 = nextchar_set (&esc0);
        }
        else
        {
            esc0 = esc1;
            ch0 = ch1;
        }
    }
    if (cc)
        com_BSet (parse_info->charset, look_chars);
    return L_CHARS;
}

static int lex_sub(void)
{
    int esc;
    while ((look_ch = nextchar (&esc)) != 0)
        if (look_ch == '\"')
        {
            if (esc)
                return L_CHAR;
            inside_string = !inside_string;
        }
        else if (esc || inside_string)
            return L_CHAR;
        else if (look_ch == '[')
            return read_charset();
        else 
        {
            const int *cc;
            if (look_ch == '/')
                logf (LOG_DEBUG, "xxxx / xxx");
            for (cc = parse_info->charMap; *cc; cc += 2)
                if (*cc == look_ch)
                {
                    if (!cc[1])
                        --expr_ptr;
                    return cc[1];
                }
            return L_CHAR;            
        }
    return 0;
}

static void lex (void)
{
    lookahead = lex_sub ();
}

static const char *str_char (unsigned c)
{
    static char s[6];
    s[0] = '\\';
    if (c < 32)
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

static void term_Tnode (void)
{
    struct Tblock *t, *t1;
    for (t = parse_info->start; (t1 = t);)
    {
        t=t->next;
        ifree (t1->tarray);
        ifree (t1);
    }
}

static struct Tnode *mk_Tnode (void)
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

struct Tnode *mk_Tnode_cset (BSet charset)
{
    struct Tnode *tn1, *tn0 = mk_Tnode();
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
                tn1 = mk_Tnode();
                tn1->pos = OR;
                tn1->u.p[0] = tn0;
                tn0 = tn1;
                tn1 = tn0->u.p[1] = mk_Tnode();
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

static void del_followpos (void)
{
    ifree (followpos);
}

static void init_pos (void)
{
    posar = (struct Tnode **) imalloc (sizeof(struct Tnode*) 
                                       * (1+parse_info->position));
}

static void del_pos (void)
{
    ifree (posar);
}

static void add_follow (Set lastpos, Set firstpos)
{
    while (lastpos)
    {
        followpos[lastpos->value] = 
               union_Set (poset, followpos[lastpos->value], firstpos);
        lastpos = lastpos->next;
    }                                                            
}

static void dfa_trav (struct Tnode *n)
{
    switch (n->pos)
    {
    case CAT:
        dfa_trav (n->u.p[0]);
        dfa_trav (n->u.p[1]);
        n->nullable = n->u.p[0]->nullable & n->u.p[1]->nullable;
        n->firstpos = mk_Set (poset);
        n->firstpos = union_Set (poset, n->firstpos, n->u.p[0]->firstpos);
        if (n->u.p[0]->nullable)
            n->firstpos = union_Set (poset, n->firstpos, n->u.p[1]->firstpos);
        n->lastpos = mk_Set (poset);
        n->lastpos = union_Set (poset, n->lastpos, n->u.p[1]->lastpos);
        if (n->u.p[1]->nullable)
            n->lastpos = union_Set (poset, n->lastpos, n->u.p[0]->lastpos);

        add_follow (n->u.p[0]->lastpos, n->u.p[1]->firstpos);

        n->u.p[0]->firstpos = rm_Set (poset, n->u.p[0]->firstpos);
        n->u.p[0]->lastpos = rm_Set (poset, n->u.p[0]->lastpos);
        n->u.p[1]->firstpos = rm_Set (poset, n->u.p[1]->firstpos);
        n->u.p[1]->lastpos = rm_Set (poset, n->u.p[1]->lastpos);
        if (debug_dfa_trav)
            printf ("CAT");
        break;
    case OR:
        dfa_trav (n->u.p[0]);
        dfa_trav (n->u.p[1]);
        n->nullable = n->u.p[0]->nullable | n->u.p[1]->nullable;

        n->firstpos = merge_Set (poset, n->u.p[0]->firstpos,
                                 n->u.p[1]->firstpos);
        n->lastpos = merge_Set (poset, n->u.p[0]->lastpos,
                                n->u.p[1]->lastpos);
        n->u.p[0]->firstpos = rm_Set (poset, n->u.p[0]->firstpos);
        n->u.p[0]->lastpos = rm_Set (poset, n->u.p[0]->lastpos);
        n->u.p[1]->firstpos = rm_Set (poset, n->u.p[1]->firstpos);
        n->u.p[1]->lastpos = rm_Set (poset, n->u.p[1]->lastpos);
        if (debug_dfa_trav)
            printf ("OR");
        break;
    case PLUS:
        dfa_trav (n->u.p[0]);
        n->nullable = n->u.p[0]->nullable;
        n->firstpos = n->u.p[0]->firstpos;
        n->lastpos = n->u.p[0]->lastpos;
        add_follow (n->lastpos, n->firstpos);
        if (debug_dfa_trav)
            printf ("PLUS");
        break;
    case STAR:
        dfa_trav (n->u.p[0]);
        n->nullable = 1;
        n->firstpos = n->u.p[0]->firstpos;
        n->lastpos = n->u.p[0]->lastpos;
        add_follow (n->lastpos, n->firstpos);
        if (debug_dfa_trav)
            printf ("STAR");
        break;
    case EPSILON:
        n->nullable = 1;
        n->lastpos = mk_Set (poset);
        n->firstpos = mk_Set (poset);
        if (debug_dfa_trav)
            printf ("EPSILON");
        break;
    default:
        posar[n->pos] = n;
        n->nullable = 0;
        n->firstpos = mk_Set (poset);
        n->firstpos = add_Set (poset, n->firstpos, n->pos);
        n->lastpos = mk_Set (poset);
        n->lastpos = add_Set (poset, n->lastpos, n->pos);
        if (debug_dfa_trav)
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
    if (debug_dfa_trav)
    {
        printf ("\n nullable : %c\n", n->nullable ? '1' : '0');
        printf (" firstpos :");
        pr_Set (poset, n->firstpos);
        printf (" lastpos  :");
        pr_Set (poset, n->lastpos);
    }
}

static void init_followpos (void)
{
    Set *fa;
    int i;
    followpos = fa = (Set *) imalloc (sizeof(Set) * (1+parse_info->position));
    for (i = parse_info->position+1; --i >= 0; fa++)
        *fa = mk_Set (poset);
}

static void mk_dfa_tran (struct DFA_states *dfas)
{
    int i, j, c;
    int max_char;
    short *pos, *pos_i;
    Set tran_set;
    int char_0, char_1;
    struct DFA_state *dfa_from, *dfa_to;

    assert (parse_info);
    max_char = parse_info->charset->size;
    pos = (short *) imalloc (sizeof(*pos) * (parse_info->position+1));

    tran_set = cp_Set (poset, parse_info->root->firstpos);
    i = add_DFA_state (dfas, &tran_set, &dfa_from);
    assert (i);
    dfa_from->rule_no = 0;
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
                    if (c < char_1)
                        char_0 = char_1;
                    else
                        char_0 = c;

            if (char_0 > max_char)
                break;

            char_1 = max_char;
                
            tran_set = mk_Set (poset);
            for (pos_i = pos; (i = *pos_i) != -1; ++pos_i)
            {
                if ((c=posar[i]->u.ch[0]) > char_0 && c <= char_1)
                    char_1 = c - 1;                /* forward chunk */
                else if ((c=posar[i]->u.ch[1]) >= char_0 && c < char_1)
                    char_1 = c;                    /* backward chunk */
                if (posar[i]->u.ch[1] >= char_0 && posar[i]->u.ch[0] <= char_0)
                    tran_set = union_Set (poset, tran_set, followpos[i]);
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

static void pr_tran (struct DFA_states *dfas)
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
        pr_Set (poset, s->set);
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

static void pr_verbose (struct DFA_states *dfas)
{
    long i, j;
    int k;
    printf ("%d/%d tree nodes used, %d bytes each\n",
        parse_info->use_Tnode, parse_info->max_Tnode, sizeof (struct Tnode));
    k = inf_BSetHandle (parse_info->charset, &i, &j);
    printf ("%ld/%ld character sets, %d bytes each\n",
            i/k, j/k, k*sizeof(BSetWord));
    k = inf_SetType (poset, &i, &j);
    printf ("%ld/%ld poset items, %d bytes each\n", i, j, k);
    printf ("%d DFA states\n", dfas->no);
}

static void pr_followpos (void)
{
    int i;
    printf ("\nfollowsets:\n");
    for (i=1; i <= parse_info->position; i++)
    {
        printf ("%3d:", i);
        pr_Set (poset, followpos[i]);
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
        dfa->charMap = imalloc (dfa->charMapSize * sizeof(*dfa->charMap));
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
        dfa->charMap = imalloc (size * sizeof(*dfa->charMap));
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
        int *cn = imalloc ((size+16) * sizeof(*dfa->charMap));
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
    parse_info = (struct DFA_parse *) imalloc (sizeof (struct DFA_parse));

    parse_info->charset = mk_BSetHandle (255, 20);
    parse_info->position = 0;
    parse_info->rule = 0;
    parse_info->root = NULL;

    parse_info->anyset = mk_BSet (&parse_info->charset);
    res_BSet (parse_info->charset, parse_info->anyset);
    add_BSet (parse_info->charset, parse_info->anyset, '\n');
    com_BSet (parse_info->charset, parse_info->anyset);
    parse_info->use_Tnode = parse_info->max_Tnode = 0;
    parse_info->charMap = NULL;
    parse_info->charMapSize = 0;
    return parse_info;
}

static void rm_dfa_parse (struct DFA_parse **dfap)
{
    parse_info = *dfap;
    assert (parse_info);
    term_Tnode();
    rm_BSetHandle (&parse_info->charset);
    ifree (parse_info->charMap);
    ifree (parse_info);
    *dfap = NULL;
}

static struct DFA_states *mk_dfas (struct DFA_parse *dfap, int poset_chunk)
{
    struct DFA_states *dfas;

    assert (poset_chunk > 10);
    assert (dfap);

    parse_info = dfap;
    poset = mk_SetType (poset_chunk);
    init_pos();
    init_followpos();
    assert (parse_info->root);
    dfa_trav (parse_info->root);

    if (debug_dfa_followpos)
        pr_followpos();
    init_DFA_states (&dfas, poset, STATE_HASH);
    mk_dfa_tran (dfas);
    if (debug_dfa_tran)
        pr_tran (dfas);
    if (dfa_verbose)
        pr_verbose (dfas);
    del_pos();
    del_followpos();
    rm_SetType (poset);
    return dfas;
}

struct DFA *dfa_init (void)
{
    struct DFA *dfa;

    dfa = imalloc (sizeof(*dfa));
    dfa->parse_info = dfa_parse_init ();
    dfa->state_info = NULL;
    dfa->states = NULL;
    dfa_parse_cmap_thompson (dfa);
    return dfa;
}

int dfa_parse (struct DFA *dfa, const char **pattern)
{
    struct Tnode *top;

    assert (dfa);
    assert (dfa->parse_info);
    do_parse (dfa->parse_info, pattern, &top);
    if (err_code)
        return err_code;
    if ( !dfa->parse_info->root )
        dfa->parse_info->root = top;
    else
    {
        struct Tnode *n;

        n = mk_Tnode ();
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
