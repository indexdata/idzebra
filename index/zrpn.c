/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zrpn.c,v $
 * Revision 1.1  1995-09-04 09:10:40  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <util.h>
#include <dict.h>
#include <isam.h>
#include <rsisam.h>
#include <rstemp.h>

#include <proto.h>

#include "index.h"

static Dict dict;
static ISAM isam;

static RSET rpn_search_APT (Z_AttributesPlusTerm *zapt)
{
    struct rset_isam_parms parms;
    const char *info;
    Z_Term *term = zapt->term;

    if (term->which != Z_Term_general)
        return NULL; 
    if (!(info = dict_lookup (dict, term->u.general->buf)))
        return NULL;
    assert (*info == sizeof(parms.pos));
    memcpy (&parms.pos, info+1, sizeof(parms.pos));
    parms.is = isam;
    return rset_create (rset_kind_isam, &parms);
}

static RSET rpn_search_and (RSET r_l, RSET r_r)
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

static RSET rpn_search_or (RSET r_l, RSET r_r)
{
    return r_l;
}

static RSET rpn_search_not (RSET r_l, RSET r_r)
{
    return r_l;
}

static RSET rpn_search_ref (Z_ResultSetId *resultSetId)
{
    return NULL;
}

static RSET rpn_search_structure (Z_RPNStructure *zs)
{
    RSET r;
    if (zs->which == Z_RPNStructure_complex)
    {
        RSET r_l, r_r;

        r_l = rpn_search_structure (zs->u.complex->s1);
        r_r = rpn_search_structure (zs->u.complex->s2);

        switch (zs->u.complex->operator->which)
        {
        case Z_Operator_and:
            r = rpn_search_and (r_l, r_r);
            break;
        case Z_Operator_or:
            r = rpn_search_or (r_l, r_r);
            break;
        case Z_Operator_and_not:
            r = rpn_search_not (r_l, r_r);
            break;
        default:
            assert (0);
        }
        rset_delete (r_l);
        rset_delete (r_r);
    }
    else if (zs->which == Z_RPNStructure_simple)
    {
        if (zs->u.simple->which == Z_Operand_APT)
            r = rpn_search_APT (zs->u.simple->u.attributesPlusTerm);
        else if (zs->u.simple->which == Z_Operand_resultSetId)
            r = rpn_search_ref (zs->u.simple->u.resultSetId);
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
