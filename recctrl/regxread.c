/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: regxread.c,v $
 * Revision 1.4  1997-02-12 20:42:58  adam
 * Changed some log messages.
 *
 * Revision 1.3  1996/11/08 14:05:33  adam
 * Bug fix: data1 node member u.tag.get_bytes weren't initialized.
 *
 * Revision 1.2  1996/10/29  14:02:09  adam
 * Doesn't use the global data1_tabpath (from YAZ). Instead the function
 * data1_get_tabpath is used.
 *
 * Revision 1.1  1996/10/11 10:57:30  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 * Revision 1.24  1996/06/17 14:25:31  adam
 * Removed LOG_DEBUG logs; can still be enabled by setting REGX_DEBUG.
 *
 * Revision 1.23  1996/06/04 10:19:00  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.22  1996/06/03  15:23:13  adam
 * Bug fix: /../ BODY /../ - pattern didn't match EOF.
 *
 * Revision 1.21  1996/05/14  16:58:38  adam
 * Minor change.
 *
 * Revision 1.20  1996/05/01  13:46:36  adam
 * First work on multiple records in one file.
 * New option, -offset, to the "unread" command in the filter module.
 *
 * Revision 1.19  1996/02/12  16:18:20  adam
 * Yet another bug fix in implementation of unread command.
 *
 * Revision 1.18  1996/02/12  16:07:54  adam
 * Bug fix in new unread command.
 *
 * Revision 1.17  1996/02/12  15:56:11  adam
 * New code command: unread.
 *
 * Revision 1.16  1996/01/17  14:57:51  adam
 * Prototype changed for reader functions in extract/retrieve. File
 *  is identified by 'void *' instead of 'int.
 *
 * Revision 1.15  1996/01/08  19:15:47  adam
 * New input filter that works!
 *
 * Revision 1.14  1996/01/08  09:10:38  adam
 * Yet another complete rework on this module.
 *
 * Revision 1.13  1995/12/15  17:21:50  adam
 * This version is able to set data.formatted_text in data1-nodes.
 *
 * Revision 1.12  1995/12/15  16:20:10  adam
 * The filter files (*.flt) are read from the path given by data1_tabpath.
 *
 * Revision 1.11  1995/12/15  12:35:16  adam
 * Better logging.
 *
 * Revision 1.10  1995/12/15  10:35:36  adam
 * Misc. bug fixes.
 *
 * Revision 1.9  1995/12/14  16:38:48  adam
 * Completely new attempt to make regular expression parsing.
 *
 * Revision 1.8  1995/12/13  17:16:59  adam
 * Small changes.
 *
 * Revision 1.7  1995/12/13  16:51:58  adam
 * Modified to set last_child in data1_nodes.
 * Uses destroy handler to free up data text nodes.
 *
 * Revision 1.6  1995/12/13  13:45:37  quinn
 * Changed data1 to use nmem.
 *
 * Revision 1.5  1995/12/11  09:12:52  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.4  1995/12/05  16:57:40  adam
 * More work on regular patterns.
 *
 * Revision 1.3  1995/12/05  09:37:09  adam
 * One malloc was renamed to xmalloc.
 *
 * Revision 1.2  1995/12/04  17:59:24  adam
 * More work on regular expression conversion.
 *
 * Revision 1.1  1995/12/04  14:25:30  adam
 * Started work on regular expression parsed input to structured records.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <tpath.h>
#include <zebrautl.h>
#include <dfa.h>
#include "grsread.h"

#define REGX_DEBUG 0

#define F_WIN_EOF 2000000000
#define F_WIN_READ 1

#define REGX_EOF     0
#define REGX_PATTERN 1
#define REGX_BODY    2
#define REGX_BEGIN   3
#define REGX_END     4
#define REGX_CODE    5

struct regxCode {
    char *str;
};

struct lexRuleAction {
    int which; 
    union {
        struct {
            struct DFA *dfa;        /* REGX_PATTERN */
            int body;
        } pattern;
        struct regxCode *code;  /* REGX_CODE */
    } u;
    struct lexRuleAction *next;
};

struct lexRuleInfo {
    int no;
    struct lexRuleAction *actionList;
};

struct lexRule {
    struct lexRuleInfo info;
    struct lexRule *next;
};

struct lexTrans {
    struct DFA *dfa;
    struct lexRule *rules;
    struct lexRuleInfo **fastRule;
    int ruleNo;
};

struct lexSpec {
    char *name;
    struct lexTrans trans;
    int lineNo;
    NMEM m;
    void *f_win_fh;
    void (*f_win_ef)(void *, off_t);
#if F_WIN_READ
    int f_win_start;
    int f_win_end;
    int f_win_size;
    char *f_win_buf;
    int (*f_win_rf)(void *, char *, size_t);
    off_t (*f_win_sf)(void *, off_t);
#else
    char *scan_buf;
    int scan_size;
#endif
    struct lexRuleAction *beginActionList;
    struct lexRuleAction *endActionList;
};

