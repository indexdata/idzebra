/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsrel.h,v $
 * Revision 1.1  1995-09-08 14:52:09  adam
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

    ISAM is;
    ISAM_P  *isam_positions;
    int     no_isam_positions;
} rset_relevance_parms;

#endif
