/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisam.h,v $
 * Revision 1.4  1998-03-05 08:37:44  adam
 * New result set model.
 *
 * Revision 1.3  1997/09/05 15:30:04  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.2  1995/09/04 09:09:53  adam
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

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_isam;

typedef struct rset_isam_parms
{
    ISAM is;
    ISAM_P pos;
    RSET_TERM rset_term;
} rset_isam_parms;

#ifdef __cplusplus
}
#endif

#endif