#if F_WIN_READ
static char *f_win_get (struct lexSpec *spec, off_t start_pos, off_t end_pos,
                        int *size)
{
    int i, r, off;

    if (start_pos < spec->f_win_start || start_pos >= spec->f_win_end)
    {
        (*spec->f_win_sf)(spec->f_win_fh, start_pos);
        spec->f_win_start = start_pos;

        if (!spec->f_win_buf)
            spec->f_win_buf = xmalloc (spec->f_win_size);
        *size = (*spec->f_win_rf)(spec->f_win_fh, spec->f_win_buf,
                                  spec->f_win_size);
        spec->f_win_end = spec->f_win_start + *size;

        if (*size > end_pos - start_pos)
            *size = end_pos - start_pos;
        return spec->f_win_buf;
    }
    if (end_pos <= spec->f_win_end)
    {
        *size = end_pos - start_pos;
        return spec->f_win_buf + (start_pos - spec->f_win_start);
    }
    off = start_pos - spec->f_win_start;
    for (i = 0; i<spec->f_win_end - start_pos; i++)
        spec->f_win_buf[i] = spec->f_win_buf[i + off];
    r = (*spec->f_win_rf)(spec->f_win_fh,
                          spec->f_win_buf + i,
                          spec->f_win_size - i);
    spec->f_win_start = start_pos;
    spec->f_win_end += r;
    *size = i + r;
    if (*size > end_pos - start_pos)
        *size = end_pos - start_pos;
    return spec->f_win_buf;
}

static int f_win_advance (struct lexSpec *spec, int *pos)
{
    int size;
    char *buf;
    
    if (*pos >= spec->f_win_start && *pos < spec->f_win_end)
        return spec->f_win_buf[(*pos)++ - spec->f_win_start];
    if (*pos == F_WIN_EOF)
        return 0;
    buf = f_win_get (spec, *pos, *pos+1, &size);
    if (size == 1)
    {
        (*pos)++;
        return *buf;
    }
    *pos = F_WIN_EOF;
    return 0;
}
#endif

static void regxCodeDel (struct regxCode **pp)
{
    struct regxCode *p = *pp;
    if (p)
    {
        xfree (p->str); 
        xfree (p);
        *pp = NULL;
    }
}

static void regxCodeMk (struct regxCode **pp, const char *buf, int len)
{
    struct regxCode *p;

    p = xmalloc (sizeof(*p));
    p->str = xmalloc (len+1);
    memcpy (p->str, buf, len);
    p->str[len] = '\0';
    *pp = p;
}

static struct DFA *lexSpecDFA (void)
{
    struct DFA *dfa;

    dfa = dfa_init ();
    dfa_parse_cmap_del (dfa, ' ');
    dfa_parse_cmap_del (dfa, '\t');
    dfa_parse_cmap_add (dfa, '/', 0);
    return dfa;
}

static struct lexSpec *lexSpecMk (const char *name)
{
    struct lexSpec *p;

    p = xmalloc (sizeof(*p));
    p->name = xmalloc (strlen(name)+1);
    strcpy (p->name, name);
    p->trans.dfa = lexSpecDFA ();
    p->trans.rules = NULL;
    p->trans.fastRule = NULL;
    p->beginActionList = NULL;
    p->endActionList = NULL;
#if F_WIN_READ
    p->f_win_buf = NULL;
#endif
    return p;
}

static void actionListDel (struct lexRuleAction **rap)
{
    struct lexRuleAction *ra1, *ra;

    for (ra = *rap; ra; ra = ra1)
    {
        ra1 = ra->next;
        switch (ra->which)
        {
        case REGX_PATTERN:
            dfa_delete (&ra->u.pattern.dfa);
            break;
        case REGX_CODE:
            regxCodeDel (&ra->u.code);
            break;
        }
        xfree (ra);
    }
    *rap = NULL;
}

static void lexSpecDel (struct lexSpec **pp)
{
    struct lexSpec *p;
    struct lexRule *rp, *rp1;

    assert (pp);
    p = *pp;
    if (!p)
        return ;
    dfa_delete (&p->trans.dfa);
    xfree (p->name);
    xfree (p->trans.fastRule);
    for (rp = p->trans.rules; rp; rp = rp1)
    {
        actionListDel (&rp->info.actionList);
        xfree (rp);
    }
    actionListDel (&p->beginActionList);
    actionListDel (&p->endActionList);
#if F_WIN_READ
    xfree (p->f_win_buf);
#endif
    xfree (p);
    *pp = NULL;
}

