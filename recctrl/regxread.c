/*
 * Copyright (C) 1994-2001, Index Data
 * All rights reserved.
 *
 * $Log: regxread.c,v $
 * Revision 1.37  2001-05-29 08:51:59  adam
 * More fixes for character encodings.
 *
 * Revision 1.36  2001/05/22 21:02:26  adam
 * Fixes for Tcl UTF8 character handling.
 *
 * Revision 1.35  2001/03/29 21:31:31  adam
 * Fixed "record begin" for Tcl filter.
 *
 * Revision 1.34  2000/11/29 14:24:01  adam
 * Script configure uses yaz pthreads options. Added locking for
 * zebra_register_{lock,unlock}.
 *
 * Revision 1.33  1999/11/30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.32  1999/09/07 07:19:21  adam
 * Work on character mapping. Implemented replace rules.
 *
 * Revision 1.31  1999/07/14 13:05:29  adam
 * Tcl filter works with objects when TCL is version 8 or later; filter
 * works with strings otherwise (slow).
 *
 * Revision 1.30  1999/07/14 10:55:28  adam
 * Fixed memory leak.
 *
 * Revision 1.29  1999/07/12 07:27:54  adam
 * Improved speed of Tcl processing. Fixed one memory leak.
 *
 * Revision 1.28  1999/07/06 12:26:04  adam
 * Fixed filters so that MS-DOS CR is ignored.
 *
 * Revision 1.27  1999/06/28 13:25:40  quinn
 * Improved diagnostics for Tcl
 *
 * Revision 1.26  1999/05/26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.25  1999/05/25 12:33:32  adam
 * Fixed bug in Tcl filter.
 *
 * Revision 1.24  1999/05/21 11:08:46  adam
 * Tcl filter attempts to read <filt>.tflt. Improvements to configure
 * script so that it reads uninstalled Tcl source.
 *
 * Revision 1.23  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.22  1998/11/03 16:07:13  adam
 * Yet another fix.
 *
 * Revision 1.21  1998/11/03 15:43:39  adam
 * Fixed bug introduced by previous commit.
 *
 * Revision 1.20  1998/11/03 14:51:28  adam
 * Changed code so that it creates as few data1 nodes as possible.
 *
 * Revision 1.19  1998/11/03 10:22:39  adam
 * Fixed memory leak that could occur for when large data1 node were
 * concatenated. Data-type data1_nodes may have multiple nodes.
 *
 * Revision 1.18  1998/10/15 13:11:47  adam
 * Added support for option -record for "end element". When specified
 * end element will mark end-of-record when at outer-level.
 *
 * Revision 1.17  1998/07/01 10:13:51  adam
 * Minor fix.
 *
 * Revision 1.16  1998/06/30 15:15:09  adam
 * Tags are trimmed: white space removed before- and after the tag.
 *
 * Revision 1.15  1998/06/30 12:55:45  adam
 * Bug fix.
 *
 * Revision 1.14  1998/03/05 08:41:00  adam
 * Implemented rule contexts.
 *
 * Revision 1.13  1997/12/12 06:33:58  adam
 * Fixed bug that showed up when multiple filter where used.
 * Made one routine thread-safe.
 *
 * Revision 1.12  1997/11/18 10:03:24  adam
 * Member num_children removed from data1_node.
 *
 * Revision 1.11  1997/11/06 11:41:01  adam
 * Implemented "begin variant" for the sgml.regx filter.
 *
 * Revision 1.10  1997/10/31 12:36:12  adam
 * Minor change that avoids compiler warning.
 *
 * Revision 1.9  1997/09/29 09:02:49  adam
 * Fixed small bug (introduced by previous commit).
 *
 * Revision 1.8  1997/09/17 12:19:22  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.7  1997/07/15 16:33:07  adam
 * Check for zero length in execData.
 *
 * Revision 1.6  1997/02/24 10:41:51  adam
 * Cleanup of code and commented out the "end element-end-record" code.
 *
 * Revision 1.5  1997/02/19 16:22:33  adam
 * Fixed "end element" to terminate record in outer-most level.
 *
 * Revision 1.4  1997/02/12 20:42:58  adam
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
#include <ctype.h>

#include <yaz/tpath.h>
#include <zebrautl.h>
#include <dfa.h>
#include "grsread.h"

#if HAVE_TCL_H
#include <tcl.h>

#if MAJOR_VERSION >= 8
#define HAVE_TCL_OBJECTS
#endif
#endif

#define REGX_DEBUG 0

#define F_WIN_EOF 2000000000
#define F_WIN_READ 1

#define REGX_EOF     0
#define REGX_PATTERN 1
#define REGX_BODY    2
#define REGX_BEGIN   3
#define REGX_END     4
#define REGX_CODE    5
#define REGX_CONTEXT 6
#define REGX_INIT    7

struct regxCode {
    char *str;
#if HAVE_TCL_OBJECTS
    Tcl_Obj *tcl_obj;
#endif
};

struct lexRuleAction {
    int which; 
    union {
        struct {
            struct DFA *dfa;    /* REGX_PATTERN */
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

struct lexContext {
    char *name;
    struct DFA *dfa;
    struct lexRule *rules;
    struct lexRuleInfo **fastRule;
    int ruleNo;
    int initFlag;

    struct lexRuleAction *beginActionList;
    struct lexRuleAction *endActionList;
    struct lexRuleAction *initActionList;
    struct lexContext *next;
};

struct lexConcatBuf {
    int max;
    char *buf;
};

struct lexSpec {
    char *name;
    struct lexContext *context;

    struct lexContext **context_stack;
    int context_stack_size;
    int context_stack_top;

    int lineNo;
    NMEM m;
    data1_handle dh;
#if HAVE_TCL_H
    Tcl_Interp *tcl_interp;
#endif
    void *f_win_fh;
    void (*f_win_ef)(void *, off_t);

    int f_win_start;      /* first byte of buffer is this file offset */
    int f_win_end;        /* last byte of buffer is this offset - 1 */
    int f_win_size;       /* size of buffer */
    char *f_win_buf;      /* buffer itself */
    int (*f_win_rf)(void *, char *, size_t);
    off_t (*f_win_sf)(void *, off_t);

    struct lexConcatBuf *concatBuf;
    int maxLevel;
    data1_node **d1_stack;
    int d1_level;
    int stop_flag;
    
    int *arg_start;
    int *arg_end;
    int arg_no;
    int ptr;
};

struct lexSpecs {
    struct lexSpec *spec;
};

