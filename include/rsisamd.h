/*
 * Copyright (C) 1996-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, 
 *
 * $Id: rsisamd.h,v 1.2 2002-07-12 18:12:22 heikki Exp $
*/

#ifndef RSET_ISAMD_H
#define RSET_ISAMD_H

#include <rset.h>
#include <isamd.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_isamd;

typedef struct rset_isamd_parms
{
    ISAMD is;
/*    ISAMD_P pos; */
    char dictentry[ISAMD_MAX_DICT_LEN+1];
    int dictlen;
    RSET_TERM rset_term;
} rset_isamd_parms;

#ifdef __cplusplus
}
#endif

#endif
