/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisam.h,v $
 * Revision 1.2  1995-09-04 09:09:53  adam
 * String arg in dict lookup is const.
 * Minor changes.
 *
 * Revision 1.1  1994/11/04  13:21:23  quinn
 * Working.
 *
 */

#ifndef RSET_ISAM_H
#define RSET_ISAM_H

#include <rset.h>
#include <isam.h>

extern const rset_control *rset_kind_isam;

typedef struct rset_isam_parms
{
    ISAM is;
    ISAM_P pos;
} rset_isam_parms;

#endif