static char *f_win_get (struct lexSpec *spec, off_t start_pos, off_t end_pos,
                        int *size)
{
    int i, r, off = start_pos - spec->f_win_start;

    if (off >= 0 && end_pos <= spec->f_win_end)
    {
        *size = end_pos - start_pos;
        return spec->f_win_buf + off;
    }
    if (off < 0 || start_pos >= spec->f_win_end)
    {
        (*spec->f_win_sf)(spec->f_win_fh, start_pos);
        spec->f_win_start = start_pos;

        if (!spec->f_win_buf)
            spec->f_win_buf = (char *) xmalloc (spec->f_win_size);
        *size = (*spec->f_win_rf)(spec->f_win_fh, spec->f_win_buf,
                                  spec->f_win_size);
        spec->f_win_end = spec->f_win_start + *size;

        if (*size > end_pos - start_pos)
            *size = end_pos - start_pos;
        return spec->f_win_buf;
    }
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

static void regxCodeDel (struct regxCode **pp)
{
    struct regxCode *p = *pp;
    if (p)
    {
#if HAVE_TCL_OBJECTS
	if (p->tcl_obj)
	    Tcl_DecrRefCount (p->tcl_obj);
#endif
        xfree (p->str); 
        xfree (p);
        *pp = NULL;
    }
}

static void regxCodeMk (struct regxCode **pp, const char *buf, int len)
{
    struct regxCode *p;

    p = (struct regxCode *) xmalloc (sizeof(*p));
    p->str = (char *) xmalloc (len+1);
    memcpy (p->str, buf, len);
    p->str[len] = '\0';
#if HAVE_TCL_OBJECTS
    p->tcl_obj = Tcl_NewStringObj ((char *) buf, len);
    if (p->tcl_obj)
	Tcl_IncrRefCount (p->tcl_obj);
#endif
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

static struct lexContext *lexContextCreate (const char *name)
{
    struct lexContext *p = (struct lexContext *) xmalloc (sizeof(*p));

    p->name = xstrdup (name);
    p->ruleNo = 1;
    p->initFlag = 0;
    p->dfa = lexSpecDFA ();
    p->rules = NULL;
    p->fastRule = NULL;
    p->beginActionList = NULL;
    p->endActionList = NULL;
    p->initActionList = NULL;
    p->next = NULL;
    return p;
}

static void lexContextDestroy (struct lexContext *p)
{
    struct lexRule *rp, *rp1;

    dfa_delete (&p->dfa);
    xfree (p->fastRule);
    for (rp = p->rules; rp; rp = rp1)
    {
	rp1 = rp->next;
        actionListDel (&rp->info.actionList);
        xfree (rp);
    }
    actionListDel (&p->beginActionList);
    actionListDel (&p->endActionList);
    actionListDel (&p->initActionList);
    xfree (p->name);
    xfree (p);
}

static struct lexSpec *lexSpecCreate (const char *name, data1_handle dh)
{
    struct lexSpec *p;
    int i;
    
    p = (struct lexSpec *) xmalloc (sizeof(*p));
    p->name = (char *) xmalloc (strlen(name)+1);
    strcpy (p->name, name);

#if HAVE_TCL_H
    p->tcl_interp = 0;
#endif
    p->dh = dh;
    p->context = NULL;
    p->context_stack_size = 100;
    p->context_stack = (struct lexContext **)
	xmalloc (sizeof(*p->context_stack) * p->context_stack_size);
    p->f_win_buf = NULL;

    p->maxLevel = 128;
    p->concatBuf = (struct lexConcatBuf *)
	xmalloc (sizeof(*p->concatBuf) * p->maxLevel);
    for (i = 0; i < p->maxLevel; i++)
    {
	p->concatBuf[i].max = 0;
	p->concatBuf[i].buf = 0;
    }
    p->d1_stack = (data1_node **) xmalloc (sizeof(*p->d1_stack) * p->maxLevel);
    p->d1_level = 0;
    return p;
}

static void lexSpecDestroy (struct lexSpec **pp)
{
    struct lexSpec *p;
    struct lexContext *lt;
    int i;

    assert (pp);
    p = *pp;
    if (!p)
        return ;

    for (i = 0; i < p->maxLevel; i++)
	xfree (p->concatBuf[i].buf);
    xfree (p->concatBuf);

    lt = p->context;
    while (lt)
    {
	struct lexContext *lt_next = lt->next;
	lexContextDestroy (lt);
	lt = lt_next;
    }
#if HAVE_TCL_OBJECTS
    if (p->tcl_interp)
	Tcl_DeleteInterp (p->tcl_interp);
#endif
    xfree (p->name);
    xfree (p->f_win_buf);
    xfree (p->context_stack);
    xfree (p->d1_stack);
    xfree (p);
    *pp = NULL;
}

static int readParseToken (const char **cpp, int *len)
{
    const char *cp = *cpp;
    char cmd[32];
    int i, level;

    while (*cp == ' ' || *cp == '\t' || *cp == '\n' || *cp == '\r')
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
            if (i < (int) sizeof(cmd)-2)
		i++;
            cp++;
        }
        cmd[i] = '\0';
        if (i == 0)
        {
            logf (LOG_WARN, "bad character %d %c", *cp, *cp);
            cp++;
            while (*cp && *cp != ' ' && *cp != '\t' &&
                   *cp != '\n' && *cp != '\r')
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
	else if (!strcmp (cmd, "context"))
	    return REGX_CONTEXT;
	else if (!strcmp (cmd, "init"))
	    return REGX_INIT;
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
    const char *s0;

    while ((tok = readParseToken (&s, &len)))
    {
        switch (tok)
        {
        case REGX_BODY:
            bodyMark = 1;
            continue;
        case REGX_CODE:
            *ap = (struct lexRuleAction *) xmalloc (sizeof(**ap));
            (*ap)->which = tok;
            regxCodeMk (&(*ap)->u.code, s, len);
            s += len+1;
            break;
        case REGX_PATTERN:
            *ap = (struct lexRuleAction *) xmalloc (sizeof(**ap));
            (*ap)->which = tok;
            (*ap)->u.pattern.body = bodyMark;
            bodyMark = 0;
            (*ap)->u.pattern.dfa = lexSpecDFA ();
	    s0 = s;
            r = dfa_parse ((*ap)->u.pattern.dfa, &s);
            if (r || *s != '/')
            {
                xfree (*ap);
                *ap = NULL;
                logf (LOG_WARN, "regular expression error '%.*s'", s-s0, s0);
                return -1;
            }
            dfa_mkstate ((*ap)->u.pattern.dfa);
            s++;
            break;
        case REGX_BEGIN:
            logf (LOG_WARN, "cannot use BEGIN here");
            continue;
        case REGX_INIT:
            logf (LOG_WARN, "cannot use INIT here");
            continue;
        case REGX_END:
            *ap = (struct lexRuleAction *) xmalloc (sizeof(**ap));
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
    int len, r, tok;
    struct lexRule *rp;
    struct lexContext *lc;

    tok = readParseToken (&s, &len);
    if (tok == REGX_CONTEXT)
    {
	char context_name[32];
	tok = readParseToken (&s, &len);
	if (tok != REGX_CODE)
	{
	    logf (LOG_WARN, "missing name after CONTEXT keyword");
	    return 0;
	}
	if (len > 31)
	    len = 31;
	memcpy (context_name, s, len);
	context_name[len] = '\0';
	lc = lexContextCreate (context_name);
	lc->next = spec->context;
	spec->context = lc;
	return 0;
    }
    if (!spec->context)
	spec->context = lexContextCreate ("main");
       
    switch (tok)
    {
    case REGX_BEGIN:
        actionListDel (&spec->context->beginActionList);
        actionListMk (spec, s, &spec->context->beginActionList);
	break;
    case REGX_END:
        actionListDel (&spec->context->endActionList);
        actionListMk (spec, s, &spec->context->endActionList);
	break;
    case REGX_INIT:
        actionListDel (&spec->context->initActionList);
        actionListMk (spec, s, &spec->context->initActionList);
	break;
    case REGX_PATTERN:
#if REGX_DEBUG
	logf (LOG_LOG, "rule %d %s", spec->context->ruleNo, s);
#endif
        r = dfa_parse (spec->context->dfa, &s);
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
        rp = (struct lexRule *) xmalloc (sizeof(*rp));
        rp->info.no = spec->context->ruleNo++;
        rp->next = spec->context->rules;
        spec->context->rules = rp;
        actionListMk (spec, s, &rp->info.actionList);
    }
    return 0;
}

int readFileSpec (struct lexSpec *spec)
{
    struct lexContext *lc;
    int c, i, errors = 0;
    FILE *spec_inf = 0;
    WRBUF lineBuf;
    char fname[256];

#if HAVE_TCL_H
    if (spec->tcl_interp)
    {
	sprintf (fname, "%s.tflt", spec->name);
	spec_inf = yaz_path_fopen (data1_get_tabpath(spec->dh), fname, "r");
    }
#endif
    if (!spec_inf)
    {
	sprintf (fname, "%s.flt", spec->name);
	spec_inf = yaz_path_fopen (data1_get_tabpath(spec->dh), fname, "r");
    }
    if (!spec_inf)
    {
        logf (LOG_ERRNO|LOG_WARN, "cannot read spec file %s", spec->name);
        return -1;
    }
    logf (LOG_LOG, "reading regx filter %s", fname);
#if HAVE_TCL_H
    if (spec->tcl_interp)
	logf (LOG_LOG, "Tcl enabled");
#endif
    lineBuf = wrbuf_alloc();
    spec->lineNo = 0;
    c = getc (spec_inf);
    while (c != EOF)
    {
	wrbuf_rewind (lineBuf);
        if (c == '#' || c == '\n' || c == ' ' || c == '\t' || c == '\r')
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
	    
            while (1)
            {
                int c1 = c;
		wrbuf_putc(lineBuf, c);
                c = getc (spec_inf);
		while (c == '\r')
		    c = getc (spec_inf);
                if (c == EOF)
                    break;
                if (c1 == '\n')
                {
                    if (c != ' ' && c != '\t')
                        break;
                    addLine++;
                }
            }
	    wrbuf_putc(lineBuf, '\0');
            readOneSpec (spec, wrbuf_buf(lineBuf));
            spec->lineNo += addLine;
        }
    }
    fclose (spec_inf);
    wrbuf_free(lineBuf, 1);

#if 0
    debug_dfa_trav = 1;
    debug_dfa_tran = 1;
    debug_dfa_followpos = 1;
    dfa_verbose = 1;
#endif
    for (lc = spec->context; lc; lc = lc->next)
    {
	struct lexRule *rp;
	lc->fastRule = (struct lexRuleInfo **)
	    xmalloc (sizeof(*lc->fastRule) * lc->ruleNo);
	for (i = 0; i < lc->ruleNo; i++)
	    lc->fastRule[i] = NULL;
	for (rp = lc->rules; rp; rp = rp->next)
	    lc->fastRule[rp->info.no] = &rp->info;
	dfa_mkstate (lc->dfa);
    }
    if (errors)
        return -1;
    
    return 0;
}

#if 0
static struct lexSpec *curLexSpec = NULL;
#endif

static void execData (struct lexSpec *spec,
                      const char *ebuf, int elen, int formatted_text)
{
    struct data1_node *res, *parent;
    int org_len;

    if (elen == 0) /* shouldn't happen, but it does! */
	return ;
#if REGX_DEBUG
    if (elen > 40)
        logf (LOG_LOG, "data (%d bytes) %.15s ... %.*s", elen,
	      ebuf, 15, ebuf + elen-15);
    else if (elen > 0)
        logf (LOG_LOG, "data (%d bytes) %.*s", elen, elen, ebuf);
    else 
        logf (LOG_LOG, "data (%d bytes)", elen);
#endif
        
    if (spec->d1_level <= 1)
        return;

    parent = spec->d1_stack[spec->d1_level -1];
    assert (parent);

    if ((res = spec->d1_stack[spec->d1_level]) && res->which == DATA1N_data)
	org_len = res->u.data.len;
    else
    {
	org_len = 0;

	res = data1_mk_node (spec->dh, spec->m);
	res->parent = parent;
	res->which = DATA1N_data;
	res->u.data.what = DATA1I_text;
	res->u.data.len = 0;
	res->u.data.formatted_text = formatted_text;
#if 0
	if (elen > DATA1_LOCALDATA)
	    res->u.data.data = nmem_malloc (spec->m, elen);
	else
	    res->u.data.data = res->lbuf;
	memcpy (res->u.data.data, ebuf, elen);
#else
	res->u.data.data = 0;
#endif
	res->root = parent->root;
	
	parent->last_child = res;
	if (spec->d1_stack[spec->d1_level])
	    spec->d1_stack[spec->d1_level]->next = res;
	else
	    parent->child = res;
	spec->d1_stack[spec->d1_level] = res;
    }
    if (org_len + elen >= spec->concatBuf[spec->d1_level].max)
    {
	char *old_buf, *new_buf;

	spec->concatBuf[spec->d1_level].max = org_len + elen + 256;
	new_buf = (char *) xmalloc (spec->concatBuf[spec->d1_level].max);
	if ((old_buf = spec->concatBuf[spec->d1_level].buf))
	{
	    memcpy (new_buf, old_buf, org_len);
	    xfree (old_buf);
	}
	spec->concatBuf[spec->d1_level].buf = new_buf;
    }
    memcpy (spec->concatBuf[spec->d1_level].buf + org_len, ebuf, elen);
    res->u.data.len += elen;
}

static void execDataP (struct lexSpec *spec,
                       const char *ebuf, int elen, int formatted_text)
{
    execData (spec, ebuf, elen, formatted_text);
}

static void tagDataRelease (struct lexSpec *spec)
{
    data1_node *res;
    
    if ((res = spec->d1_stack[spec->d1_level]) &&
	res->which == DATA1N_data && 
	res->u.data.what == DATA1I_text)
    {
	assert (!res->u.data.data);
	assert (res->u.data.len > 0);
	if (res->u.data.len > DATA1_LOCALDATA)
	    res->u.data.data = (char *) nmem_malloc (spec->m, res->u.data.len);
	else
	    res->u.data.data = res->lbuf;
	memcpy (res->u.data.data, spec->concatBuf[spec->d1_level].buf,
		res->u.data.len);
    }
}

static void variantBegin (struct lexSpec *spec, 
			  const char *class_str, int class_len,
			  const char *type_str, int type_len,
			  const char *value_str, int value_len)
{
    struct data1_node *parent = spec->d1_stack[spec->d1_level -1];
    char tclass[DATA1_MAX_SYMBOL], ttype[DATA1_MAX_SYMBOL];
    data1_vartype *tp;
    int i;
    data1_node *res;

    if (spec->d1_level == 0)
    {
        logf (LOG_WARN, "in variant begin. No record type defined");
        return ;
    }
    if (class_len >= DATA1_MAX_SYMBOL)
	class_len = DATA1_MAX_SYMBOL-1;
    memcpy (tclass, class_str, class_len);
    tclass[class_len] = '\0';

    if (type_len >= DATA1_MAX_SYMBOL)
	type_len = DATA1_MAX_SYMBOL-1;
    memcpy (ttype, type_str, type_len);
    ttype[type_len] = '\0';

#if REGX_DEBUG 
    logf (LOG_LOG, "variant begin %s %s (%d)", tclass, ttype,
	  spec->d1_level);
#endif

    if (!(tp =
	  data1_getvartypebyct(spec->dh, parent->root->u.root.absyn->varset,
			       tclass, ttype)))
	return;
    
    if (parent->which != DATA1N_variant)
    {
	res = data1_mk_node (spec->dh, spec->m);
	res->parent = parent;
	res->which = DATA1N_variant;
	res->u.variant.type = 0;
	res->u.variant.value = 0;
	res->root = parent->root;

	parent->last_child = res;
	if (spec->d1_stack[spec->d1_level])
	{
	    tagDataRelease (spec);
	    spec->d1_stack[spec->d1_level]->next = res;
	}
	else
	    parent->child = res;
	spec->d1_stack[spec->d1_level] = res;
	spec->d1_stack[++(spec->d1_level)] = NULL;
    }
    for (i = spec->d1_level-1; spec->d1_stack[i]->which == DATA1N_variant; i--)
	if (spec->d1_stack[i]->u.variant.type == tp)
	{
	    spec->d1_level = i;
	    break;
	}

#if REGX_DEBUG 
    logf (LOG_LOG, "variant node (%d)", spec->d1_level);
#endif
    parent = spec->d1_stack[spec->d1_level-1];
    res = data1_mk_node (spec->dh, spec->m);
    res->parent = parent;
    res->which = DATA1N_variant;
    res->root = parent->root;
    res->u.variant.type = tp;

    if (value_len >= DATA1_LOCALDATA)
	value_len =DATA1_LOCALDATA-1;
    memcpy (res->lbuf, value_str, value_len);
    res->lbuf[value_len] = '\0';

    res->u.variant.value = res->lbuf;
    
    parent->last_child = res;
    if (spec->d1_stack[spec->d1_level])
    {
	tagDataRelease (spec);
        spec->d1_stack[spec->d1_level]->next = res;
    }
    else
        parent->child = res;
    spec->d1_stack[spec->d1_level] = res;
    spec->d1_stack[++(spec->d1_level)] = NULL;
}

static void tagStrip (const char **tag, int *len)
{
    int i;

    for (i = *len; i > 0 && isspace((*tag)[i-1]); --i)
        ;
    *len = i;
    for (i = 0; i < *len && isspace((*tag)[i]); i++)
        ;
    *tag += i;
    *len -= i;
}

static void tagBegin (struct lexSpec *spec, 
                      const char *tag, int len)
{
    struct data1_node *parent;
    data1_element *elem = NULL;
    data1_node *partag;
    data1_node *res;
    data1_element *e = NULL;
    int localtag = 0;

    if (spec->d1_level == 0)
    {
        logf (LOG_WARN, "in element begin. No record type defined");
        return ;
    }
    tagStrip (&tag, &len);

    parent = spec->d1_stack[spec->d1_level -1];
    partag = get_parent_tag(spec->dh, parent);
   
    res = data1_mk_node_type (spec->dh, spec->m, DATA1N_tag);
    res->parent = parent;

    if (len >= DATA1_LOCALDATA)
	res->u.tag.tag = (char *) nmem_malloc (spec->m, len+1);
    else
	res->u.tag.tag = res->lbuf;

    memcpy (res->u.tag.tag, tag, len);
    res->u.tag.tag[len] = '\0';
   
#if REGX_DEBUG 
    logf (LOG_LOG, "begin tag %s (%d)", res->u.tag.tag, spec->d1_level);
#endif
    if (parent->which == DATA1N_variant)
        return ;
    if (partag)
        if (!(e = partag->u.tag.element))
            localtag = 1;
    
    elem = data1_getelementbytagname (spec->dh,
				      spec->d1_stack[0]->u.root.absyn,
				      e, res->u.tag.tag);
    res->u.tag.element = elem;
    res->root = parent->root;

    parent->last_child = res;
    if (spec->d1_stack[spec->d1_level])
    {
	tagDataRelease (spec);
        spec->d1_stack[spec->d1_level]->next = res;
    }
    else
        parent->child = res;
    spec->d1_stack[spec->d1_level] = res;
    spec->d1_stack[++(spec->d1_level)] = NULL;
}

static void tagEnd (struct lexSpec *spec, int min_level,
                    const char *tag, int len)
{
    tagStrip (&tag, &len);
    while (spec->d1_level > min_level)
    {
	tagDataRelease (spec);
        (spec->d1_level)--;
        if (spec->d1_level == 0)
	    break;
        if ((spec->d1_stack[spec->d1_level]->which == DATA1N_tag) &&
	    (!tag ||
	     (strlen(spec->d1_stack[spec->d1_level]->u.tag.tag) ==
	      (size_t) len &&
	      !memcmp (spec->d1_stack[spec->d1_level]->u.tag.tag, tag, len))))
            break;
    }
#if REGX_DEBUG
    logf (LOG_LOG, "end tag (%d)", spec->d1_level);
#endif
}


static int tryMatch (struct lexSpec *spec, int *pptr, int *mptr,
                     struct DFA *dfa)
{
    struct DFA_state *state = dfa->states[0];
    struct DFA_tran *t;
    unsigned char c;
    unsigned char c_prev = 0;
    int ptr = *pptr;          /* current pointer */
    int start_ptr = *pptr;    /* first char of match */
    int last_ptr = 0;         /* last char of match */
    int last_rule = 0;        /* rule number of current match */
    int i;

    while (1)
    {
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
                c_prev = c;
                break;
            }
            else if (c >= t->ch[0] && c <= t->ch[1])
            {
                state = dfa->states[t->to];
                if (state->rule_no)
                {
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
                }
                break;
            }
            else
                t++;
    }
    return 0;
}

static int execTok (struct lexSpec *spec, const char **src,
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
        if (spec->arg_no == 0)
        {
            *tokBuf = "";
            *tokLen = 0;
        }
        else
        {
            if (n >= spec->arg_no)
                n = spec->arg_no-1;
            *tokBuf = f_win_get (spec, spec->arg_start[n], spec->arg_end[n],
				 tokLen);
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
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r' &&
               *s != ';')
            s++;
        *tokLen = s - *tokBuf;
        *src = s;
        return 3;
    }
    else
    {
        *tokBuf = s++;
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r' &&
               *s != ';')
            s++;
        *tokLen = s - *tokBuf;
    }
    *src = s;
    return 2;
}