static int readParseToken (const char **cpp, int *len)
{
    const char *cp = *cpp;
    char cmd[32];
    int i, level;

    while (*cp == ' ' || *cp == '\t' || *cp == '\n')
        cp++;
    switch (*cp)
    {
    case '\0':
        return 0;
    case '/':
        *cpp = cp+1;
        return REGX_PATTERN;
    case '{':
        *cpp = cp+1;
        level = 1;
        while (*++cp)
        {
            if (*cp == '{')
                level++;
            else if (*cp == '}')
            {
                level--;
                if (level == 0)
                    break;
            }
        }
        *len = cp - *cpp;
        return REGX_CODE;
    default:
        i = 0;
        while (1)
        {
            if (*cp >= 'a' && *cp <= 'z')
                cmd[i] = *cp;
            else if (*cp >= 'A' && *cp <= 'Z')
                cmd[i] = *cp + 'a' - 'A';
            else
                break;
            if (i > sizeof(cmd)-2)
                break;
            i++;
            cp++;
        }
        cmd[i] = '\0';
        if (i == 0)
        {
            logf (LOG_WARN, "bad character %d %c", *cp, *cp);
            cp++;
            while (*cp && *cp != ' ' && *cp != '\t' && *cp != '\n')
                cp++;
            *cpp = cp;
            return 0;
        }
        *cpp = cp;
        if (!strcmp (cmd, "begin"))
            return REGX_BEGIN;
        else if (!strcmp (cmd, "end"))
            return REGX_END;
        else if (!strcmp (cmd, "body"))
            return REGX_BODY;
        else
        {
            logf (LOG_WARN, "bad command %s", cmd);
            return 0;
        }
    }
}

static int actionListMk (struct lexSpec *spec, const char *s,
                         struct lexRuleAction **ap)
{
    int r, tok, len;
    int bodyMark = 0;

    while ((tok = readParseToken (&s, &len)))
    {
        switch (tok)
        {
        case REGX_BODY:
            bodyMark = 1;
            continue;
        case REGX_CODE:
            *ap = xmalloc (sizeof(**ap));
            (*ap)->which = tok;
            regxCodeMk (&(*ap)->u.code, s, len);
            s += len+1;
            break;
        case REGX_PATTERN:
            *ap = xmalloc (sizeof(**ap));
            (*ap)->which = tok;
            (*ap)->u.pattern.body = bodyMark;
            bodyMark = 0;
            (*ap)->u.pattern.dfa = lexSpecDFA ();
            r = dfa_parse ((*ap)->u.pattern.dfa, &s);
            if (r || *s != '/')
            {
                xfree (*ap);
                *ap = NULL;
                logf (LOG_WARN, "regular expression error. r=%d", r);
                return -1;
            }
            dfa_mkstate ((*ap)->u.pattern.dfa);
            s++;
            break;
        case REGX_BEGIN:
            logf (LOG_WARN, "cannot use begin here");
            continue;
        case REGX_END:
            *ap = xmalloc (sizeof(**ap));
            (*ap)->which = tok;
            break;
        }
        ap = &(*ap)->next;
    }
    *ap = NULL;
    return 0;
}

int readOneSpec (struct lexSpec *spec, const char *s)
{
    int tok, len;

    tok = readParseToken (&s, &len);
    if (tok == REGX_BEGIN)
    {
        actionListDel (&spec->beginActionList);
        actionListMk (spec, s, &spec->beginActionList);
    }
    else if (tok == REGX_END)
    {
        actionListDel (&spec->endActionList);
        actionListMk (spec, s, &spec->endActionList);
    }
    else if (tok == REGX_PATTERN)
    {
        int r;
        struct lexRule *rp;
        r = dfa_parse (spec->trans.dfa, &s);
        if (r)
        {
            logf (LOG_WARN, "regular expression error. r=%d", r);
            return -1;
        }
        if (*s != '/')
        {
            logf (LOG_WARN, "expects / at end of pattern. got %c", *s);
            return -1;
        }
        s++;
        rp = xmalloc (sizeof(*rp));
        rp->info.no = spec->trans.ruleNo++;
        rp->next = spec->trans.rules;
        spec->trans.rules = rp;
        actionListMk (spec, s, &rp->info.actionList);
    }
    return 0;
}

int readFileSpec (struct lexSpec *spec)
{
    char *lineBuf;
    int lineSize = 512;
    struct lexRule *rp;
    int c, i, errors = 0;
    FILE *spec_inf;

    lineBuf = xmalloc (1+lineSize);
    logf (LOG_LOG, "reading regx filter %s.flt", spec->name);
    sprintf (lineBuf, "%s.flt", spec->name);
    if (!(spec_inf = yaz_path_fopen (data1_get_tabpath(), lineBuf, "r")))
    {
        logf (LOG_ERRNO|LOG_WARN, "cannot read spec file %s", spec->name);
        xfree (lineBuf);
        return -1;
    }
    spec->lineNo = 0;
    spec->trans.ruleNo = 1;
    c = getc (spec_inf);
    while (c != EOF)
    {
        int off = 0;
        if (c == '#' || c == '\n' || c == ' ' || c == '\t')
        {
            while (c != '\n' && c != EOF)
                c = getc (spec_inf);
            spec->lineNo++;
            if (c == '\n')
                c = getc (spec_inf);
        }
        else
        {
            int addLine = 0;

            lineBuf[off++] = c;
            while (1)
            {
                int c1 = c;
                c = getc (spec_inf);
                if (c == EOF)
                    break;
                if (c1 == '\n')
                {
                    if (c != ' ' && c != '\t')
                        break;
                    addLine++;
                }
                lineBuf[off] = c;
                if (off < lineSize)
                    off++;
            }
            lineBuf[off] = '\0';
            readOneSpec (spec, lineBuf);
            spec->lineNo += addLine;
        }
    }
    fclose (spec_inf);
    xfree (lineBuf);
    spec->trans.fastRule = xmalloc (sizeof(*spec->trans.fastRule) *
                                   spec->trans.ruleNo);
    for (i = 0; i<spec->trans.ruleNo; i++)
        spec->trans.fastRule[i] = NULL;
    for (rp = spec->trans.rules; rp; rp = rp->next)
        spec->trans.fastRule[rp->info.no] = &rp->info;
    if (errors)
        return -1;
#if 0
    debug_dfa_trav = 1;
    debug_dfa_tran = 1;
    debug_dfa_followpos = 1;
    dfa_verbose = 1;
#endif
    dfa_mkstate (spec->trans.dfa);
    return 0;
}

