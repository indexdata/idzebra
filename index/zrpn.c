/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zrpn.c,v $
 * Revision 1.8  1995-09-08 14:52:27  adam
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

static RSET rpn_search_APT (ZServerInfo *zi, Z_AttributesPlusTerm *zapt)
{
    char termz[IT_MAX_WORD+1];
    size_t sizez;
    struct rset_isam_parms parms;
    const char *info;
    int i;
    Z_Term *term = zapt->term;

    if (term->which != Z_Term_general)
        return NULL; 
    sizez = term->u.general->len;
    if (sizez > IT_MAX_WORD)
        sizez = IT_MAX_WORD;
    for (i = 0; i<sizez; i++)
        termz[i] = index_char_cvt (term->u.general->buf[i]);
    termz[i] = '\0';
    logf (LOG_DEBUG, "dict_lookup: %s", termz);
    if (!(info = dict_lookup (zi->wordDict, termz)))
        return rset_create (rset_kind_null, NULL);
    assert (*info == sizeof(parms.pos));
    memcpy (&parms.pos, info+1, sizeof(parms.pos));
    parms.is = zi->wordIsam;
    logf (LOG_DEBUG, "rset_create isam");
    return rset_create (rset_kind_isam, &parms);
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
        bool_parms.rset_r = rpn_search_structure (zi, zs->u.complex->s2);
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

static RSET rpn_save_set (RSET r, int *count)
{
#if 0
    RSET d;
    rset_temp_parms parms;
#endif
    int psysno = 0;
    struct it_key key;
    RSFD rfd;

    logf (LOG_DEBUG, "rpn_save_set");
    *count = 0;
#if 0
    parms.key_size = sizeof(struct it_key);
    d = rset_create (rset_kind_temp, &parms);
    rset_open (d, 1);
#endif

    rfd = rset_open (r, 0);
    while (rset_read (r, rfd, &key))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
            (*count)++;
        }
#if 0
        rset_write (d, &key);
#endif
    }
    rset_close (r, rfd);
#if 0
    rset_close (d);
#endif
    logf (LOG_DEBUG, "%d distinct sysnos", *count);
#if 0
    return d;
#endif
}

int rpn_search (ZServerInfo *zi,
                Z_RPNQuery *rpn, int num_bases, char **basenames, 
                const char *setname, int *hits)
{
    RSET rset, result_rset;

    rset = rpn_search_structure (zi, rpn->RPNStructure);
    if (!rset)
        return 0;
    result_rset = rpn_save_set (rset, hits);
#if 0
    rset_delete (result_rset);
#endif

    resultSetAdd (zi, setname, 1, rset);
    return 0;
}