static char *regxStrz (const char *src, int len, char *str)
{
    if (len > 63)
        len = 63;
    memcpy (str, src, len);
    str[len] = '\0';
    return str;
}

#if HAVE_TCL_H
static int cmd_tcl_begin (ClientData clientData, Tcl_Interp *interp,
			  int argc, char **argv)
{
    struct lexSpec *spec = (struct lexSpec *) clientData;
    if (argc < 2)
	return TCL_ERROR;
    if (!strcmp(argv[1], "record") && argc == 3)
    {
	char *absynName = argv[2];
	data1_absyn *absyn;

#if REGX_DEBUG
	logf (LOG_LOG, "begin record %s", absynName);
#endif
	if (!(absyn = data1_get_absyn (spec->dh, absynName)))
	    logf (LOG_WARN, "Unknown tagset: %s", absynName);
	else
	{
	    data1_node *res;
	    
	    res = data1_mk_node (spec->dh, spec->m);
	    res->which = DATA1N_root;
	    res->u.root.type =
                data1_insert_string(spec->dh, res, spec->m, absynName);
	    res->u.root.absyn = absyn;
	    res->root = res;
	    
	    spec->d1_stack[spec->d1_level] = res;
	    spec->d1_stack[++(spec->d1_level)] = NULL;
	}
    }
    else if (!strcmp(argv[1], "element") && argc == 3)
    {
	tagBegin (spec, argv[2], strlen(argv[2]));
    }
    else if (!strcmp (argv[1], "variant") && argc == 5)
    {
	variantBegin (spec, argv[2], strlen(argv[2]),
		      argv[3], strlen(argv[3]),
		      argv[4], strlen(argv[4]));
    }
    else if (!strcmp (argv[1], "context") && argc == 3)
    {
	struct lexContext *lc = spec->context;
#if REGX_DEBUG
	logf (LOG_LOG, "begin context %s",argv[2]);
#endif
	while (lc && strcmp (argv[2], lc->name))
	    lc = lc->next;
	if (lc)
	{
	    spec->context_stack[++(spec->context_stack_top)] = lc;
	}
	else
	    logf (LOG_WARN, "unknown context %s", argv[2]);
    }
    else
	return TCL_ERROR;
    return TCL_OK;
}

