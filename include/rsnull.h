/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsnull.h,v $
 * Revision 1.4  1999-02-02 14:50:42  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.3  1998/03/05 08:37:44  adam
 * New result set model.
 *
 * Revision 1.2  1997/09/05 15:30:05  adam
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

typedef struct rset_null_parms
{
    int     key_size;
    char   *temp_path;
    RSET_TERM rset_term;    
} rset_null_parms;

extern const struct rset_control *rset_kind_null;

#ifdef __cplusplus
}
#endif

#endif
