/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsrel.h,v $
 * Revision 1.3  1996-11-08 11:08:02  adam
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

extern const rset_control *rset_kind_relevance;

typedef struct rset_relevance_parms
{
    int     key_size;
    int     max_rec;
    int     (*cmp)(const void *p1, const void *p2);

    ISAM    is;
    ISAMC   isc;
    ISAM_P  *isam_positions;

    int     no_isam_positions;
    int no_terms;
    int *term_no;
} rset_relevance_parms;

#endif
