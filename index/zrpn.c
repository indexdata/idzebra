/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zrpn.c,v $
 * Revision 1.4  1995-09-05 15:28:40  adam
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

static RSET rpn_search_APT (ZServerInfo *zi, Z_AttributesPlusTerm *zapt)
{
    struct rset_isam_parms parms;
    const char *info;
    Z_Term *term = zapt->term;

    if (term->which != Z_Term_general)
        return NULL; 
    logf (LOG_DEBUG, "dict_lookup: %s", term->u.general->buf);    
    if (!(info = dict_lookup (zi->wordDict, term->u.general->buf)))
    {
        rset_temp_parms parms;

        parms.key_size = sizeof(struct it_key);
        return rset_create (rset_kind_temp, &parms);
    }
    assert (*info == sizeof(parms.pos));
    memcpy (&parms.pos, info+1, sizeof(parms.pos));
    parms.is = zi->wordIsam;
    logf (LOG_DEBUG, "rset_create isam");
    return rset_create (rset_kind_isam, &parms);
}

static RSET rpn_search_and (ZServerInfo *zi, RSET r_l, RSET r_r)
{
    struct it_key k1, k2;
    RSET r_dst;
    int i1, i2;
    rset_open (r_l, 0);
    rset_open (r_r, 0);
    r_dst = rset_create (rset_kind_temp, NULL);
    rset_open (r_dst, 1);
    
    i1 = rset_read (r_l, &k1);
    i2 = rset_read (r_r, &k2);
    while (i1 && i2)
    {
        if (k1.sysno > k2.sysno)
            i2 = rset_read (r_r, &k2);
        else if (k1.sysno < k2.sysno)
            i1 = rset_read (r_l, &k1);
        else if (!(i1 = key_compare_x (&k1, &k2)))
        {
            rset_write (r_dst, &k1);
            i1 = rset_read (r_l, &k1);
            i2 = rset_read (r_r, &k2);
        }
        else if (i1 > 0)
        {
            rset_write (r_dst, &k2);
            i2 = rset_read (r_r, &k2);
        }
        else
        {
            rset_write (r_dst, &k1);
            i1 = rset_read (r_l, &k1);
        }
    } 
    rset_close (r_dst);
    return r_dst;
}

static RSET rpn_search_or (ZServerInfo *zi, RSET r_l, RSET r_r)
{
    return r_l;
}

static RSET rpn_search_not (ZServerInfo *zi, RSET r_l, RSET r_r)
{
    return r_l;
}

static RSET rpn_search_ref (ZServerInfo *zi, Z_ResultSetId *resultSetId)
{
    return NULL;
}

static RSET rpn_search_structure (ZServerInfo *zi, Z_RPNStructure *zs)
{
    RSET r;
    if (zs->which == Z_RPNStructure_complex)
    {
        RSET r_l, r_r;

        r_l = rpn_search_structure (zi, zs->u.complex->s1);
        r_r = rpn_search_structure (zi, zs->u.complex->s2);

        switch (zs->u.complex->operator->which)
        {
        case Z_Operator_and:
            rset_delete (r_r);
            break;
        case Z_Operator_or:
            rset_delete (r_r);
            break;
        case Z_Operator_and_not:
            rset_delete (r_r);
            break;
        default:
            assert (0);
        }
        r = r_l;
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

    logf (LOG_DEBUG, "rpn_save_set");
    *count = 0;
#if 0
    parms.key_size = sizeof(struct it_key);
    d = rset_create (rset_kind_temp, &parms);
    rset_open (d, 1);
#endif

    rset_open (r, 0);
    while (rset_read (r, &key))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
            (*count)++;
        }
        logf (LOG_DEBUG, "lllllllllllllllll");
#if 0
        rset_write (d, &key);
#endif
    }
    rset_close (r);
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