static int cmd_tcl_end (ClientData clientData, Tcl_Interp *interp,
			int argc, char **argv)
{
    struct lexSpec *spec = (struct lexSpec *) clientData;
    if (argc < 2)
	return TCL_ERROR;
    
    if (!strcmp (argv[1], "record"))
    {
	while (spec->d1_level)
	{
	    tagDataRelease (spec);
	    (spec->d1_level)--;
	}
#if REGX_DEBUG
	logf (LOG_LOG, "end record");
#endif
	spec->stop_flag = 1;
    }
    else if (!strcmp (argv[1], "element"))
    {
	int min_level = 1;
	char *element = 0;
	if (argc >= 3 && !strcmp(argv[2], "-record"))
	{
	    min_level = 0;
	    if (argc == 4)
		element = argv[3];
	}
	else
	    if (argc == 3)
		element = argv[2];
	tagEnd (spec, min_level, element, (element ? strlen(element) : 0));
	if (spec->d1_level == 0)
	{
#if REGX_DEBUG
	    logf (LOG_LOG, "end element end records");
#endif
	    spec->stop_flag = 1;
	}
    }
    else if (!strcmp (argv[1], "context"))
    {
#if REGX_DEBUG
	logf (LOG_LOG, "end context");
#endif
	if (spec->context_stack_top)
	    (spec->context_stack_top)--;
    }
    else
	return TCL_ERROR;
    return TCL_OK;
}

