/* $Id: rsmultior.h,v 1.3 2004-08-20 14:44:45 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#ifndef RSET_MULTIOR_H
#define RSET_MULTIOR_H

#include <rset.h>

YAZ_BEGIN_CDECL

extern const struct rset_control *rset_kind_multior;

typedef struct rset_multior_parms
{
    int     key_size;
    int     (*cmp)(const void *p1, const void *p2);

    int no_rsets;
    RSET *rsets;  /* array of rsets to multi-or */
                  /* allocated when creating parms, */
                  /* rset will free when deleted */
    int     no_isam_positions;
    int     no_save_positions;
} rset_multior_parms;

YAZ_END_CDECL

#endif
