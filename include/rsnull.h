/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsnull.h,v $
 * Revision 1.2  1997-09-05 15:30:05  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.1  1995/09/06 10:36:16  adam
 * Null set implemented.
 *
 */

#ifndef RSET_NULL_H
#define RSET_NULL_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const rset_control *rset_kind_null;

#ifdef __cplusplus
}
#endif

#endif
