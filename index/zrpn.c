/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zrpn.c,v $
 * Revision 1.49  1996-06-04 10:18:11  adam
 * Search/scan uses character mapping module.
 *
 * Revision 1.48  1996/05/28  15:15:01  adam
 * Bug fix: Didn't handle unknown database correctly.
 *
 * Revision 1.47  1996/05/15  18:36:28  adam
 * Function trans_term transforms unsearchable characters to blanks.
 *
 * Revision 1.46  1996/05/15  11:57:56  adam
 * Fixed bug introduced by set/field mapping in search operations.
 *
 * Revision 1.45  1996/05/14  11:34:00  adam
 * Scan support in multiple registers/databases.
 *
 * Revision 1.44  1996/05/14  06:16:44  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.43  1996/05/09 09:54:43  adam
 * Server supports maps from one logical attributes to a list of physical
 * attributes.
 * The extraction process doesn't make space consuming 'any' keys.
 *
 * Revision 1.42  1996/05/09  07:28:56  quinn
 * Work towards phrases and multiple registers
 *
 * Revision 1.41  1996/03/20  09:36:43  adam
 * Function dict_lookup_grep got extra parameter, init_pos, which marks
 * from which position in pattern approximate pattern matching should occur.
 * Approximate pattern matching is used in relevance=re-2.
 *
 * Revision 1.40  1996/02/02  13:44:44  adam
 * The public dictionary functions simply use char instead of Dict_char
 * to represent search strings. Dict_char is used internally only.
 *
 * Revision 1.39  1996/01/03  16:22:13  quinn
 * operator->roperator
 *
 * Revision 1.38  1995/12/11  09:12:55  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.37  1995/12/06  15:05:28  adam
 * More verbose in count_set.
 *
 * Revision 1.36  1995/12/06  12:41:27  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.35  1995/11/27  09:29:00  adam
 * Bug fixes regarding conversion to regular expressions.
 *
 * Revision 1.34  1995/11/16  17:00:56  adam
 * Better logging of rpn query.
 *
 * Revision 1.33  1995/11/01  13:58:28  quinn
 * Moving data1 to yaz/retrieval
 *
 * Revision 1.32  1995/10/27  14:00:11  adam
 * Implemented detection of database availability.
 *
 * Revision 1.31  1995/10/17  18:02:10  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.30  1995/10/16  09:32:38  adam
 * More work on relational op.
 *
 * Revision 1.29  1995/10/13  16:01:49  adam
 * Work on relations.
 *
 * Revision 1.28  1995/10/13  12:26:43  adam
 * Optimization of truncation.
 *
 * Revision 1.27  1995/10/12  17:07:22  adam
 * Truncation works.
 *
 * Revision 1.26  1995/10/12  12:40:54  adam
 * Bug fixes in rpn_prox.
 *
 * Revision 1.25  1995/10/10  13:59:24  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.24  1995/10/09  16:18:37  adam
 * Function dict_lookup_grep got extra client data parameter.
 *
 * Revision 1.23  1995/10/06  16:33:37  adam
 * Use attribute mappings.
 *
 * Revision 1.22  1995/10/06  15:07:39  adam
 * Structure 'local-number' handled.
 *
 * Revision 1.21  1995/10/06  13:52:06  adam
 * Bug fixes. Handler may abort further scanning.
 *
 * Revision 1.20  1995/10/06  11:06:33  adam
 * Scan entries include 'occurrences' now.
 *
 * Revision 1.19  1995/10/06  10:43:56  adam
 * Scan added. 'occurrences' in scan entries not set yet.
 *
 * Revision 1.18  1995/10/04  16:57:20  adam
 * Key input and merge sort in one pass.
 *
 * Revision 1.17  1995/10/04  12:55:17  adam
 * Bug fix in ranked search. Use=Any keys inserted.
 *
 * Revision 1.16  1995/10/02  16:24:40  adam
 * Use attribute actually used in search requests.
 *
 * Revision 1.15  1995/10/02  15:18:52  adam
 * New member in recRetrieveCtrl: diagnostic.
 *
 * Revision 1.14  1995/09/28  12:10:32  adam
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
#include <ctype.h>

#include "zserver.h"
#include "attribute.h"

#include <charmap.h>
#include <rsisam.h>
#include <rstemp.h>
#include <rsnull.h>
#include <rsbool.h>
#include <rsrel.h>

typedef struct {
    int type;
    int major;
    int minor;
    Z_AttributesPlusTerm *zapt;
} AttrType;

static int attr_find (AttrType *src, oid_value *attributeSetP)
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
                if (element->attributeSet && attributeSetP)
                {
                    oident *attrset;

                    attrset = oid_getentbyoid (element->attributeSet);
                    *attributeSetP = attrset->value;
                }
                return *element->value.numeric;
                break;
            case Z_AttributeValue_complex:
                if (src->minor >= element->value.complex->num_list ||
                    element->value.complex->list[src->minor]->which !=  
                    Z_StringOrNumeric_numeric)
                    break;
                ++(src->minor);
                if (element->attributeSet && attributeSetP)
                {
                    oident *attrset;

                    attrset = oid_getentbyoid (element->attributeSet);
                    *attributeSetP = attrset->value;
                }
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
    int  *ptr;
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

    swap = ti->ptr[i1];
    ti->ptr[i1] = ti->ptr[i2];
    ti->ptr[i2] = swap;
}

