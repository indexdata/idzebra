/*
 * Copyright (C) 1996-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: rsisams.h,v 1.1 1999-05-12 15:24:25 adam Exp $
 */

#ifndef RSET_ISAMS_H
#define RSET_ISAMS_H

#include <rset.h>
#include <isams.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_isams;

typedef struct rset_isams_parms
{
    ISAMS is;
    ISAMS_P pos;
    RSET_TERM rset_term;
} rset_isams_parms;

#ifdef __cplusplus
}
#endif

#endif