static int cmd_tcl_data (ClientData clientData, Tcl_Interp *interp,
			 int argc, char **argv)
{
    int argi = 1;
    int textFlag = 0;
    const char *element = 0;
    struct lexSpec *spec = (struct lexSpec *) clientData;
    
    while (argi < argc)
    {
	if (!strcmp("-text", argv[argi]))
	{
	    textFlag = 1;
	    argi++;
	}
	else if (!strcmp("-element", argv[argi]))
	{
	    argi++;
	    if (argi < argc)
		element = argv[argi++];
	}
	else
	    break;
    }
    if (element)
	tagBegin (spec, element, strlen(element));

    while (argi < argc)
    {
#if TCL_MAJOR_VERSION > 8 || (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION > 0)
	Tcl_DString ds;
	char *native = Tcl_UtfToExternalDString(0, argv[argi], -1, &ds);
	execData (spec, native, strlen(native), textFlag);
	Tcl_DStringFree (&ds);
#else
	execData (spec, argv[argi], strlen(argv[argi]), textFlag);
#endif
	argi++;
    }
    if (element)
	tagEnd (spec, 1, NULL, 0);
    return TCL_OK;
}

static int cmd_tcl_unread (ClientData clientData, Tcl_Interp *interp,
			   int argc, char **argv)
{
    struct lexSpec *spec = (struct lexSpec *) clientData;
    int argi = 1;
    int offset = 0;
    int no;
    
    while (argi < argc)
    {
	if (!strcmp("-offset", argv[argi]))
	{
	    argi++;
	    if (argi < argc)
	    {
		offset = atoi(argv[argi]);
		argi++;
	    }
	}
	else
	    break;
    }
    if (argi != argc-1)
	return TCL_ERROR;
    no = atoi(argv[argi]);
    if (no >= spec->arg_no)
	no = spec->arg_no - 1;
    spec->ptr = spec->arg_start[no] + offset;
    return TCL_OK;
}

