/*
 * Copyright (C) 1996-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: rsisamc.h,v 1.5 1999-05-12 13:08:06 adam Exp $
 */

#ifndef RSET_ISAMC_H
#define RSET_ISAMC_H

#include <rset.h>
#include <isamc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_isamc;

typedef struct rset_isamc_parms
{
    ISAMC is;
    ISAMC_P pos;
    RSET_TERM rset_term;
} rset_isamc_parms;

#ifdef __cplusplus
}
#endif

#endif