static struct lexSpec *curLexSpec = NULL;

static void destroy_data (struct data1_node *n)
{
    assert (n->which == DATA1N_data);
    xfree (n->u.data.data);
}

static void execData (struct lexSpec *spec,
                      data1_node **d1_stack, int *d1_level,
                      const char *ebuf, int elen, int formatted_text)
{
    struct data1_node *res, *parent;

#if REGX_DEBUG
    if (elen > 40)
        logf (LOG_DEBUG, "execData %.15s ... %.*s", ebuf, 15, ebuf + elen-15);
    else if (elen > 0)
        logf (LOG_DEBUG, "execData %.*s", elen, ebuf);
    else 
        logf (LOG_DEBUG, "execData len=%d", elen);
#endif
        
        if (*d1_level <= 1)
        return;

    parent = d1_stack[*d1_level -1];
    assert (parent);
    if ((res=d1_stack[*d1_level]) && res->which == DATA1N_data)
    {
        if (elen + res->u.data.len <= DATA1_LOCALDATA)
            memcpy (res->u.data.data + res->u.data.len, ebuf, elen);
        else
        {
            char *nb = xmalloc (elen + res->u.data.len);
            memcpy (nb, res->u.data.data, res->u.data.len);
            memcpy (nb + res->u.data.len, ebuf, elen);
            if (res->u.data.len > DATA1_LOCALDATA)
                xfree (res->u.data.data);
            res->u.data.data = nb;
            res->destroy = destroy_data;
        }
        res->u.data.len += elen;
    }
    else
    {
        res = data1_mk_node (spec->m);
        res->parent = parent;
        res->which = DATA1N_data;
        res->u.data.what = DATA1I_text;
        res->u.data.len = elen;
        res->u.data.formatted_text = formatted_text;
        if (elen > DATA1_LOCALDATA)
        {
            res->u.data.data = xmalloc (elen);
            res->destroy = destroy_data;
        }
        else
            res->u.data.data = res->lbuf;
        memcpy (res->u.data.data, ebuf, elen);
        res->root = parent->root;
        
        parent->num_children++;
        parent->last_child = res;
        if (d1_stack[*d1_level])
            d1_stack[*d1_level]->next = res;
        else
            parent->child = res;
        d1_stack[*d1_level] = res;
    }
}

static void execDataP (struct lexSpec *spec,
                       data1_node **d1_stack, int *d1_level,
                       const char *ebuf, int elen, int formatted_text)
{
    execData (spec, d1_stack, d1_level, ebuf, elen, formatted_text);
}


static void tagBegin (struct lexSpec *spec, 
                      data1_node **d1_stack, int *d1_level,
                      const char *tag, int len)
{
    struct data1_node *parent = d1_stack[*d1_level -1];
    data1_element *elem = NULL;
    data1_node *partag = get_parent_tag(parent);
    data1_node *res;
    data1_element *e = NULL;
    int localtag = 0;

    if (*d1_level == 0)
    {
        logf (LOG_WARN, "in element begin. No record type defined");
        return ;
    }
    
    res = data1_mk_node (spec->m);
    res->parent = parent;
    res->which = DATA1N_tag;
    res->u.tag.tag = res->lbuf;
    res->u.tag.get_bytes = -1;

    if (len >= DATA1_LOCALDATA)
        len = DATA1_LOCALDATA-1;

    memcpy (res->u.tag.tag, tag, len);
    res->u.tag.tag[len] = '\0';
   
#if REGX_DEBUG 
    logf (LOG_DEBUG, "tag begin %s (%d)", res->u.tag.tag, *d1_level);
#endif
    if (parent->which == DATA1N_variant)
        return ;
    if (partag)
        if (!(e = partag->u.tag.element))
            localtag = 1;
    
    elem = data1_getelementbytagname (d1_stack[0]->u.root.absyn, e,
                                      res->u.tag.tag);
    
    res->u.tag.element = elem;
    res->u.tag.node_selected = 0;
    res->u.tag.make_variantlist = 0;
    res->u.tag.no_data_requested = 0;
    res->root = parent->root;
    parent->num_children++;
    parent->last_child = res;
    if (d1_stack[*d1_level])
        d1_stack[*d1_level]->next = res;
    else
        parent->child = res;
    d1_stack[*d1_level] = res;
    d1_stack[++(*d1_level)] = NULL;
}