static void execTcl (struct lexSpec *spec, struct regxCode *code)
{   
    int i;
    int ret;
    for (i = 0; i < spec->arg_no; i++)
    {
	char var_name[10], *var_buf;
	int var_len, ch;
	
	sprintf (var_name, "%d", i);
	var_buf = f_win_get (spec, spec->arg_start[i], spec->arg_end[i],
			     &var_len);	
	if (var_buf)
	{
	    ch = var_buf[var_len];
	    var_buf[var_len] = '\0';
	    Tcl_SetVar (spec->tcl_interp, var_name, var_buf, 0);
	    var_buf[var_len] = ch;
	}
    }
#if HAVE_TCL_OBJECTS
    ret = Tcl_GlobalEvalObj(spec->tcl_interp, code->tcl_obj);
#else
    ret = Tcl_GlobalEval (spec->tcl_interp, code->str);
#endif
    if (ret != TCL_OK)
    {
    	const char *err = Tcl_GetVar(spec->tcl_interp, "errorInfo", 0);
	logf(LOG_FATAL, "Tcl error, line=%d, \"%s\"\n%s", 
	    spec->tcl_interp->errorLine,
	    spec->tcl_interp->result,
	    err ? err : "[NO ERRORINFO]");
    }
}
/* HAVE_TCL_H */
#endif

static void execCode (struct lexSpec *spec, struct regxCode *code)
{
    const char *s = code->str;
    int cmd_len, r;
    const char *cmd_str;
    
    r = execTok (spec, &s, &cmd_str, &cmd_len);
    while (r)
    {
        char *p, ptmp[64];
        
        if (r == 1)
        {
            r = execTok (spec, &s, &cmd_str, &cmd_len);
            continue;
        }
        p = regxStrz (cmd_str, cmd_len, ptmp);
        if (!strcmp (p, "begin"))
        {
            r = execTok (spec, &s, &cmd_str, &cmd_len);
            if (r < 2)
	    {
		logf (LOG_WARN, "missing keyword after 'begin'");
                continue;
	    }
            p = regxStrz (cmd_str, cmd_len, ptmp);
            if (!strcmp (p, "record"))
            {
                r = execTok (spec, &s, &cmd_str, &cmd_len);
                if (r < 2)
                    continue;
                if (spec->d1_level == 0)
                {
                    static char absynName[64];
                    data1_absyn *absyn;

                    if (cmd_len > 63)
                        cmd_len = 63;
                    memcpy (absynName, cmd_str, cmd_len);
                    absynName[cmd_len] = '\0';

#if REGX_DEBUG
                    logf (LOG_LOG, "begin record %s", absynName);
#endif
                    if (!(absyn = data1_get_absyn (spec->dh, absynName)))
                        logf (LOG_WARN, "Unknown tagset: %s", absynName);
                    else
                    {
                        data1_node *res;

                        res = data1_mk_node (spec->dh, spec->m);
                        res->which = DATA1N_root;
                        res->u.root.type = absynName;
                        res->u.root.absyn = absyn;
                        res->root = res;
                        
                        spec->d1_stack[spec->d1_level] = res;
                        spec->d1_stack[++(spec->d1_level)] = NULL;
                    }
                }
                r = execTok (spec, &s, &cmd_str, &cmd_len);
            }
            else if (!strcmp (p, "element"))
            {
                r = execTok (spec, &s, &cmd_str, &cmd_len);
                if (r < 2)
                    continue;
                tagBegin (spec, cmd_str, cmd_len);
                r = execTok (spec, &s, &cmd_str, &cmd_len);
            } 
	    else if (!strcmp (p, "variant"))
	    {
		int class_len;
		const char *class_str = NULL;
		int type_len;
		const char *type_str = NULL;
		int value_len;
		const char *value_str = NULL;
		r = execTok (spec, &s, &cmd_str, &cmd_len);
		if (r < 2)
		    continue;
		class_str = cmd_str;
		class_len = cmd_len;
		r = execTok (spec, &s, &cmd_str, &cmd_len);
		if (r < 2)
		    continue;
		type_str = cmd_str;
		type_len = cmd_len;

		r = execTok (spec, &s, &cmd_str, &cmd_len);
		if (r < 2)
		    continue;
		value_str = cmd_str;
		value_len = cmd_len;

                variantBegin (spec, class_str, class_len,
			      type_str, type_len, value_str, value_len);
		
		
		r = execTok (spec, &s, &cmd_str, &cmd_len);
	    }
	    else if (!strcmp (p, "context"))
	    {
		if (r > 1)
		{
		    struct lexContext *lc = spec->context;
		    r = execTok (spec, &s, &cmd_str, &cmd_len);
		    p = regxStrz (cmd_str, cmd_len, ptmp);
#if REGX_DEBUG
		    logf (LOG_LOG, "begin context %s", p);
#endif
		    while (lc && strcmp (p, lc->name))
			lc = lc->next;
		    if (lc)
			spec->context_stack[++(spec->context_stack_top)] = lc;
		    else
			logf (LOG_WARN, "unknown context %s", p);
		    
		}
		r = execTok (spec, &s, &cmd_str, &cmd_len);
	    }
	    else
	    {
		logf (LOG_WARN, "bad keyword '%s' after begin", p);
	    }
        }
        else if (!strcmp (p, "end"))
        {
            r = execTok (spec, &s, &cmd_str, &cmd_len);
            if (r < 2)
	    {
		logf (LOG_WARN, "missing keyword after 'end'");
		continue;
	    }
	    p = regxStrz (cmd_str, cmd_len, ptmp);
	    if (!strcmp (p, "record"))
	    {
		while (spec->d1_level)
		{
		    tagDataRelease (spec);
		    (spec->d1_level)--;
		}
		r = execTok (spec, &s, &cmd_str, &cmd_len);
#if REGX_DEBUG
		logf (LOG_LOG, "end record");
#endif
		spec->stop_flag = 1;
	    }
	    else if (!strcmp (p, "element"))
	    {
                int min_level = 1;
                while ((r = execTok (spec, &s, &cmd_str, &cmd_len)) == 3)
                {
                    if (cmd_len==7 && !memcmp ("-record", cmd_str, cmd_len))
                        min_level = 0;
                }
		if (r > 2)
		{
		    tagEnd (spec, min_level, cmd_str, cmd_len);
		    r = execTok (spec, &s, &cmd_str, &cmd_len);
		}
		else
		    tagEnd (spec, min_level, NULL, 0);
                if (spec->d1_level == 0)
                {
#if REGX_DEBUG
		    logf (LOG_LOG, "end element end records");
#endif
		    spec->stop_flag = 1;
                }

	    }
	    else if (!strcmp (p, "context"))
	    {
#if REGX_DEBUG
		logf (LOG_LOG, "end context");
#endif
		if (spec->context_stack_top)
		    (spec->context_stack_top)--;
		r = execTok (spec, &s, &cmd_str, &cmd_len);
	    }	    
	    else
		logf (LOG_WARN, "bad keyword '%s' after end", p);
	}
        else if (!strcmp (p, "data"))
        {
            int textFlag = 0;
            int element_len;
            const char *element_str = NULL;
            
            while ((r = execTok (spec, &s, &cmd_str, &cmd_len)) == 3)
            {
                if (cmd_len==5 && !memcmp ("-text", cmd_str, cmd_len))
                    textFlag = 1;
                else if (cmd_len==8 && !memcmp ("-element", cmd_str, cmd_len))
                {
                    r = execTok (spec, &s, &element_str, &element_len);
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
                tagBegin (spec, element_str, element_len);
            do
            {
                execData (spec, cmd_str, cmd_len,textFlag);
                r = execTok (spec, &s, &cmd_str, &cmd_len);
            } while (r > 1);
            if (element_str)
                tagEnd (spec, 1, NULL, 0);
        }
        else if (!strcmp (p, "unread"))
        {
            int no, offset;
            r = execTok (spec, &s, &cmd_str, &cmd_len);
            if (r==3 && cmd_len == 7 && !memcmp ("-offset", cmd_str, cmd_len))
            {
                r = execTok (spec, &s, &cmd_str, &cmd_len);
                if (r < 2)
                {
                    logf (LOG_WARN, "missing number after -offset");
                    continue;
                }
                p = regxStrz (cmd_str, cmd_len, ptmp);
                offset = atoi (p);
                r = execTok (spec, &s, &cmd_str, &cmd_len);
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
                if (no >= spec->arg_no)
                    no = spec->arg_no - 1;
                spec->ptr = spec->arg_start[no] + offset;
            }
            r = execTok (spec, &s, &cmd_str, &cmd_len);
        }
	else if (!strcmp (p, "context"))
	{
            if (r > 1)
	    {
		struct lexContext *lc = spec->context;
		r = execTok (spec, &s, &cmd_str, &cmd_len);
		p = regxStrz (cmd_str, cmd_len, ptmp);
		
		while (lc && strcmp (p, lc->name))
		    lc = lc->next;
		if (lc)
		    spec->context_stack[spec->context_stack_top] = lc;
		else
		    logf (LOG_WARN, "unknown context %s", p);

	    }
	    r = execTok (spec, &s, &cmd_str, &cmd_len);
	}
        else
        {
            logf (LOG_WARN, "unknown code command '%.*s'", cmd_len, cmd_str);
            r = execTok (spec, &s, &cmd_str, &cmd_len);
            continue;
        }
        if (r > 1)
        {
            logf (LOG_WARN, "ignoring token %.*s", cmd_len, cmd_str);
            do {
                r = execTok (spec, &s, &cmd_str, &cmd_len);
            } while (r > 1);
        }
    }
}


static int execAction (struct lexSpec *spec, struct lexRuleAction *ap,
                       int start_ptr, int *pptr)
{
    int sptr;
    int arg_start[20];
    int arg_end[20];
    int arg_no = 1;

    if (!ap)
	return 1;
    arg_start[0] = start_ptr;
    arg_end[0] = *pptr;
    spec->arg_start = arg_start;
    spec->arg_end = arg_end;

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
	    spec->arg_no = arg_no;
	    spec->ptr = *pptr;
#if HAVE_TCL_H
	    if (spec->tcl_interp)
		execTcl(spec, ap->u.code);
	    else
		execCode (spec, ap->u.code);
#else
	    execCode (spec, ap->u.code);
#endif
	    *pptr = spec->ptr;
	    if (spec->stop_flag)
		return 0;
            break;
        case REGX_END:
            arg_start[arg_no] = *pptr;
            arg_end[arg_no] = F_WIN_EOF;
            arg_no++;
            *pptr = F_WIN_EOF;
        }
        ap = ap->next;
    }
    return 1;
}

