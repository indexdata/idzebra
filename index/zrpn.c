/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zrpn.c,v $
 * Revision 1.14  1995-09-28 12:10:32  adam
 * Bug fixes. Field prefix used in queries.
 *
 * Revision 1.13  1995/09/18  14:17:50  adam
 * Minor changes.
 *
 * Revision 1.12  1995/09/15  14:45:21  adam
 * Retrieve control.
 * Work on truncation.
 *
 * Revision 1.11  1995/09/14  11:53:27  adam
 * First work on regular expressions/truncations.
 *
 * Revision 1.10  1995/09/11  15:23:26  adam
 * More work on relevance search.
 *
 * Revision 1.9  1995/09/11  13:09:35  adam
 * More work on relevance feedback.
 *
 * Revision 1.8  1995/09/08  14:52:27  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.7  1995/09/07  13:58:36  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 * Result-set references.
 *
 * Revision 1.6  1995/09/06  16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.5  1995/09/06  10:33:04  adam
 * More work on present. Some log messages removed.
 *
 * Revision 1.4  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 * Revision 1.3  1995/09/04  15:20:22  adam
 * Minor changes.
 *
 * Revision 1.2  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/04  09:10:40  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "zserver.h"

#include <rsisam.h>
#include <rstemp.h>
#include <rsnull.h>
#include <rsbool.h>
#include <rsrel.h>

/*
 * attr_print: log attributes
 */
static void attr_print (Z_AttributesPlusTerm *t)
{
    int of, i;
    for (of = 0; of < t->num_attributes; of++)
    {
        Z_AttributeElement *element;
        element = t->attributeList[of];

        switch (element->which) 
        {
        case Z_AttributeValue_numeric:
            logf (LOG_DEBUG, "attributeType=%d value=%d", 
                  *element->attributeType,
                  *element->value.numeric);
            break;
        case Z_AttributeValue_complex:
            logf (LOG_DEBUG, "attributeType=%d complex", 
                  *element->attributeType);
            for (i = 0; i<element->value.complex->num_list; i++)
            {
                if (element->value.complex->list[i]->which ==
                    Z_StringOrNumeric_string)
                    logf (LOG_DEBUG, "   string: '%s'",
                          element->value.complex->list[i]->u.string);
                else if (element->value.complex->list[i]->which ==
                         Z_StringOrNumeric_numeric)
                    logf (LOG_DEBUG, "   numeric: '%d'",
                          *element->value.complex->list[i]->u.numeric);
            }
            break;
        default:
            assert (0);
        }
    }
}

typedef struct {
    int type;
    int major;
    int minor;
    Z_AttributesPlusTerm *zapt;
} AttrType;

static int attr_find (AttrType *src)
{
    while (src->major < src->zapt->num_attributes)
    {
        Z_AttributeElement *element;
        element = src->zapt->attributeList[src->major];

        if (src->type == *element->attributeType)
        {
            switch (element->which) 
            {
            case Z_AttributeValue_numeric:
                ++(src->major);
                return *element->value.numeric;
                break;
            case Z_AttributeValue_complex:
                if (src->minor >= element->value.complex->num_list ||
                    element->value.complex->list[src->minor]->which !=  
                    Z_StringOrNumeric_numeric)
                    break;
                ++(src->minor);
                return *element->value.complex->list[src->minor-1]->u.numeric;
            default:
                assert (0);
            }
        }
        ++(src->major);
    }
    return -1;
}