static void tagEnd (struct lexSpec *spec, 
                    data1_node **d1_stack, int *d1_level,
                    const char *tag, int len)
{
    while (*d1_level > 1)
    {
        (*d1_level)--;
        if (!tag ||
            (strlen(d1_stack[*d1_level]->u.tag.tag) == len &&
             !memcmp (d1_stack[*d1_level]->u.tag.tag, tag, len)))
            break;
    }
#if REGX_DEBUG
    logf (LOG_DEBUG, "tag end (%d)", *d1_level);
#endif
}


static int tryMatch (struct lexSpec *spec, int *pptr, int *mptr,
                     struct DFA *dfa)
{
    struct DFA_state *state = dfa->states[0];
    struct DFA_tran *t;
    unsigned char c;
#if F_WIN_READ
    unsigned char c_prev = 0;
#endif
    int ptr = *pptr;
    int start_ptr = *pptr;
    int last_rule = 0;
    int last_ptr = 0;
    int i;

    while (1)
    {
#if F_WIN_READ
        c = f_win_advance (spec, &ptr);
        if (ptr == F_WIN_EOF)
        {
            if (last_rule)
            {
                *mptr = start_ptr;
                *pptr = last_ptr;
                return 1;
            }
            break;
        }
#else
        if (ptr == spec->scan_size)
        {
            if (last_rule)
            {
                *mptr = start_ptr;
                *pptr = last_ptr;
                return 1;
            }
            break;
        }
        c = spec->scan_buf[ptr++];
#endif
        t = state->trans;
        i = state->tran_no;
        while (1)
            if (--i < 0)
            {
                if (last_rule)
                {
                    *mptr = start_ptr;     /* match starts here */
                    *pptr = last_ptr;      /* match end here (+1) */
                    return 1;
                }
                state = dfa->states[0];
                start_ptr = ptr;
#if F_WIN_READ
                c_prev = c;
#endif
                break;
            }
            else if (c >= t->ch[0] && c <= t->ch[1])
            {
                state = dfa->states[t->to];
                if (state->rule_no)
                {
#if F_WIN_READ
                    if (c_prev == '\n')
                    {
                        last_rule = state->rule_no;
                        last_ptr = ptr;
                    }
                    else
                    {
                        last_rule = state->rule_nno;
                        last_ptr = ptr;
                    }
#else
                    last_rule = state->rule_no;
                    last_ptr = ptr;
#endif
                }
                break;
            }
            else
                t++;
    }
    return 0;
}

static int execTok (struct lexSpec *spec, const char **src,
                    int arg_no, int *arg_start, int *arg_end,
                    const char **tokBuf, int *tokLen)
{
    const char *s = *src;

    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
        return 0;
    if (*s == '$' && s[1] >= '0' && s[1] <= '9')
    {
        int n = 0;
        s++;
        while (*s >= '0' && *s <= '9')
            n = n*10 + (*s++ -'0');
        if (arg_no == 0)
        {
            *tokBuf = "";
            *tokLen = 0;
        }
        else
        {
            if (n >= arg_no)
                n = arg_no-1;
#if F_WIN_READ
            *tokBuf = f_win_get (spec, arg_start[n], arg_end[n], tokLen);
#else
            *tokBuf = spec->scan_buf + arg_start[n];
            *tokLen = arg_end[n] - arg_start[n];
#endif
        }
    }
    else if (*s == '\"')
    {
        *tokBuf = ++s;
        while (*s && *s != '\"')
            s++;
        *tokLen = s - *tokBuf;
        if (*s)
            s++;
        *src = s;
    }
    else if (*s == '\n' || *s == ';')
    {
        *src = s+1;
        return 1;
    }
    else if (*s == '-')
    {
        *tokBuf = s++;
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != ';')
            s++;
        *tokLen = s - *tokBuf;
        *src = s;
        return 3;
    }
    else
    {
        *tokBuf = s++;
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != ';')
            s++;
        *tokLen = s - *tokBuf;
    }
    *src = s;
    return 2;
}

static char *regxStrz (const char *src, int len)
{
    static char str[64];
    
    if (len > 63)
        len = 63;
    memcpy (str, src, len);
    str[len] = '\0';
    return str;
}

