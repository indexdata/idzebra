/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rstemp.h,v $
 * Revision 1.7  2002-03-20 20:24:29  adam
 * Hits per term. Returned in SearchResult-1
 *
 * Revision 1.6  1999/02/02 14:50:43  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.5  1998/03/05 08:37:44  adam
 * New result set model.
 *
 * Revision 1.4  1997/09/17 12:19:11  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.3  1997/09/05 15:30:05  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.2  1995/09/04 15:20:13  adam
 * More work on temp sets. is_open member removed.
 *
 * Revision 1.1  1994/11/04  13:21:23  quinn
 * Working.
 *
 */

#ifndef RSET_TEMP_H
#define RSET_TEMP_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct rset_control *rset_kind_temp;

typedef struct rset_temp_parms
{
    int (*cmp)(const void *p1, const void *p2);
    int     key_size;
    char   *temp_path;
    RSET_TERM rset_term;    
} rset_temp_parms;

#ifdef __cplusplus
}
#endif

#endif
