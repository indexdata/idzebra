/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsm_or.h,v $
 * Revision 1.1  1996-12-20 11:06:45  adam
 * Implemented multi-or result set.
 *
 *
 */

#ifndef RSET_M_OR_H
#define RSET_M_OR_H

#include <rset.h>

extern const rset_control *rset_kind_m_or;

typedef struct rset_m_or_parms
{
    int     key_size;
    int     (*cmp)(const void *p1, const void *p2);

    ISAMC   isc;
    ISAM_P  *isam_positions;

    int     no_isam_positions;
} rset_m_or_parms;

#endif
