/*
 * Copyright (C) 1995-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsrel.h,v $
 * Revision 1.7  1998-01-07 13:53:41  adam
 * Queries using simple ranked operands returns right number of hits.
 *
 * Revision 1.6  1997/12/18 10:54:24  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.5  1997/09/22 12:39:06  adam
 * Added get_pos method for the ranked result sets.
 *
 * Revision 1.4  1997/09/05 15:30:05  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.3  1996/11/08 11:08:02  adam
 * New internal release.
 *
 * Revision 1.2  1996/06/11 10:53:16  quinn
 * Relevance work.
 *
 * Revision 1.1  1995/09/08  14:52:09  adam
 * Work on relevance sets.
 *
 */

#ifndef RSET_REL_H
#define RSET_REL_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const rset_control *rset_kind_relevance;

#define RSREL_METHOD_A  1
#define RSREL_METHOD_B  2

typedef struct rset_relevance_parms
{
    int     key_size;
    int     max_rec;
    int     (*cmp)(const void *p1, const void *p2);

    ISAM    is;
    ISAMC   isc;
    ISAM_P  *isam_positions;            /* positions */
    int     no_isam_positions;          /* no of positions (no of ISAM_P) */
    int     no_terms;                   /* no of terms */
    int     *term_no;                   /* which term at isam_position */
    int     (*get_pos)(const void *p);

    int     method;
} rset_relevance_parms;

#ifdef __cplusplus
}
#endif

#endif
