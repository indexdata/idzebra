/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsbool.h,v $
 * Revision 1.1  1995-09-06 13:27:37  adam
 * New set type: bool. Not finished yet.
 *
 */

#ifndef RSET_BOOL_H
#define RSET_BOOL_H

#include <rset.h>

extern const rset_control *rset_kind_bool;

typedef struct rset_bool_parms
{
    int     key_size;
    int     op;
} rset_bool_parms;

#endif
