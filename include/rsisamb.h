/*
 * Copyright (C) 2001-2002, Index Data
 * All rights reserved.
 *
 * $Id: rsisamb.h,v 1.1 2002-04-16 22:31:42 adam Exp $
 */

#ifndef RSET_ISAMB_H
#define RSET_ISAMB_H

#include <rset.h>
#include <isamb.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_isamb;

typedef struct rset_isamb_parms
{
    int (*cmp)(const void *p1, const void *p2);
    int key_size;
    ISAMB is;
    ISAMB_P pos;
    RSET_TERM rset_term;
} rset_isamb_parms;

#ifdef __cplusplus
}
#endif

#endif