static void attr_init (AttrType *src, Z_AttributesPlusTerm *zapt,
                       int type)
{
    src->zapt = zapt;
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

struct trunc_info {
    int  *indx;
    char **heap;
    int  heapnum;
    int  (*cmp)(const void *p1, const void *p2);
    int  keysize;
    char *swapbuf;
    char *tmpbuf;
    char *buf;
};

static void heap_swap (struct trunc_info *ti, int i1, int i2)
{
    int swap;

    memcpy (ti->swapbuf, ti->heap[i1], ti->keysize);
    memcpy (ti->heap[i1], ti->heap[i2], ti->keysize);
    memcpy (ti->heap[i2], ti->swapbuf, ti->keysize);

    swap = ti->indx[i1];
    ti->indx[i1] = ti->indx[i2];
    ti->indx[i2] = swap;
}

static void heap_delete (struct trunc_info *ti)
{
    int cur = 1, child = 2;

    assert (ti->heapnum > 0);
    memcpy (ti->heap[1], ti->heap[ti->heapnum], ti->keysize);
    ti->indx[1] = ti->indx[ti->heapnum--];
    while (child <= ti->heapnum) {
        if (child < ti->heapnum &&
            (*ti->cmp)(ti->heap[child], ti->heap[1+child]) > 0)
            child++;
        if ((*ti->cmp)(ti->heap[cur], ti->heap[child]) > 0)
        {
            heap_swap (ti, cur, child);
            cur = child;
            child = 2*cur;
        }
        else
            break;
    }
}

static void heap_insert (struct trunc_info *ti, const char *buf, int indx)
{
    int cur, parent;

    cur = ++(ti->heapnum);
    memcpy (ti->heap[cur], buf, ti->keysize);
    ti->indx[cur] = indx;
    parent = cur/2;
    while (parent && (*ti->cmp)(ti->heap[parent], ti->heap[cur]) > 0)
    {
        heap_swap (ti, cur, parent);
        cur = parent;
        parent = cur/2;
    }
}

static
struct trunc_info *heap_init (int size, int key_size,
                              int (*cmp)(const void *p1, const void *p2))
{
    struct trunc_info *ti = xmalloc (sizeof(*ti));
    int i;

    ++size;
    ti->heapnum = 0;
    ti->keysize = key_size;
    ti->cmp = cmp;
    ti->indx = xmalloc (size * sizeof(*ti->indx));
    ti->heap = xmalloc (size * sizeof(*ti->heap));
    ti->swapbuf = xmalloc (ti->keysize);
    ti->tmpbuf = xmalloc (ti->keysize);
    ti->buf = xmalloc (size * ti->keysize);
    for (i = size; --i >= 0; )
        ti->heap[i] = ti->buf + ti->keysize * i;
    return ti;
}

static void heap_close (struct trunc_info *ti)
{
    xfree (ti->indx);
    xfree (ti->heap);
    xfree (ti->swapbuf);
    xfree (ti->tmpbuf);
    xfree (ti);
}

static RSET rset_trunc (ISAM isam, ISAM_P *isam_p, int from, int to,
                        int merge_chunk)
{
    logf (LOG_DEBUG, "rset_trunc, range=%d-%d", from, to-1);
    if (to - from > merge_chunk)
    {
        return NULL;
    }
    else
    {
        ISPT *ispt;
        int i;
        struct trunc_info *ti;
        RSET result;
        RSFD rsfd;
        rset_temp_parms parms;

        ispt = xmalloc (sizeof(*ispt) * (to-from));
        parms.key_size = sizeof (struct it_key);
        result = rset_create (rset_kind_temp, &parms);
        rsfd = rset_open (result, 1);

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = is_position (isam, isam_p[from+i]);
            if (is_readkey (ispt[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
        }
        while (ti->heapnum)
        {
            int n = ti->indx[1];

            rset_write (result, rsfd, ti->heap[1]);
            heap_delete (ti);
            if (is_readkey (ispt[n], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, n);
        }
        for (i = to-from; --i >= 0; )
            is_pt_free (ispt[i]);
        rset_close (result, rsfd);
        heap_close (ti);
        xfree (ispt);
        return result;
    }
}

static ISAM_P *isam_p_buf = NULL;
static int isam_p_size = 0;
static int isam_p_indx;

static void add_isam_p (const char *info)
{
    if (isam_p_indx == isam_p_size)
    {
        ISAM_P *new_isam_p_buf;
        
        isam_p_size = 2*isam_p_size + 100;
        new_isam_p_buf = xmalloc (sizeof(*new_isam_p_buf) *
                                  isam_p_size);
        if (isam_p_buf)
        {
            memcpy (new_isam_p_buf, isam_p_buf,
                    isam_p_indx * sizeof(*isam_p_buf));
            xfree (isam_p_buf);
        }
        isam_p_buf = new_isam_p_buf;
    }
    assert (*info == sizeof(*isam_p_buf));
    memcpy (isam_p_buf + isam_p_indx, info+1, sizeof(*isam_p_buf));
    isam_p_indx++;
}

static int grep_handle (Dict_char *name, const char *info)
{
    logf (LOG_DEBUG, "dict name: %s", name);
    add_isam_p (info);
    return 0;
}

static int trunc_term (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
                       const char *term_sub, ISAM_P **isam_ps)
{
    char term_dict[2*IT_MAX_WORD+2];
    int i, j;
    const char *info;    
    AttrType truncation;
    int truncation_value;

    attr_init (&truncation, zapt, 5);
    truncation_value = attr_find (&truncation);
    logf (LOG_DEBUG, "truncation value %d", truncation_value);
    switch (truncation_value)
    {
    case -1:         /* not specified */
    case 100:        /* do not truncate */
        strcpy (term_dict, term_sub);
        logf (LOG_DEBUG, "dict_lookup: %s", term_dict);
        if ((info = dict_lookup (zi->wordDict, term_dict)))
            add_isam_p (info);
        break;
    case 1:          /* right truncation */
        strcpy (term_dict, term_sub);
        strcat (term_dict, ".*");
        dict_lookup_grep (zi->wordDict, term_dict, 0, grep_handle);
        break;
    case 2:          /* left truncation */
    case 3:          /* left&right truncation */
        zi->errCode = 120;
        return -1;
    case 101:        /* process # in term */
        for (j = 0, i = 0; term_sub[i] && i < 2; i++)
            term_dict[j++] = term_sub[i];
        for (; term_sub[i]; i++)
            if (term_sub[i] == '#')
            {
                term_dict[j++] = '.';
                term_dict[j++] = '*';
            }
            else
                term_dict[j++] = term_sub[i];
        term_dict[j] = '\0';
        dict_lookup_grep (zi->wordDict, term_dict, 0, grep_handle);
        break;
    case 102:        /* regular expression */
        strcpy (term_dict, term_sub);
        dict_lookup_grep (zi->wordDict, term_dict, 0, grep_handle);
        break;
    }
    *isam_ps = isam_p_buf;
    logf (LOG_DEBUG, "%d positions", isam_p_indx);
    return 0;
}

static RSET rpn_search_APT_relevance (ZServerInfo *zi, 
                                      Z_AttributesPlusTerm *zapt)
{
    rset_relevance_parms parms;
    char termz[IT_MAX_WORD+1];
    char term_sub[IT_MAX_WORD+1];
    char *p0 = termz, *p1 = NULL;
    Z_Term *term = zapt->term;
    size_t sizez, i;

    parms.key_size = sizeof(struct it_key);
    parms.max_rec = 100;
    parms.cmp = key_compare;
    parms.is = zi->wordIsam;

    if (term->which != Z_Term_general)
    {
        zi->errCode = 124;
        return NULL;
    }
    i = index_word_prefix (termz, 1, 1016);
    sizez = i + term->u.general->len;
    if (sizez > IT_MAX_WORD)
        sizez = IT_MAX_WORD;
    for ( ; i < sizez; i++)
        termz[i] = index_char_cvt (term->u.general->buf[i]);
    termz[i] = '\0';

    isam_p_indx = 0;  /* global, set by trunc_term - see below */
    while (1)
    {
        if ((p1 = strchr (p0, ' ')))
        {
            memcpy (term_sub, p0, p1-p0);
            term_sub[p1-p0] = '\0';
        }
        else
            strcpy (term_sub, p0);
        if (trunc_term (zi, zapt, term_sub, &parms.isam_positions))
            return NULL;
        if (!p1)
            break;
        p0 = p1+1;
    }
    parms.no_isam_positions = isam_p_indx;
    if (isam_p_indx > 0)
        return rset_create (rset_kind_relevance, &parms);
    else
        return rset_create (rset_kind_null, NULL);
}

static RSET rpn_search_APT_word (ZServerInfo *zi,
                                 Z_AttributesPlusTerm *zapt)
{
    ISAM_P *isam_positions;
    rset_isam_parms parms;

    char termz[IT_MAX_WORD+1];
    Z_Term *term = zapt->term;
    size_t sizez, i;

    if (term->which != Z_Term_general)
    {
        zi->errCode = 124;
        return NULL;
    }
    i = index_word_prefix (termz, 1, 1016);
    sizez = i + term->u.general->len;
    if (sizez > IT_MAX_WORD)
        sizez = IT_MAX_WORD;
    for ( ; i < sizez; i++)
        termz[i] = index_char_cvt (term->u.general->buf[i]);
    termz[i] = '\0';

    isam_p_indx = 0;  /* global, set by trunc_term - see below */
    if (trunc_term (zi, zapt, termz, &isam_positions))
        return NULL;
    if (isam_p_indx < 1)
        return rset_create (rset_kind_null, NULL);
    else if (isam_p_indx == 1)
    {
        parms.is = zi->wordIsam;
        parms.pos = *isam_positions;
        return rset_create (rset_kind_isam, &parms);
    }
    else
        return rset_trunc (zi->wordIsam, isam_positions, 0, isam_p_indx, 400);
}

static RSET rpn_search_APT_phrase (ZServerInfo *zi,
                                   Z_AttributesPlusTerm *zapt)
{
    ISAM_P *isam_positions;
    rset_isam_parms parms;

    char termz[IT_MAX_WORD+1];
    Z_Term *term = zapt->term;
    size_t sizez, i;

    if (term->which != Z_Term_general)
    {
        zi->errCode = 124;
        return NULL;
    }
    i = index_word_prefix (termz, 1, 1016);
    sizez = i + term->u.general->len;
    if (sizez > IT_MAX_WORD)
        sizez = IT_MAX_WORD;
    for ( ; i < sizez; i++)
        termz[i] = index_char_cvt (term->u.general->buf[i]);
    termz[i] = '\0';

    isam_p_indx = 0;  /* global, set by trunc_term - see below */
    if (trunc_term (zi, zapt, termz, &isam_positions))
        return NULL;
    if (isam_p_indx != 1)
        return rset_create (rset_kind_null, NULL);
    parms.is = zi->wordIsam;
    parms.pos = *isam_positions;
    return rset_create (rset_kind_isam, &parms);
}

static RSET rpn_search_APT (ZServerInfo *zi, Z_AttributesPlusTerm *zapt)
{
    AttrType relation;
    AttrType structure;
    int relation_value, structure_value;

    attr_init (&relation, zapt, 2);
    attr_init (&structure, zapt, 4);
    
    relation_value = attr_find (&relation);
    structure_value = attr_find (&structure);
    switch (structure_value)
    {
    case -1:
        if (relation_value == 102) /* relevance relation */
            return rpn_search_APT_relevance (zi, zapt);
        return rpn_search_APT_word (zi, zapt);
    case 1: /* phrase */
        if (relation_value == 102) /* relevance relation */
            return rpn_search_APT_relevance (zi, zapt);
        return rpn_search_APT_phrase (zi, zapt);
        break;
    case 2: /* word */
        if (relation_value == 102) /* relevance relation */
            return rpn_search_APT_relevance (zi, zapt);
        return rpn_search_APT_word (zi, zapt);
    case 3: /* key */
        break;
    case 4: /* year */
        break;
    case 5: /* date - normalized */
        break;
    case 6: /* word list */
        return rpn_search_APT_relevance (zi, zapt);
    case 100: /* date - un-normalized */
        break;
    case 101: /* name - normalized */
        break;
    case 102: /* date - un-normalized */
        break;
    case 103: /* structure */
        break;
    case 104: /* urx */
        break;
    case 105: /* free-form-text */
        return rpn_search_APT_relevance (zi, zapt);
    case 106: /* document-text */
        return rpn_search_APT_relevance (zi, zapt);
    case 107: /* local-number */
        break;
    case 108: /* string */ 
        return rpn_search_APT_word (zi, zapt);
    case 109: /* numeric string */
        break;
    }
    zi->errCode = 118;
    return NULL;
}

static RSET rpn_search_ref (ZServerInfo *zi, Z_ResultSetId *resultSetId)
{
    ZServerSet *s;

    if (!(s = resultSetGet (zi, resultSetId)))
        return rset_create (rset_kind_null, NULL);
    return s->rset;
}

static RSET rpn_search_structure (ZServerInfo *zi, Z_RPNStructure *zs)
{
    RSET r = NULL;
    if (zs->which == Z_RPNStructure_complex)
    {
        rset_bool_parms bool_parms;

        bool_parms.rset_l = rpn_search_structure (zi, zs->u.complex->s1);
        if (bool_parms.rset_l == NULL)
            return NULL;
        bool_parms.rset_r = rpn_search_structure (zi, zs->u.complex->s2);
        if (bool_parms.rset_r == NULL)
        {
            rset_delete (bool_parms.rset_l);
            return NULL;
        }
        bool_parms.key_size = sizeof(struct it_key);
        bool_parms.cmp = key_compare;

        switch (zs->u.complex->operator->which)
        {
        case Z_Operator_and:
            r = rset_create (rset_kind_and, &bool_parms);
            break;
        case Z_Operator_or:
            r = rset_create (rset_kind_or, &bool_parms);
            break;
        case Z_Operator_and_not:
            r = rset_create (rset_kind_not, &bool_parms);
            break;
        default:
            assert (0);
        }
    }
    else if (zs->which == Z_RPNStructure_simple)
    {
        if (zs->u.simple->which == Z_Operand_APT)
        {
            logf (LOG_DEBUG, "rpn_search_APT");
            r = rpn_search_APT (zi, zs->u.simple->u.attributesPlusTerm);
        }
        else if (zs->u.simple->which == Z_Operand_resultSetId)
        {
            logf (LOG_DEBUG, "rpn_search_ref");
            r = rpn_search_ref (zi, zs->u.simple->u.resultSetId);
        }
        else
        {
            assert (0);
        }
    }
    else
    {
        assert (0);
    }
    return r;
}

static void count_set (RSET r, int *count)
{
    int psysno = 0;
    struct it_key key;
    RSFD rfd;

    logf (LOG_DEBUG, "rpn_save_set");
    *count = 0;
    rfd = rset_open (r, 0);
    while (rset_read (r, rfd, &key))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
            (*count)++;
        }
    }
    rset_close (r, rfd);
    logf (LOG_DEBUG, "%d distinct sysnos", *count);
}

int rpn_search (ZServerInfo *zi,
                Z_RPNQuery *rpn, int num_bases, char **basenames, 
                const char *setname, int *hits)
{
    RSET rset;

    zi->errCode = 0;
    zi->errString = NULL;
    rset = rpn_search_structure (zi, rpn->RPNStructure);
    if (!rset)
        return zi->errCode;
    count_set (rset, hits);
    resultSetAdd (zi, setname, 1, rset);
    return zi->errCode;
}

