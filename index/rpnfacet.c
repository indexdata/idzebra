/* $Id: rpnfacet.c,v 1.1 2007-11-01 14:56:07 adam Exp $
   Copyright (C) 1995-2007
   Index Data ApS

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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <assert.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#include <yaz/diagbib1.h>
#include "index.h"
#include <zebra_xpath.h>
#include <yaz/wrbuf.h>
#include <attrfind.h>
#include <charmap.h>
#include <rset.h>
#include <yaz/oid_db.h>

ZEBRA_RES rpn_facet(ZebraHandle zh, ODR stream, NMEM nmem,
                    struct rset_key_control *kc,
                    Z_AttributesPlusTerm *zapt,
                    int *position, int *num_entries, 
                    ZebraScanEntry **list,
                    int *is_partial, RSET limit_set,
                    const char *index_type,
                    int ord_no, int *ords)
{
    /* for each ord .. */
    /*   check that sort idx exist for ord */
    /*   sweep through result set and sort_idx at the same time */
    zebra_setError(zh, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, 
                   "facet not implemented");
                
    return ZEBRA_FAIL;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

