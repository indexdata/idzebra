/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsm_or.h,v $
 * Revision 1.5  1999-02-02 14:50:41  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.4  1998/03/05 08:37:44  adam
 * New result set model.
 *
 * Revision 1.3  1997/09/05 15:30:04  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.2  1996/12/23 15:29:54  adam
 * More work on truncation algorithm.
 *
 * Revision 1.1  1996/12/20 11:06:45  adam
 * Implemented multi-or result set.
 *
 *
 */

#ifndef RSET_M_OR_H
#define RSET_M_OR_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_m_or;

typedef struct rset_m_or_parms
{
    int     key_size;
    int     (*cmp)(const void *p1, const void *p2);

    ISAMC   isc;
    ISAM_P  *isam_positions;
    RSET_TERM rset_term;

    int     no_isam_positions;
    int     no_save_positions;
} rset_m_or_parms;

#ifdef __cplusplus
}
#endif

#endif