static int execRule (struct lexSpec *spec, struct lexContext *context,
                     int ruleNo, int start_ptr, int *pptr)
{
#if REGX_DEBUG
    logf (LOG_LOG, "exec rule %d", ruleNo);
#endif
    return execAction (spec, context->fastRule[ruleNo]->actionList,
                       start_ptr, pptr);
}

data1_node *lexNode (struct lexSpec *spec, int *ptr)
{
    struct lexContext *context = spec->context_stack[spec->context_stack_top];
    struct DFA_state *state = context->dfa->states[0];
    struct DFA_tran *t;
    unsigned char c;
    unsigned char c_prev = '\n';
    int i;
    int last_rule = 0;        /* rule number of current match */
    int last_ptr = *ptr;      /* last char of match */
    int start_ptr = *ptr;     /* first char of match */
    int skip_ptr = *ptr;      /* first char of run */

    while (1)
    {
        c = f_win_advance (spec, ptr);
        if (*ptr == F_WIN_EOF)
        {
	    /* end of file met */
            if (last_rule)
            {
		/* there was a match */
                if (skip_ptr < start_ptr)
                {
		    /* deal with chars that didn't match */
                    int size;
                    char *buf;
                    buf = f_win_get (spec, skip_ptr, start_ptr, &size);
                    execDataP (spec, buf, size, 0);
                }
		/* restore pointer */
                *ptr = last_ptr;
		/* execute rule */
                if (!execRule (spec, context, last_rule, start_ptr, ptr))
		    break;
		/* restore skip pointer */
                skip_ptr = *ptr;
                last_rule = 0;
            }
            else if (skip_ptr < *ptr)
            {
		/* deal with chars that didn't match */
                int size;
                char *buf;
                buf = f_win_get (spec, skip_ptr, *ptr, &size);
                execDataP (spec, buf, size, 0);
            }
            if (*ptr == F_WIN_EOF)
                break;
        }
        t = state->trans;
        i = state->tran_no;
        while (1)
            if (--i < 0)
            {   /* no transition for character c ... */
                if (last_rule)
                {
                    if (skip_ptr < start_ptr)
                    {
			/* deal with chars that didn't match */
                        int size;
                        char *buf;
                        buf = f_win_get (spec, skip_ptr, start_ptr, &size);
                        execDataP (spec, buf, size, 0);
                    }
		    /* restore pointer */
                    *ptr = last_ptr;
                    if (!execRule (spec, context, last_rule, start_ptr, ptr))
                    {
                        if (spec->f_win_ef && *ptr != F_WIN_EOF)
			{
#if REGX_DEBUG
			    logf (LOG_LOG, "regx: endf ptr=%d", *ptr);
#endif
                            (*spec->f_win_ef)(spec->f_win_fh, *ptr);
			}
                        return NULL;
                    }
		    context = spec->context_stack[spec->context_stack_top];
                    skip_ptr = *ptr;
                    last_rule = 0;
                    last_ptr = start_ptr = *ptr;
                    if (start_ptr > 0)
                    {
                        --start_ptr;
                        c_prev = f_win_advance (spec, &start_ptr);
                    }
                }
                else
                {
                    c_prev = f_win_advance (spec, &start_ptr);
                    *ptr = start_ptr;
                }
                state = context->dfa->states[0];
                break;
            }
            else if (c >= t->ch[0] && c <= t->ch[1])
            {   /* transition ... */
                state = context->dfa->states[t->to];
                if (state->rule_no)
                {
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
                }
                break;
            }
            else
                t++;
    }
    return NULL;
}

