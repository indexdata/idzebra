/*
 * Copyright (C) 1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisamc.h,v $
 * Revision 1.1  1996-10-29 13:41:33  adam
 * First use of isamc.
 *
 */

#ifndef RSET_ISAMC_H
#define RSET_ISAMC_H

#include <rset.h>
#include <isamc.h>

extern const rset_control *rset_kind_isamc;

typedef struct rset_isamc_parms
{
    ISAMC is;
    ISAMC_P pos;
} rset_isamc_parms;

#endif
