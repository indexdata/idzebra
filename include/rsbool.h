/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsbool.h,v $
 * Revision 1.5  1997-09-05 15:30:02  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.4  1995/12/11 09:07:53  adam
 * New rset member 'flag', that holds various flags about a result set -
 * currently 'volatile' (set is register dependent) and 'ranked' (set is
 * ranked).
 * New set types sand/sor/snot. They handle and/or/not for ranked and
 * semi-ranked result sets.
 *
 * Revision 1.3  1995/09/07  13:58:08  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.2  1995/09/06  16:10:57  adam
 * More work on boolean sets.
 *
 * Revision 1.1  1995/09/06  13:27:37  adam
 * New set type: bool. Not finished yet.
 *
 */

#ifndef RSET_BOOL_H
#define RSET_BOOL_H

#include <rset.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const rset_control *rset_kind_and;
extern const rset_control *rset_kind_or;
extern const rset_control *rset_kind_not;

extern const rset_control *rset_kind_sand;
extern const rset_control *rset_kind_sor;
extern const rset_control *rset_kind_snot;

typedef struct rset_bool_parms
{
    int     key_size;
    RSET    rset_l;
    RSET    rset_r;
    int (*cmp)(const void *p1, const void *p2);
} rset_bool_parms;

#ifdef __cplusplus
}
#endif

#endif