static int execCode (struct lexSpec *spec,
                     int arg_no, int *arg_start, int *arg_end, int *pptr,
                     struct regxCode *code,
                     data1_node **d1_stack, int *d1_level)
{
    const char *s = code->str;
    int cmd_len, r;
    int returnCode = 1;
    const char *cmd_str;
    
    r = execTok (spec, &s, arg_no, arg_start, arg_end, &cmd_str, &cmd_len);
    while (r)
    {
        char *p;
        
        if (r == 1)
        {
            r = execTok (spec, &s, arg_no, arg_start, arg_end,
                         &cmd_str, &cmd_len);
            continue;
        }
        p = regxStrz (cmd_str, cmd_len);
        if (!strcmp (p, "begin"))
        {
            r = execTok (spec, &s, arg_no, arg_start, arg_end,
                         &cmd_str, &cmd_len);
            if (r < 2)
                continue;
            p = regxStrz (cmd_str, cmd_len);
            if (!strcmp (p, "record"))
            {
                r = execTok (spec, &s, arg_no, arg_start, arg_end,
                             &cmd_str, &cmd_len);
                if (r < 2)
                    continue;
                if (*d1_level == 0)
                {
                    static char absynName[64];
                    data1_absyn *absyn;

                    if (cmd_len > 63)
                        cmd_len = 63;
                    memcpy (absynName, cmd_str, cmd_len);
                    absynName[cmd_len] = '\0';

#if REGX_DEBUG
                    logf (LOG_DEBUG, "begin record %s", absynName);
#endif
                    if (!(absyn = data1_get_absyn (absynName)))
                        logf (LOG_WARN, "Unknown tagset: %s", absynName);
                    else
                    {
                        data1_node *res;

                        res = data1_mk_node (spec->m);
                        res->which = DATA1N_root;
                        res->u.root.type = absynName;
                        res->u.root.absyn = absyn;
                        res->root = res;
                        
                        d1_stack[*d1_level] = res;
                        d1_stack[++(*d1_level)] = NULL;
                    }
                }
                r = execTok (spec, &s, arg_no, arg_start, arg_end,
                             &cmd_str, &cmd_len);
            }
            else if (!strcmp (p, "element"))
            {
                r = execTok (spec, &s, arg_no, arg_start, arg_end,
                             &cmd_str, &cmd_len);
                if (r < 2)
                    continue;
                tagBegin (spec, d1_stack, d1_level, cmd_str, cmd_len);
                r = execTok (spec, &s, arg_no, arg_start, arg_end,
                             &cmd_str, &cmd_len);
            }
        }
        else if (!strcmp (p, "end"))
        {
            r = execTok (spec, &s, arg_no, arg_start, arg_end,
                         &cmd_str, &cmd_len);
            if (r > 1)
            {
                p = regxStrz (cmd_str, cmd_len);
                if (!strcmp (p, "record"))
                {
                    *d1_level = 0;
                    r = execTok (spec, &s, arg_no, arg_start, arg_end,
                                 &cmd_str, &cmd_len);
#if REGX_DEBUG
                    logf (LOG_DEBUG, "end record");
#endif
                    returnCode = 0;
                }
                else if (!strcmp (p, "element"))
                {
                    r = execTok (spec, &s, arg_no, arg_start, arg_end,
                                 &cmd_str, &cmd_len);
                    if (r > 2)
                    {
                        tagEnd (spec, d1_stack, d1_level, cmd_str, cmd_len);
                        r = execTok (spec, &s, arg_no, arg_start, arg_end,
                                     &cmd_str, &cmd_len);
                    }
                    else
                        tagEnd (spec, d1_stack, d1_level, NULL, 0);
                }
                else
                    logf (LOG_WARN, "missing record/element/variant");
            }
            else
                logf (LOG_WARN, "missing record/element/variant");
        }
        else if (!strcmp (p, "data"))
        {
            int textFlag = 0;
            int element_len;
            const char *element_str = NULL;
            
            while ((r = execTok (spec, &s, arg_no, arg_start, arg_end,
                                 &cmd_str, &cmd_len)) == 3)
            {
                if (cmd_len==5 && !memcmp ("-text", cmd_str, cmd_len))
                    textFlag = 1;
                else if (cmd_len==8 && !memcmp ("-element", cmd_str, cmd_len))
                {
                    r = execTok (spec, &s, arg_no, arg_start, arg_end,
                                 &element_str, &element_len);
                    if (r < 2)
                        break;
                }
                else 
                    logf (LOG_WARN, "bad data option: %.*s",
                          cmd_len, cmd_str);
            }
            if (r != 2)
            {
                logf (LOG_WARN, "missing data item after data");
                continue;
            }
            if (element_str)
                tagBegin (spec, d1_stack, d1_level, element_str, element_len);
            do
            {
                execData (spec, d1_stack, d1_level, cmd_str, cmd_len,
                          textFlag);
                r = execTok (spec, &s, arg_no, arg_start, arg_end,
                         &cmd_str, &cmd_len);
            } while (r > 1);
            if (element_str)
                tagEnd (spec, d1_stack, d1_level, NULL, 0);
        }
        else if (!strcmp (p, "unread"))
        {
            int no, offset;
            r = execTok (spec, &s, arg_no, arg_start, arg_end,
                         &cmd_str, &cmd_len);
            if (r==3 && cmd_len == 7 && !memcmp ("-offset", cmd_str, cmd_len))
            {
                r = execTok (spec, &s, arg_no, arg_start, arg_end,
                             &cmd_str, &cmd_len);
                if (r < 2)
                {
                    logf (LOG_WARN, "missing number after -offset");
                    continue;
                }
                p = regxStrz (cmd_str, cmd_len);
                offset = atoi (p);
                r = execTok (spec, &s, arg_no, arg_start, arg_end,
                             &cmd_str, &cmd_len);
            }
            else
                offset = 0;
            if (r < 2)
            {
                logf (LOG_WARN, "missing index after unread command");
                continue;
            }
            if (cmd_len != 1 || *cmd_str < '0' || *cmd_str > '9')
            {
                logf (LOG_WARN, "bad index after unread command");
                continue;
            }
            else
            {
                no = *cmd_str - '0';
                if (no >= arg_no)
                    no = arg_no - 1;
                *pptr = arg_start[no] + offset;
            }
            r = execTok (spec, &s, arg_no, arg_start, arg_end,
                         &cmd_str, &cmd_len);
        }
        else
        {
            logf (LOG_WARN, "unknown code command: %.*s", cmd_len, cmd_str);
            r = execTok (spec, &s, arg_no, arg_start, arg_end,
                         &cmd_str, &cmd_len);
            continue;
        }
        if (r > 1)
        {
            logf (LOG_WARN, "ignoring token %.*s", cmd_len, cmd_str);
            do {
                r = execTok (spec, &s, arg_no, arg_start, arg_end, &cmd_str,
                             &cmd_len);
            } while (r > 1);
        }
    }
    return returnCode;
}