static data1_node *lexRoot (struct lexSpec *spec, off_t offset,
			    const char *context_name)
{
    struct lexContext *lt = spec->context;
    int ptr = offset;

    spec->stop_flag = 0;
    spec->d1_level = 0;
    spec->context_stack_top = 0;    
    while (lt)
    {
	if (!strcmp (lt->name, context_name))
	    break;
	lt = lt->next;
    }
    if (!lt)
    {
	logf (LOG_WARN, "cannot find context %s", context_name);
	return NULL;
    }
    spec->context_stack[spec->context_stack_top] = lt;
    spec->d1_stack[spec->d1_level] = NULL;
#if 1
    if (!lt->initFlag)
    {
	lt->initFlag = 1;
	execAction (spec, lt->initActionList, ptr, &ptr);
    }
#endif
    execAction (spec, lt->beginActionList, ptr, &ptr);
    lexNode (spec, &ptr);
    while (spec->d1_level)
    {
	tagDataRelease (spec);
	(spec->d1_level)--;
    }
    execAction (spec, lt->endActionList, ptr, &ptr);
    return spec->d1_stack[0];
}

void grs_destroy(void *clientData)
{
    struct lexSpecs *specs = (struct lexSpecs *) clientData;
    if (specs->spec)
    {
	lexSpecDestroy(&specs->spec);
    }
    xfree (specs);
}

void *grs_init(void)
{
    struct lexSpecs *specs = (struct lexSpecs *) xmalloc (sizeof(*specs));
    specs->spec = 0;
    return specs;
}

data1_node *grs_read_regx (struct grs_read_info *p)
{
    int res;
    struct lexSpecs *specs = (struct lexSpecs *) p->clientData;
    struct lexSpec **curLexSpec = &specs->spec;

#if REGX_DEBUG
    logf (LOG_LOG, "grs_read_regx");
#endif
    if (!*curLexSpec || strcmp ((*curLexSpec)->name, p->type))
    {
        if (*curLexSpec)
            lexSpecDestroy (curLexSpec);
        *curLexSpec = lexSpecCreate (p->type, p->dh);
        res = readFileSpec (*curLexSpec);
        if (res)
        {
            lexSpecDestroy (curLexSpec);
            return NULL;
        }
    }
    (*curLexSpec)->dh = p->dh;
    if (!p->offset)
    {
        (*curLexSpec)->f_win_start = 0;
        (*curLexSpec)->f_win_end = 0;
        (*curLexSpec)->f_win_rf = p->readf;
        (*curLexSpec)->f_win_sf = p->seekf;
        (*curLexSpec)->f_win_fh = p->fh;
        (*curLexSpec)->f_win_ef = p->endf;
        (*curLexSpec)->f_win_size = 500000;
    }
    (*curLexSpec)->m = p->mem;
    return lexRoot (*curLexSpec, p->offset, "main");
}

static struct recTypeGrs regx_type = {
    "regx",
    grs_init,
    grs_destroy,
    grs_read_regx
};

RecTypeGrs recTypeGrs_regx = &regx_type;

#if HAVE_TCL_H
data1_node *grs_read_tcl (struct grs_read_info *p)
{
    int res;
    struct lexSpecs *specs = (struct lexSpecs *) p->clientData;
    struct lexSpec **curLexSpec = &specs->spec;

#if REGX_DEBUG
    logf (LOG_LOG, "grs_read_tcl");
#endif
    if (!*curLexSpec || strcmp ((*curLexSpec)->name, p->type))
    {
	Tcl_Interp *tcl_interp;
        if (*curLexSpec)
            lexSpecDestroy (curLexSpec);
        *curLexSpec = lexSpecCreate (p->type, p->dh);
	Tcl_FindExecutable("");
	tcl_interp = (*curLexSpec)->tcl_interp = Tcl_CreateInterp();
	Tcl_Init(tcl_interp);
	Tcl_CreateCommand (tcl_interp, "begin", cmd_tcl_begin, *curLexSpec, 0);
	Tcl_CreateCommand (tcl_interp, "end", cmd_tcl_end, *curLexSpec, 0);
	Tcl_CreateCommand (tcl_interp, "data", cmd_tcl_data, *curLexSpec, 0);
	Tcl_CreateCommand (tcl_interp, "unread", cmd_tcl_unread,
			   *curLexSpec, 0);
        res = readFileSpec (*curLexSpec);
        if (res)
        {
            lexSpecDestroy (curLexSpec);
            return NULL;
        }
    }
    (*curLexSpec)->dh = p->dh;
    if (!p->offset)
    {
        (*curLexSpec)->f_win_start = 0;
        (*curLexSpec)->f_win_end = 0;
        (*curLexSpec)->f_win_rf = p->readf;
        (*curLexSpec)->f_win_sf = p->seekf;
        (*curLexSpec)->f_win_fh = p->fh;
        (*curLexSpec)->f_win_ef = p->endf;
        (*curLexSpec)->f_win_size = 500000;
    }
    (*curLexSpec)->m = p->mem;
    return lexRoot (*curLexSpec, p->offset, "main");
}

static struct recTypeGrs tcl_type = {
    "tcl",
    grs_init,
    grs_destroy,
    grs_read_tcl
};

RecTypeGrs recTypeGrs_tcl = &tcl_type;
#endif
