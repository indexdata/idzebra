/* $Id: rootblk.c,v 1.4 2002-08-02 19:26:56 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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



/*
 * Read and write the blocktype header.
 */

#include <stdio.h>
#include <isam.h>
#include "rootblk.h"

int is_rb_write(isam_blocktype *ib, is_type_header *hd)
{
    int pt = 0, ct = 0, towrite;

    while ((towrite = sizeof(*hd) - pt) > 0)
    {
    	if (towrite > bf_blocksize(ib->bf))
    		towrite = bf_blocksize(ib->bf);
    	if (bf_write(ib->bf, ct, 0, towrite, (char *)hd + pt) < 0)
	    return -1;
	pt += bf_blocksize(ib->bf);
	ct++;
    }
    return ct;
}

int is_rb_read(isam_blocktype *ib, is_type_header *hd)
{
    int pt = 0, ct = 0, rs, toread;

    while ((toread = sizeof(*hd) - pt) > 0)
    {
	if (toread > bf_blocksize(ib->bf))
	    toread = bf_blocksize(ib->bf);
    	if ((rs = bf_read(ib->bf, ct, 0, toread, (char*)hd + pt)) <= 0)
	    return rs;
	pt += bf_blocksize(ib->bf);
	ct++;
    }
    return ct;
}
