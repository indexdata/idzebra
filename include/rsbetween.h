/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, Heikki Levanto
 *
 * $Id: rsbetween.h,v 1.1 2002-04-09 15:24:13 heikki Exp $
 *
 * Result set that returns anything in between two things,
 * typically start-tag, stuff, end-tag.
 *
 */

#ifndef RSET_BETWEEN_H
#define RSET_BETWEEN_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_between;

typedef struct rset_between_parms
{
    int     key_size;
    RSET    rset_l; 
    RSET    rset_m;
    RSET    rset_r;
    int (*cmp)(const void *p1, const void *p2);
} rset_between_parms;

#ifdef __cplusplus
}
#endif

#endif