static int execAction (struct lexSpec *spec, struct lexRuleAction *ap,
                       data1_node **d1_stack, int *d1_level,
                       int start_ptr, int *pptr)
{
    int sptr;
    int arg_start[20];
    int arg_end[20];
    int arg_no = 1;

    arg_start[0] = start_ptr;
    arg_end[0] = *pptr;

    while (ap)
    {
        switch (ap->which)
        {
        case REGX_PATTERN:
            if (ap->u.pattern.body)
            {
                arg_start[arg_no] = *pptr;
                if (!tryMatch (spec, pptr, &sptr, ap->u.pattern.dfa))
                {
                    arg_end[arg_no] = F_WIN_EOF;
                    arg_no++;
                    arg_start[arg_no] = F_WIN_EOF;
                    arg_end[arg_no] = F_WIN_EOF;
/* return 1*/
                }
                else
                {
                    arg_end[arg_no] = sptr;
                    arg_no++;
                    arg_start[arg_no] = sptr;
                    arg_end[arg_no] = *pptr;
                }
            }
            else
            {
                arg_start[arg_no] = *pptr;
                if (!tryMatch (spec, pptr, &sptr, ap->u.pattern.dfa))
                    return 1;
                if (sptr != arg_start[arg_no])
                    return 1;
                arg_end[arg_no] = *pptr;
            }
            arg_no++;
            break;
        case REGX_CODE:
            if (!execCode (spec, arg_no, arg_start, arg_end, pptr,
                           ap->u.code, d1_stack, d1_level))
                return 0;
            break;
        case REGX_END:
            arg_start[arg_no] = *pptr;
#if F_WIN_READ
            arg_end[arg_no] = F_WIN_EOF;
#else
            arg_end[arg_no] = spec->scan_size;
#endif
            arg_no++;
#if F_WIN_READ
            *pptr = F_WIN_EOF;
#else
            *pptr = spec->scan_size;
#endif
        }
        ap = ap->next;
    }
    return 1;
}

static int execRule (struct lexSpec *spec, struct lexTrans *trans,
                     data1_node **d1_stack, int *d1_level,
                     int ruleNo, int start_ptr, int *pptr)
{
#if REGX_DEBUG
    logf (LOG_DEBUG, "execRule %d", ruleNo);
#endif
    return execAction (spec, trans->fastRule[ruleNo]->actionList,
                       d1_stack, d1_level, start_ptr, pptr);
}