static void heap_delete (struct trunc_info *ti)
{
    int cur = 1, child = 2;

    heap_swap (ti, 1, ti->heapnum--);
    while (child <= ti->heapnum) {
        if (child < ti->heapnum &&
            (*ti->cmp)(ti->heap[ti->ptr[child]],
                       ti->heap[ti->ptr[1+child]]) > 0)
            child++;
        if ((*ti->cmp)(ti->heap[ti->ptr[cur]],
                       ti->heap[ti->ptr[child]]) > 0)
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
    memcpy (ti->heap[ti->ptr[cur]], buf, ti->keysize);
    ti->indx[ti->ptr[cur]] = indx;
    parent = cur/2;
    while (parent && (*ti->cmp)(ti->heap[ti->ptr[parent]],
                                ti->heap[ti->ptr[cur]]) > 0)
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
    ti->ptr = xmalloc (size * sizeof(*ti->ptr));
    ti->swapbuf = xmalloc (ti->keysize);
    ti->tmpbuf = xmalloc (ti->keysize);
    ti->buf = xmalloc (size * ti->keysize);
    for (i = size; --i >= 0; )
    {
        ti->ptr[i] = i;
        ti->heap[i] = ti->buf + ti->keysize * i;
    }
    return ti;
}

static void heap_close (struct trunc_info *ti)
{
    xfree (ti->ptr);
    xfree (ti->indx);
    xfree (ti->heap);
    xfree (ti->swapbuf);
    xfree (ti->tmpbuf);
    xfree (ti);
}

static RSET rset_trunc_r (ISAM isam, ISAM_P *isam_p, int from, int to,
                         int merge_chunk)
{
    RSET result; 
    RSFD result_rsfd;
    rset_temp_parms parms;

    parms.key_size = sizeof(struct it_key);
    result = rset_create (rset_kind_temp, &parms);
    result_rsfd = rset_open (result, RSETF_WRITE|RSETF_SORT_SYSNO);

    if (to - from > merge_chunk)
    {
        RSFD *rsfd;
        RSET *rset;
        int i, i_add = (to-from)/merge_chunk + 1;
        struct trunc_info *ti;
        int rscur = 0;
        int rsmax = (to-from)/i_add + 1;
        
        rset = xmalloc (sizeof(*rset) * rsmax);
        rsfd = xmalloc (sizeof(*rsfd) * rsmax);
        
        for (i = from; i < to; i += i_add)
        {
            if (i_add <= to - i)
                rset[rscur] = rset_trunc_r (isam, isam_p, i, i+i_add,
                                            merge_chunk);
            else
                rset[rscur] = rset_trunc_r (isam, isam_p, i, to,
                                            merge_chunk);
            rscur++;
        }
        ti = heap_init (rscur, sizeof(struct it_key), key_compare);
        for (i = rscur; --i >= 0; )
        {
            rsfd[i] = rset_open (rset[i], RSETF_READ|RSETF_SORT_SYSNO);
            if (rset_read (rset[i], rsfd[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
            {
                rset_close (rset[i], rsfd[i]);
                rset_delete (rset[i]);
            }
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);

            while (1)
            {
                if (!rset_read (rset[n], rsfd[n], ti->tmpbuf))
                {
                    heap_delete (ti);
                    rset_close (rset[n], rsfd[n]);
                    rset_delete (rset[n]);
                    break;
                }
                if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
                {
                    heap_delete (ti);
                    heap_insert (ti, ti->tmpbuf, n);
                    break;
                }
            }
        }
        xfree (rset);
        xfree (rsfd);
        heap_close (ti);
    }
    else
    {
        ISPT *ispt;
        int i;
        struct trunc_info *ti;

        ispt = xmalloc (sizeof(*ispt) * (to-from));

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = is_position (isam, isam_p[from+i]);
            if (is_readkey (ispt[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
                is_pt_free (ispt[i]);
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);
#if 0
/* section that preserve all keys */
            heap_delete (ti);
            if (is_readkey (ispt[n], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, n);
            else
                is_pt_free (ispt[n]);
#else
/* section that preserve all keys with unique sysnos */
            while (1)
            {
                if (!is_readkey (ispt[n], ti->tmpbuf))
                {
                    heap_delete (ti);
                    is_pt_free (ispt[n]);
                    break;
                }
                if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
                {
                    heap_delete (ti);
                    heap_insert (ti, ti->tmpbuf, n);
                    break;
                }
            }
#endif
        }
        heap_close (ti);
        xfree (ispt);
    }
    rset_close (result, result_rsfd);
    return result;
}

static int isam_trunc_cmp (const void *p1, const void *p2)
{
    ISAM_P i1 = *(ISAM_P*) p1;
    ISAM_P i2 = *(ISAM_P*) p2;
    int d;

    d = is_type (i1) - is_type (i2);
    if (d)
        return d;
    return is_block (i1) - is_block (i2);
}

static RSET rset_trunc (ISAM isam, ISAM_P *isam_p, int no)
{

    qsort (isam_p, no, sizeof(*isam_p), isam_trunc_cmp);
    return rset_trunc_r (isam, isam_p, 0, no, 100);
}

struct grep_info {
    ISAM_P *isam_p_buf;
    int isam_p_size;
    int isam_p_indx;
};

static void add_isam_p (const char *info, struct grep_info *p)
{
    if (p->isam_p_indx == p->isam_p_size)
    {
        ISAM_P *new_isam_p_buf;
        
        p->isam_p_size = 2*p->isam_p_size + 100;
        new_isam_p_buf = xmalloc (sizeof(*new_isam_p_buf) *
                                  p->isam_p_size);
        if (p->isam_p_buf)
        {
            memcpy (new_isam_p_buf, p->isam_p_buf,
                    p->isam_p_indx * sizeof(*p->isam_p_buf));
            xfree (p->isam_p_buf);
        }
        p->isam_p_buf = new_isam_p_buf;
    }
    assert (*info == sizeof(*p->isam_p_buf));
    memcpy (p->isam_p_buf + p->isam_p_indx, info+1, sizeof(*p->isam_p_buf));
    (p->isam_p_indx)++;
}

static int grep_handle (char *name, const char *info, void *p)
{
    logf (LOG_DEBUG, "dict name: %s", name);
    add_isam_p (info, p);
    return 0;
}

static void gen_regular_rel (char *dst, int val, int islt)
{
    int dst_p = 1;
    int w, d, i;
    int pos = 0;
    char numstr[20];

    *dst = '(';
    sprintf (numstr, "%d", val);
    for (w = strlen(numstr); --w >= 0; pos++)
    {
        d = numstr[w];
        if (pos > 0)
        {
            if (islt)
            {
                if (d == '0')
                    continue;
                d--;
            } 
            else
            {
                if (d == '9')
                    continue;
                d++;
            }
        }
        
        strcpy (dst + dst_p, numstr);
        dst_p = strlen(dst) - pos - 1;

        if (islt)
        {
            if (d != '0')
            {
                dst[dst_p++] = '[';
                dst[dst_p++] = '0';
                dst[dst_p++] = '-';
                dst[dst_p++] = d;
                dst[dst_p++] = ']';
            }
            else
                dst[dst_p++] = d;
        }
        else
        {
            if (d != '9')
            { 
                dst[dst_p++] = '[';
                dst[dst_p++] = d;
                dst[dst_p++] = '-';
                dst[dst_p++] = '9';
                dst[dst_p++] = ']';
            }
            else
                dst[dst_p++] = d;
        }
        for (i = 0; i<pos; i++)
        {
            dst[dst_p++] = '[';
            dst[dst_p++] = '0';
            dst[dst_p++] = '-';
            dst[dst_p++] = '9';
            dst[dst_p++] = ']';
        }
        dst[dst_p++] = '|';
    }
    dst[dst_p] = '\0';
    if (islt)
    {
        for (i=1; i<pos; i++)
            strcat (dst, "[0-9]?");
    }
    else
    {
        for (i = 0; i <= pos; i++)
            strcat (dst, "[0-9]");
        strcat (dst, "[0-9]*");
    }
    strcat (dst, ")");
}

static int relational_term (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
                            const char *term_sub,
                            char *term_dict,
                            oid_value attributeSet,
                            struct grep_info *grep_info,
                            int *max_pos)
{
    AttrType relation;
    int relation_value;
    int term_value;
    int r;

    attr_init (&relation, zapt, 2);
    relation_value = attr_find (&relation, NULL);
    term_value = atoi (term_sub);

    switch (relation_value)
    {
    case 1:
        if (term_value <= 0)
            return 1;
        logf (LOG_DEBUG, "Relation <");
        gen_regular_rel (term_dict + strlen(term_dict), term_value-1, 1);
        break;
    case 2:
        if (term_value < 0)
            return 1;
        logf (LOG_DEBUG, "Relation <=");
        gen_regular_rel (term_dict + strlen(term_dict), term_value, 1);
        break;
    case 4:
        if (term_value < 0)
            term_value = 0;
        logf (LOG_DEBUG, "Relation >=");
        gen_regular_rel (term_dict + strlen(term_dict), term_value, 0);
        break;
    case 5:
        if (term_value < 0)
            term_value = 0;
        logf (LOG_DEBUG, "Relation >");
        gen_regular_rel (term_dict + strlen(term_dict), term_value+1, 0);
        break;
    default:
        return 0;
    }
    logf (LOG_DEBUG, "dict_lookup_grep: %s", term_dict);
    r = dict_lookup_grep (zi->wordDict, term_dict, 0, grep_info, max_pos,
                          0, grep_handle);
    if (r)
        logf (LOG_WARN, "dict_lookup_grep fail, rel=gt: %d", r);
    logf (LOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return 1;
}

static void verbatim_char (int ch, int *indx, char *dst)
{
    if (!isalnum (ch))
        dst[(*indx)++] = '\\';
    dst[(*indx)++] = ch;
}

static int field_term (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
                       const char *term_sub, int regType,
                       oid_value attributeSet, struct grep_info *grep_info,
                       int num_bases, char **basenames)
{
    char term_dict[2*IT_MAX_WORD+2];
    int i, j, r, base_no;
    AttrType truncation;
    int truncation_value;
    AttrType use;
    int use_value;
    oid_value curAttributeSet = attributeSet;

    attr_init (&use, zapt, 1);
    use_value = attr_find (&use, &curAttributeSet);
    logf (LOG_DEBUG, "use value %d", use_value);
    attr_init (&truncation, zapt, 5);
    truncation_value = attr_find (&truncation, NULL);
    logf (LOG_DEBUG, "truncation value %d", truncation_value);

    if (use_value == -1)
        use_value = 1016;

    for (base_no = 0; base_no < num_bases; base_no++)
    {
        attent *attp;
        data1_local_attribute *local_attr;
        int max_pos, prefix_len = 0;

        attp = att_getentbyatt (curAttributeSet, use_value);
        if (!attp)
        {
            logf (LOG_DEBUG, "att_getentbyatt fail. set=%d use=%d",
                  curAttributeSet, use_value);
            zi->errCode = 114;
            return -1;
        }
        if (zebTargetInfo_curDatabase (zi->zti, basenames[base_no]))
        {
            zi->errCode = 109; /* Database unavailable */
            zi->errString = basenames[base_no];
            return -1;
        }
        for (local_attr = attp->local_attributes; local_attr;
             local_attr = local_attr->next)
        {
            int ord;

            ord = zebTargetInfo_lookupSU (zi->zti, attp->attset_ordinal,
                                          local_attr->local);
            if (ord < 0)
                continue;
            if (prefix_len)
                term_dict[prefix_len++] = '|';
            else
                term_dict[prefix_len++] = '(';
            term_dict[prefix_len++] = 1;
            term_dict[prefix_len++] = ord;
        }
        if (!prefix_len)
        {
            zi->errCode = 114;
            return -1;
        }
        term_dict[prefix_len++] = ')';        
        term_dict[prefix_len++] = 1;
        term_dict[prefix_len++] = regType;
        term_dict[prefix_len] = '\0';
        if (!relational_term (zi, zapt, term_sub, term_dict,
                              attributeSet, grep_info, &max_pos))
        {
            const char *cp;

            j = prefix_len;
            switch (truncation_value)
            {
            case -1:         /* not specified */
            case 100:        /* do not truncate */
                term_dict[j++] = '(';
                for (i = 0; term_sub[i]; i++)
                    verbatim_char (term_sub[i], &j, term_dict);
                strcpy (term_dict+j, ")");
                r = dict_lookup_grep (zi->wordDict, term_dict, 0, grep_info,
                                      &max_pos, 0, grep_handle);
                if (r)
                    logf (LOG_WARN, "dict_lookup_grep err, trunc=none:%d", r);
                break;
            case 1:          /* right truncation */
                term_dict[j++] = '(';
                for (i = 0; term_sub[i]; i++)
                    verbatim_char (term_sub[i], &j, term_dict);
                strcpy (term_dict+j, ".*)");
                dict_lookup_grep (zi->wordDict, term_dict, 0, grep_info,
                                  &max_pos, 0, grep_handle);
                break;
            case 2:          /* left truncation */
            case 3:          /* left&right truncation */
                zi->errCode = 120;
                return -1;
            case 101:        /* process # in term */
                term_dict[j++] = '(';
                for (i=0; term_sub[i]; i++)
                    if (term_sub[i] == '#' && i > 2)
                    {
                        term_dict[j++] = '.';
                        term_dict[j++] = '*';
                    }
                    else
                        verbatim_char (term_sub[i], &j, term_dict);
                strcpy (term_dict+j, ")");
                r = dict_lookup_grep (zi->wordDict, term_dict, 0, grep_info,
                                      &max_pos, 0, grep_handle);
                if (r)
                    logf (LOG_WARN, "dict_lookup_grep err, trunc=#: %d",
                          r);
                break;
            case 102:        /* regular expression */
		sprintf (term_dict + j, "(%s)", term_sub);
                r = dict_lookup_grep (zi->wordDict, term_dict, 0, grep_info,
                                      &max_pos, 0, grep_handle);
                if (r)
                    logf (LOG_WARN, "dict_lookup_grep err, trunc=regular: %d",
                          r);
                break;
            case 103:        /* regular expression with error correction */
                cp = term_sub;
                r = 0;
		if (*cp == '*' && cp[1] && cp[2])
                {
                    r = atoi (cp+1);
                    cp += 2;
                }
		sprintf (term_dict + j, "(%s)", cp);
                r = dict_lookup_grep (zi->wordDict, term_dict, r, grep_info,
                                      &max_pos, j, grep_handle);
                if (r)
                    logf (LOG_WARN, "dict_lookup_grep err, trunc=eregular: %d",
                          r);
                break;
            }
        }
    }
    logf (LOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return 0;
}

static void trans_term (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
                        char *termz)
{
    size_t sizez;
    Z_Term *term = zapt->term;

    sizez = term->u.general->len;
    if (sizez > IT_MAX_WORD-1)
        sizez = IT_MAX_WORD-1;
    memcpy (termz, term->u.general->buf, sizez);
    termz[sizez] = '\0';
}

static void trans_scan_term (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
                             char *termz)
{
    Z_Term *term = zapt->term;
    char **map;
    const char *cp = (const char *) term->u.general->buf;
    const char *cp_end = cp + term->u.general->len;
    const char *src;
    int i = 0;
    int prev_space = 0;
    int len;
    
    while ((len = (cp_end - cp)) > 0)
    {
        map = map_chrs_input (&cp, len);
        if (**map == *CHR_SPACE)
        {
            if (prev_space)
                continue;
            prev_space = 1;
        } 
        else
            prev_space = 0;
        for (src = *map; *src; src++)
            termz[i++] = *src;
    }
    termz[i] = '\0';
}

static RSET rpn_search_APT_relevance (ZServerInfo *zi, 
                                      Z_AttributesPlusTerm *zapt,
                                      oid_value attributeSet,
                                      int num_bases, char **basenames)
{
    rset_relevance_parms parms;
    char termz[IT_MAX_WORD+1];
    char term_sub[IT_MAX_WORD+1];
    struct grep_info grep_info;
    char *p0 = termz, *p1 = NULL;
    RSET result;

    parms.key_size = sizeof(struct it_key);
    parms.max_rec = 100;
    parms.cmp = key_compare;
    parms.is = zi->wordIsam;

    if (zapt->term->which != Z_Term_general)
    {
        zi->errCode = 124;
        return NULL;
    }
    trans_term (zi, zapt, termz);

    grep_info.isam_p_indx = 0;
    grep_info.isam_p_size = 0;
    grep_info.isam_p_buf = NULL;
    while (1)
    {
        char **map;
        char *p2 = NULL;
        p1 = p0;
        while (*p1)
        {
            map = map_chrs_input (&p1, strlen(p1));
            if (**map == *CHR_SPACE)
                break;
            p2 = p1;
        }
        if (p1 == p0)
            break;
        memcpy (term_sub, p0, p2-p0);
        term_sub[p2-p0] = '\0';
        while (*p1)
        {
            map = map_chrs_input (&p1, strlen(p1));
            if (**map == *CHR_SPACE)
                break;
        }
        if (field_term (zi, zapt, term_sub, 'w', attributeSet, &grep_info,
                        num_bases, basenames))
            return NULL;
        if (!p1)
            break;
        p0 = p1;
    }
    parms.isam_positions = grep_info.isam_p_buf;
    parms.no_isam_positions = grep_info.isam_p_indx;
    if (grep_info.isam_p_indx > 0)
        result = rset_create (rset_kind_relevance, &parms);
    else
        result = rset_create (rset_kind_null, NULL);
    xfree (grep_info.isam_p_buf);
    return result;
}

static RSET rpn_search_APT_cphrase (ZServerInfo *zi,
                                    Z_AttributesPlusTerm *zapt,
                                    oid_value attributeSet,
                                    int num_bases, char **basenames)
{
    rset_isam_parms parms;
    char termz[IT_MAX_WORD+1];
    struct grep_info grep_info;
    RSET result;

    if (zapt->term->which != Z_Term_general)
    {
        zi->errCode = 124;
        return NULL;
    }
    trans_term (zi, zapt, termz);

    grep_info.isam_p_indx = 0;
    grep_info.isam_p_size = 0;
    grep_info.isam_p_buf = NULL;

    if (field_term (zi, zapt, termz, 'p', attributeSet, &grep_info,
                    num_bases, basenames))
        return NULL;
    if (grep_info.isam_p_indx < 1)
        result = rset_create (rset_kind_null, NULL);
    else if (grep_info.isam_p_indx == 1)
    {
        parms.is = zi->wordIsam;
        parms.pos = *grep_info.isam_p_buf;
        result = rset_create (rset_kind_isam, &parms);
    }
    else
        result = rset_trunc (zi->wordIsam, grep_info.isam_p_buf,
                             grep_info.isam_p_indx);
    xfree (grep_info.isam_p_buf);
    return result;
}

static RSET rpn_prox (RSET *rset, int rset_no)
{
    int i;
    RSFD *rsfd;
    int  *more;
    struct it_key **buf;
    RSFD rsfd_result;
    RSET result;
    rset_temp_parms parms;
    
    rsfd = xmalloc (sizeof(*rsfd)*rset_no);
    more = xmalloc (sizeof(*more)*rset_no);
    buf = xmalloc (sizeof(*buf)*rset_no);

    for (i = 0; i<rset_no; i++)
    {
        buf[i] = xmalloc (sizeof(**buf));
        rsfd[i] = rset_open (rset[i], RSETF_READ|RSETF_SORT_SYSNO);
        if (!(more[i] = rset_read (rset[i], rsfd[i], buf[i])))
        {
            while (i >= 0)
            {
                rset_close (rset[i], rsfd[i]);
                xfree (buf[i]);
                --i;
            }
            xfree (rsfd);
            xfree (more);
            xfree (buf);
            return rset_create (rset_kind_null, NULL);
        }
    }
    parms.key_size = sizeof (struct it_key);
    result = rset_create (rset_kind_temp, &parms);
    rsfd_result = rset_open (result, RSETF_WRITE|RSETF_SORT_SYSNO);
    
    while (*more)
    {
        for (i = 1; i<rset_no; i++)
        {
            int cmp;
            
            if (!more[i])
            {
                *more = 0;
                break;
            }
            cmp = key_compare (buf[i], buf[i-1]);
            if (cmp > 1)
            {
                more[i-1] = rset_read (rset[i-1], rsfd[i-1], buf[i-1]);
                break;
            }
            else if (cmp == 1)
            {
                if (buf[i-1]->seqno+1 != buf[i]->seqno)
                {
                    more[i-1] = rset_read (rset[i-1], rsfd[i-1], buf[i-1]);
                    break;
                }
            }
            else
            {
                more[i] = rset_read (rset[i], rsfd[i], buf[i]);
                break;
            }
        }
        if (i == rset_no)
        {
            rset_write (result, rsfd_result, buf[0]);
            more[0] = rset_read (*rset, *rsfd, *buf);
        }
    }
    
    for (i = 0; i<rset_no; i++)
    {
        rset_close (rset[i], rsfd[i]);
        xfree (buf[i]);
    }
    rset_close (result, rsfd_result);
    xfree (buf);
    xfree (more);
    xfree (rsfd);
    return result;
}

static RSET rpn_search_APT_phrase (ZServerInfo *zi,
                                   Z_AttributesPlusTerm *zapt,
                                   oid_value attributeSet,
                                   int num_bases, char **basenames)
{
    char termz[IT_MAX_WORD+1];
    char term_sub[IT_MAX_WORD+1];
    char *p0 = termz, *p1 = NULL;
    RSET rset[60], result;
    int i, rset_no = 0;
    struct grep_info grep_info;

    if (zapt->term->which != Z_Term_general)
    {
        zi->errCode = 124;
        return NULL;
    }
    trans_term (zi, zapt, termz);

    grep_info.isam_p_size = 0;
    grep_info.isam_p_buf = NULL;

    while (1)
    {
        char **map;
        char *p2 = NULL;
        p1 = p0;
        while (*p1)
        {
            map = map_chrs_input (&p1, strlen(p1));
            if (**map == *CHR_SPACE)
                break;
            p2 = p1;
        }
        if (p1 == p0)
            break;
        memcpy (term_sub, p0, p2-p0);
        term_sub[p2-p0] = '\0';
        p0 = p1;
        while (*p1)
        {
            map = map_chrs_input (&p1, strlen(p1));
            if (**map != *CHR_SPACE)
                break;
            p0 = p1;
        }
        grep_info.isam_p_indx = 0;
        if (field_term (zi, zapt, term_sub, 'w', attributeSet, &grep_info,
                        num_bases, basenames))
            return NULL;
        if (grep_info.isam_p_indx == 0)
            rset[rset_no] = rset_create (rset_kind_null, NULL);
        else if (grep_info.isam_p_indx > 1)
            rset[rset_no] = rset_trunc (zi->wordIsam,
                                        grep_info.isam_p_buf,
                                        grep_info.isam_p_indx);
        else
        {
            rset_isam_parms parms;
            
            parms.is = zi->wordIsam;
            parms.pos = *grep_info.isam_p_buf;
            rset[rset_no] = rset_create (rset_kind_isam, &parms);
        }
        assert (rset[rset_no]);
        if (++rset_no >= sizeof(rset)/sizeof(*rset))
            break;
        if (!p1)
            break;
    }
    xfree (grep_info.isam_p_buf);
    if (rset_no == 0)
        return rset_create (rset_kind_null, NULL);
    else if (rset_no == 1)
        return (rset[0]);

    result = rpn_prox (rset, rset_no);
    for (i = 0; i<rset_no; i++)
        rset_delete (rset[i]);
    return result;
}

static RSET rpn_search_APT_local (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
                                  oid_value attributeSet)
{
    RSET result;
    RSFD rsfd;
    struct it_key key;
    rset_temp_parms parms;
    char termz[IT_MAX_WORD+1];

    if (zapt->term->which != Z_Term_general)
    {
        zi->errCode = 124;
        return NULL;
    }
    parms.key_size = sizeof (struct it_key);
    result = rset_create (rset_kind_temp, &parms);
    rsfd = rset_open (result, RSETF_WRITE|RSETF_SORT_SYSNO);

    trans_term (zi, zapt, termz);

    key.sysno = atoi (termz);
    if (key.sysno <= 0)
        key.sysno = 1;
    rset_write (result, rsfd, &key);
    rset_close (result, rsfd);
    return result;
}

static RSET rpn_search_APT (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
                            oid_value attributeSet,
                            int num_bases, char **basenames)
{
    AttrType relation;
    AttrType structure;
    AttrType completeness;
    int relation_value, structure_value, completeness_value;

    attr_init (&relation, zapt, 2);
    attr_init (&structure, zapt, 4);
    attr_init (&completeness, zapt, 6);
    
    relation_value = attr_find (&relation, NULL);
    structure_value = attr_find (&structure, NULL);
    completeness_value = attr_find (&completeness, NULL);
    switch (structure_value)
    {
    case -1:
        if (relation_value == 102) /* relevance relation */
            return rpn_search_APT_relevance (zi, zapt, attributeSet,
                                             num_bases, basenames);
        if (completeness_value == 2 || completeness_value == 3)
            return rpn_search_APT_cphrase (zi, zapt, attributeSet,
                                           num_bases, basenames);
        return rpn_search_APT_phrase (zi, zapt, attributeSet,
                                      num_bases, basenames);
    case 1: /* phrase */
        if (relation_value == 102) /* relevance relation */
            return rpn_search_APT_relevance (zi, zapt, attributeSet,
                                             num_bases, basenames);
        if (completeness_value == 2 || completeness_value == 3)
            return rpn_search_APT_cphrase (zi, zapt, attributeSet,
                                           num_bases, basenames);
        return rpn_search_APT_phrase (zi, zapt, attributeSet,
                                      num_bases, basenames);
        break;
    case 2: /* word */
        if (relation_value == 102) /* relevance relation */
            return rpn_search_APT_relevance (zi, zapt, attributeSet,
                                             num_bases, basenames);
        if (completeness_value == 2 || completeness_value == 3)
            return rpn_search_APT_cphrase (zi, zapt, attributeSet,
                                           num_bases, basenames);
        return rpn_search_APT_phrase (zi, zapt, attributeSet,
                                      num_bases, basenames);
    case 3: /* key */
        break;
    case 4: /* year */
        break;
    case 5: /* date - normalized */
        break;
    case 6: /* word list */
        return rpn_search_APT_relevance (zi, zapt, attributeSet,
                                         num_bases, basenames);
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
        return rpn_search_APT_relevance (zi, zapt, attributeSet,
                                         num_bases, basenames);
    case 106: /* document-text */
        return rpn_search_APT_relevance (zi, zapt, attributeSet,
                                         num_bases, basenames);
    case 107: /* local-number */
        return rpn_search_APT_local (zi, zapt, attributeSet);
    case 108: /* string */ 
        return rpn_search_APT_phrase (zi, zapt, attributeSet,
                                      num_bases, basenames);
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

static RSET rpn_search_structure (ZServerInfo *zi, Z_RPNStructure *zs,
                                  oid_value attributeSet,
                                  int num_bases, char **basenames)
{
    RSET r = NULL;
    if (zs->which == Z_RPNStructure_complex)
    {
        rset_bool_parms bool_parms;
        int soft = 0;

        bool_parms.rset_l = rpn_search_structure (zi, zs->u.complex->s1,
                                                  attributeSet,
                                                  num_bases, basenames);
        if (bool_parms.rset_l == NULL)
            return NULL;
        if (rset_is_ranked(bool_parms.rset_l))
            soft = 1;
        bool_parms.rset_r = rpn_search_structure (zi, zs->u.complex->s2,
                                                  attributeSet,
                                                  num_bases, basenames);
        if (bool_parms.rset_r == NULL)
        {
            rset_delete (bool_parms.rset_l);
            return NULL;
        }
        if (rset_is_ranked(bool_parms.rset_r))
            soft = 1;
        bool_parms.key_size = sizeof(struct it_key);
        bool_parms.cmp = key_compare;

        switch (zs->u.complex->roperator->which)
        {
        case Z_Operator_and:
            r = rset_create (soft ? rset_kind_sand:rset_kind_and, &bool_parms);
            break;
        case Z_Operator_or:
            r = rset_create (soft ? rset_kind_sor:rset_kind_or, &bool_parms);
            break;
        case Z_Operator_and_not:
            r = rset_create (soft ? rset_kind_snot:rset_kind_not, &bool_parms);
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
            r = rpn_search_APT (zi, zs->u.simple->u.attributesPlusTerm,
                                attributeSet, num_bases, basenames);
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

void count_set_save (RSET *r, int *count)
{
    int psysno = 0;
    int kno = 0;
    struct it_key key;
    RSFD rfd, wfd;
    RSET w;
    rset_temp_parms parms;

    logf (LOG_DEBUG, "count_set_save");
    *count = 0;
    parms.key_size = sizeof(struct it_key);
    w = rset_create (rset_kind_temp, &parms);
    wfd = rset_open (w, RSETF_WRITE|RSETF_SORT_SYSNO);
    rfd = rset_open (*r, RSETF_READ|RSETF_SORT_SYSNO);
    while (rset_read (*r, rfd, &key))
    {
        if (key.sysno != psysno)
        {
            rset_write (w, wfd, &key);
            psysno = key.sysno;
            (*count)++;
        }
        kno++;
    }
    rset_close (*r, rfd);
    rset_delete (*r);
    rset_close (w, wfd);
    *r = w;
    logf (LOG_DEBUG, "%d keys, %d distinct sysnos", kno, *count);
}

static void count_set (RSET r, int *count)
{
    int psysno = 0;
    int kno = 0;
    struct it_key key;
    RSFD rfd;

    logf (LOG_DEBUG, "count_set");
    *count = 0;
    rfd = rset_open (r, RSETF_READ|RSETF_SORT_SYSNO);
    while (rset_read (r, rfd, &key))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
            (*count)++;
        }
        kno++;
    }
    rset_close (r, rfd);
    logf (LOG_DEBUG, "%d keys, %d distinct sysnos", kno, *count);
}

int rpn_search (ZServerInfo *zi,
                Z_RPNQuery *rpn, int num_bases, char **basenames, 
                const char *setname, int *hits)
{
    RSET rset;
    oident *attrset;
    oid_value attributeSet;

    dict_grep_cmap (zi->wordDict, map_chrs_input);
    zlog_rpn (rpn);

    zi->errCode = 0;
    zi->errString = NULL;

    attrset = oid_getentbyoid (rpn->attributeSetId);
    attributeSet = attrset->value;
    rset = rpn_search_structure (zi, rpn->RPNStructure, attributeSet,
                                 num_bases, basenames);
    if (!rset)
        return zi->errCode;
    if (rset_is_volatile(rset))
        count_set_save(&rset,hits);
    else
        count_set (rset, hits);
    resultSetAdd (zi, setname, 1, rset);
    if (zi->errCode)
        logf (LOG_DEBUG, "search error: %d", zi->errCode);
    return zi->errCode;
}

struct scan_info_entry {
    char *term;
    ISAM_P isam_p;
};

struct scan_info {
    struct scan_info_entry *list;
    ODR odr;
    int before, after;
    char prefix[20];
};

static int scan_handle (char *name, const char *info, int pos, void *client)
{
    int len_prefix, idx;
    struct scan_info *scan_info = client;

    len_prefix = strlen(scan_info->prefix);
    if (memcmp (name, scan_info->prefix, len_prefix))
        return 1;
    if (pos > 0)
        idx = scan_info->after - pos + scan_info->before;
    else
        idx = - pos - 1;
    logf (LOG_DEBUG, "%-3d %s", idx, name+len_prefix);
    scan_info->list[idx].term = odr_malloc (scan_info->odr,
                                            strlen(name + len_prefix)+1);
    strcpy (scan_info->list[idx].term, name + len_prefix);
    assert (*info == sizeof(ISAM_P));
    memcpy (&scan_info->list[idx].isam_p, info+1, sizeof(ISAM_P));
    return 0;
}


static void scan_term_untrans (ODR odr, char **dstp, const char *src)
{    
    char *dst = odr_malloc (odr, strlen(src)*2+1);
    *dstp = dst;

    while (*src)
    {
        const char *cp = map_chrs_output (&src);
        while (*cp)
            *dst++ = *cp++;
    }
    *dst = '\0';
}

int rpn_scan (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
              oid_value attributeset,
              int num_bases, char **basenames,
              int *position, int *num_entries, struct scan_entry **list,
              int *status)
{
    int i;
    int pos = *position;
    int num = *num_entries;
    int before;
    int after;
    int base_no;
    char termz[IT_MAX_WORD+20];
    AttrType use;
    int use_value;
    AttrType completeness;
    int completeness_value;
    struct scan_info *scan_info_array;
    struct scan_entry *glist;
    int ords[32], ord_no = 0;
    int ptr[32];

    logf (LOG_DEBUG, "scan, position = %d, num = %d", pos, num);

    attr_init (&use, zapt, 1);
    use_value = attr_find (&use, NULL);
    logf (LOG_DEBUG, "use value %d", use_value);

    attr_init (&completeness, zapt, 6);
    completeness_value = attr_find (&completeness, NULL);
    logf (LOG_DEBUG, "completeness value %d", completeness_value);

    if (attributeset == VAL_NONE)
        attributeset = VAL_BIB1;
        
    if (use_value == -1)
        use_value = 1016;
    for (base_no = 0; base_no < num_bases && ord_no < 32; base_no++)
    {
        attent *attp;
        data1_local_attribute *local_attr;

        attp = att_getentbyatt (attributeset, use_value);
        if (!attp)
        {
            logf (LOG_DEBUG, "att_getentbyatt fail. set=%d use=%d",
                  attributeset, use_value);
            return zi->errCode = 114;
        }
        if (zebTargetInfo_curDatabase (zi->zti, basenames[base_no]))
        {
            zi->errString = basenames[base_no];
            return zi->errCode = 109; /* Database unavailable */
        }
        for (local_attr = attp->local_attributes; local_attr && ord_no < 32;
             local_attr = local_attr->next)
        {
            int ord;

            ord = zebTargetInfo_lookupSU (zi->zti, attp->attset_ordinal,
                                          local_attr->local);
            if (ord > 0)
                ords[ord_no++] = ord;
        }
    }
    if (ord_no == 0)
        return zi->errCode = 113;
    before = pos-1;
    after = 1+num-pos;
    scan_info_array = odr_malloc (zi->odr, ord_no * sizeof(*scan_info_array));
    for (i = 0; i < ord_no; i++)
    {
        int j, prefix_len = 0;
        int before_tmp = before, after_tmp = after;
        struct scan_info *scan_info = scan_info_array + i;

        scan_info->before = before;
        scan_info->after = after;
        scan_info->odr = zi->odr;

        scan_info->list = odr_malloc (zi->odr, (before+after)*
                                      sizeof(*scan_info->list));
        for (j = 0; j<before+after; j++)
            scan_info->list[j].term = NULL;
        termz[prefix_len++] = ords[i];
        termz[prefix_len++] =
            (completeness_value==2 || completeness_value==3) ? 'p': 'w';
        termz[prefix_len] = 0;
        strcpy (scan_info->prefix, termz);

        trans_scan_term (zi, zapt, termz+prefix_len);
                    
        dict_scan (zi->wordDict, termz, &before_tmp, &after_tmp, scan_info,
                   scan_handle);
    }
    glist = odr_malloc (zi->odr, (before+after)*sizeof(*glist));
    for (i = 0; i < ord_no; i++)
        ptr[i] = before;
    
    *status = BEND_SCAN_SUCCESS;
    for (i = 0; i<after; i++)
    {
        int j, j0 = -1;
        const char *mterm = NULL;
        const char *tst;
        RSET rset;
        rset_isam_parms parms;
        
        for (j = 0; j < ord_no; j++)
        {
            if (ptr[j] < before+after &&
                (tst=scan_info_array[j].list[ptr[j]].term) &&
                (!mterm || strcmp (tst, mterm) < 0))
            {
                j0 = j;
                mterm = tst;
            }
        }
        if (j0 == -1)
            break;
        scan_term_untrans (zi->odr, &glist[i+before].term, mterm);
        parms.is = zi->wordIsam;
        parms.pos = scan_info_array[j0].list[ptr[j0]].isam_p;
        rset = rset_create (rset_kind_isam, &parms);

        ptr[j0]++;
        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before+after &&
                (tst=scan_info_array[j].list[ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
                rset_isam_parms parms;
                rset_bool_parms bool_parms;
                RSET rset2;

                parms.is = zi->wordIsam;
                parms.pos = scan_info_array[j].list[ptr[j]].isam_p;
                rset2 = rset_create (rset_kind_isam, &parms);

                bool_parms.key_size = sizeof(struct it_key);
                bool_parms.cmp = key_compare;
                bool_parms.rset_l = rset;
                bool_parms.rset_r = rset2;
              
                rset = rset_create (rset_kind_or, &bool_parms);

                ptr[j]++;
            }
        }
        count_set (rset, &glist[i+before].occurrences);
        rset_delete (rset);
    }
    if (i < after)
    {
        *num_entries -= (after-i);
        *status = BEND_SCAN_PARTIAL;
    }

    for (i = 0; i<ord_no; i++)
        ptr[i] = 0;

    for (i = 0; i<before; i++)
    {
        int j, j0 = -1;
        const char *mterm = NULL;
        const char *tst;
        RSET rset;
        rset_isam_parms parms;
        
        for (j = 0; j <ord_no; j++)
        {
            if (ptr[j] < before &&
                (tst=scan_info_array[j].list[before-1-ptr[j]].term) &&
                (!mterm || strcmp (tst, mterm) > 0))
            {
                j0 = j;
                mterm = tst;
            }
        }
        if (j0 == -1)
            break;

        scan_term_untrans (zi->odr, &glist[before-1-i].term, mterm);

        parms.is = zi->wordIsam;
        parms.pos = scan_info_array[j0].list[before-1-ptr[j0]].isam_p;
        rset = rset_create (rset_kind_isam, &parms);

        ptr[j0]++;

        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before &&
                (tst=scan_info_array[j].list[before-1-ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
                rset_isam_parms parms;
                rset_bool_parms bool_parms;
                RSET rset2;

                parms.is = zi->wordIsam;
                parms.pos = scan_info_array[j].list[before-1-ptr[j]].isam_p;
                rset2 = rset_create (rset_kind_isam, &parms);

                bool_parms.key_size = sizeof(struct it_key);
                bool_parms.cmp = key_compare;
                bool_parms.rset_l = rset;
                bool_parms.rset_r = rset2;
              
                rset = rset_create (rset_kind_or, &bool_parms);

                ptr[j]++;
            }
        }
        count_set (rset, &glist[before-1-i].occurrences);
        rset_delete (rset);
    }
    i = before-i;
    if (i)
    {
        *status = BEND_SCAN_PARTIAL;
        *position -= i;
        *num_entries -= i;
    }
    *list = glist + i;               /* list is set to first 'real' entry */
    
    logf (LOG_DEBUG, "position = %d, num_entries = %d",
          *position, *num_entries);
    if (zi->errCode)
        logf (LOG_DEBUG, "scan error: %d", zi->errCode);
    return zi->errCode;
}
              
