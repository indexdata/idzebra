/*
 * Copyright (C) 1996-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisamc.h,v $
 * Revision 1.3  1998-03-05 08:37:44  adam
 * New result set model.
 *
 * Revision 1.2  1997/09/05 15:30:04  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.1  1996/10/29 13:41:33  adam
 * First use of isamc.
 *
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