data1_node *lexNode (struct lexSpec *spec, struct lexTrans *trans,
                     data1_node **d1_stack, int *d1_level,
                     int *ptr)
{
    struct DFA_state *state = trans->dfa->states[0];
    struct DFA_tran *t;
    unsigned char c;
#if F_WIN_READ
    unsigned char c_prev = '\n';
#endif
    int i;
    int last_rule = 0;
    int last_ptr = *ptr;
    int start_ptr = *ptr;
    int skip_ptr = *ptr;

    while (1)
    {
#if F_WIN_READ
        c = f_win_advance (spec, ptr);
        if (*ptr == F_WIN_EOF)
        {
            if (last_rule)
            {
                if (skip_ptr < start_ptr)
                {
                    int size;
                    char *buf;
                    buf = f_win_get (spec, skip_ptr, start_ptr, &size);
                    execDataP (spec, d1_stack, d1_level, buf, size, 0);
                }
                *ptr = last_ptr;
                if (!execRule (spec, trans, d1_stack, d1_level, last_rule,
                               start_ptr, ptr))
                    break;
                skip_ptr = *ptr;
                last_rule = 0;
            }
            else if (skip_ptr < *ptr)
            {
                int size;
                char *buf;
                buf = f_win_get (spec, skip_ptr, *ptr, &size);
                execDataP (spec, d1_stack, d1_level, buf, size, 0);
            }
            if (*ptr == F_WIN_EOF)
                break;
        }
#else
        if (*ptr == spec->scan_size)
        {
            if (last_rule)
            {
                if (skip_ptr < start_ptr)
                {
                    execDataP (spec, d1_stack, d1_level,
                              spec->scan_buf + skip_ptr, start_ptr - skip_ptr,
                              0);
                }
                *ptr = last_ptr;
                execRule (spec, trans, d1_stack, d1_level, last_rule,
                          start_ptr, ptr);
                skip_ptr = *ptr;
                last_rule = 0;
            }
            else if (skip_ptr < *ptr)
            {
                execDataP (spec, d1_stack, d1_level,
                          spec->scan_buf + skip_ptr, *ptr - skip_ptr, 0);
            }
            if (*ptr == spec->scan_size)
                break;
        }
        c = spec->scan_buf[(*ptr)++];
#endif
        t = state->trans;
        i = state->tran_no;
        while (1)
            if (--i < 0)
            {   /* no transition for character c ... */
                if (last_rule)
                {
                    if (skip_ptr < start_ptr)
                    {
#if F_WIN_READ
                        int size;
                        char *buf;
                        buf = f_win_get (spec, skip_ptr, start_ptr, &size);
                        execDataP (spec, d1_stack, d1_level, buf, size, 0);
#else                        
                        execDataP (spec, d1_stack, d1_level,
                                  spec->scan_buf + skip_ptr,
                                  start_ptr - skip_ptr, 0);
#endif
                    }
                    *ptr = last_ptr;
                    if (!execRule (spec, trans, d1_stack, d1_level, last_rule,
                                   start_ptr, ptr))
                    {
                        if (spec->f_win_ef && *ptr != F_WIN_EOF)
                            (*spec->f_win_ef)(spec->f_win_fh, *ptr);
                        return NULL;
                    }
                    skip_ptr = *ptr;
                    last_rule = 0;
                    start_ptr = *ptr;
#if F_WIN_READ
                    if (start_ptr > 0)
                    {
                        --start_ptr;
                        c_prev = f_win_advance (spec, &start_ptr);
                    }
#endif
                }
                else
                {
#if F_WIN_READ
                    c_prev = f_win_advance (spec, &start_ptr);
                    *ptr = start_ptr;
#else
                    *ptr = ++start_ptr;
#endif
                }
                state = trans->dfa->states[0];
                break;
            }
            else if (c >= t->ch[0] && c <= t->ch[1])
            {   /* transition ... */
                state = trans->dfa->states[t->to];
                if (state->rule_no)
                {
#if F_WIN_READ
                    if (c_prev == '\n')
                    {
                        last_rule = state->rule_no;
                        last_ptr = *ptr;
                    } 
                    else if (state->rule_nno)
                    {
                        last_rule = state->rule_nno;
                        last_ptr = *ptr;
                    }
#else
                    if (!start_ptr || spec->scan_buf[start_ptr-1] == '\n')
                    {
                        last_rule = state->rule_no;
                        last_ptr = *ptr;
                    } 
                    else if (state->rule_nno)
                    {
                        last_rule = state->rule_nno;
                        last_ptr = *ptr;
                    }
#endif
                }
                break;
            }
            else
                t++;
    }
    return NULL;
}

static data1_node *lexRoot (struct lexSpec *spec, off_t offset)
{
    data1_node *d1_stack[512];
    int d1_level = 0;
    int ptr = offset;

    d1_stack[d1_level] = NULL;
    if (spec->beginActionList)
        execAction (spec, spec->beginActionList,
                    d1_stack, &d1_level, 0, &ptr);
    lexNode (spec, &spec->trans, d1_stack, &d1_level, &ptr);
    if (spec->endActionList)
        execAction (spec, spec->endActionList,
                    d1_stack, &d1_level, ptr, &ptr);
    return *d1_stack;
}

data1_node *grs_read_regx (struct grs_read_info *p)
/*
                             int (*rf)(void *, char *, size_t),
                             off_t (*sf)(void *, off_t),
                             void (*ef)(void *, off_t),
                             void *fh,
                             off_t offset,
                             const char *name, NMEM m
*/
{
    int res;
#if !F_WIN_READ
    static int size;
    int rd = 0;
#endif
    data1_node *n;

#if REGX_DEBUG
    logf (LOG_DEBUG, "data1_read_regx, offset=%ld type=%s",(long) offset,
          name);
#endif
    if (!curLexSpec || strcmp (curLexSpec->name, p->type))
    {
        if (curLexSpec)
            lexSpecDel (&curLexSpec);
        curLexSpec = lexSpecMk (p->type);
        res = readFileSpec (curLexSpec);
        if (res)
        {
            lexSpecDel (&curLexSpec);
            return NULL;
        }
    }
#if F_WIN_READ
    if (!p->offset)
    {
        curLexSpec->f_win_start = 0;
        curLexSpec->f_win_end = 0;
        curLexSpec->f_win_rf = p->readf;
        curLexSpec->f_win_sf = p->seekf;
        curLexSpec->f_win_fh = p->fh;
        curLexSpec->f_win_ef = p->endf;
        curLexSpec->f_win_size = 500000;
    }
#else
    if (!(curLexSpec->scan_buf = xmalloc (size = 4096)))
	abort();
    do
    {
	if (rd+4096 > size && !(curLexSpec->scan_buf
                                = xrealloc (curLexSpec->scan_buf, size *= 2)))
	    abort();
	if ((res = (*rf)(fh, curLexSpec->scan_buf + rd, 4096)) < 0)
            return NULL;
	rd += res;
    } while (res); 
    curLexSpec->scan_size = rd;
#endif
    curLexSpec->m = p->mem;
    n = lexRoot (curLexSpec, p->offset);
#if !F_WIN_READ
    xfree (curLexSpec->scan_buf);
#endif
    return n;
}
