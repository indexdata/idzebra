/* $Id: rsm_or.h,v 1.7 2004-08-04 08:35:23 adam Exp $
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



#ifndef RSET_M_OR_H
#define RSET_M_OR_H

#include <rset.h>

YAZ_BEGIN_CDECL

extern const struct rset_control *rset_kind_m_or;

typedef struct rset_m_or_parms
{
    int     key_size;
    int     (*cmp)(const void *p1, const void *p2);

    ISAMC   isc;
    ISAMC_P  *isam_positions;
    RSET_TERM rset_term;

    int     no_isam_positions;
    int     no_save_positions;
} rset_m_or_parms;

YAZ_END_CDECL

#endif