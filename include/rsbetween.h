/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, Heikki Levanto
 *
 * $Id: rsbetween.h,v 1.2 2002-04-12 14:51:34 heikki Exp $
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
    char* (*printer)(void *p,char *buf);  /* prints p into buf and returns buf */
} rset_between_parms;

#ifdef __cplusplus
}
#endif

#endif
