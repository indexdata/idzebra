/* $Id: dclose.c,v 1.8 2005-01-15 19:38:21 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "dict-p.h"

int dict_bf_close (Dict_BFile dbf)
{
    int i;
    dict_bf_flush_blocks (dbf, -1);
    
    xfree (dbf->all_blocks);
    xfree (dbf->all_data);
    xfree (dbf->hash_array);
    i = bf_close (dbf->bf);
    xfree (dbf);
    return i;
}
