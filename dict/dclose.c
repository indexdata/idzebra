/* $Id: dclose.c,v 1.12 2007-01-15 15:10:15 adam Exp $
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



#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "dict-p.h"

int dict_bf_close (Dict_BFile dbf)
{
    dict_bf_flush_blocks (dbf, -1);
    
    xfree (dbf->all_blocks);
    xfree (dbf->all_data);
    xfree (dbf->hash_array);
    bf_close(dbf->bf);
    xfree (dbf);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

