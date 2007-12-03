/* $Id: rset_isam.c,v 1.1 2007-12-03 13:04:04 adam Exp $
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

/** \file
    \brief factory function for RSET from ISAM 
*/
#include <stdio.h>
#include <assert.h>

#include "index.h"
#include <rset.h>

RSET zebra_create_rset_isam(ZebraHandle zh,
                            NMEM rset_nmem, struct rset_key_control *kctrl,
                            int scope, ISAM_P pos, TERMID termid)
{
    assert(zh);
    assert(zh->reg);
    if (zh->reg->isamb)
        return rsisamb_create(rset_nmem, kctrl, scope,
                              zh->reg->isamb, pos, termid);
    else if (zh->reg->isams)
        return rsisams_create(rset_nmem, kctrl, scope,
                              zh->reg->isams, pos, termid);
    else if (zh->reg->isamc)
        return rsisamc_create(rset_nmem, kctrl, scope,
                              zh->reg->isamc, pos, termid);
    else
	return rset_create_null(rset_nmem, kctrl, termid);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

