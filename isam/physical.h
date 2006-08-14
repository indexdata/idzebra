/* $Id: physical.h,v 1.6.2.1 2006-08-14 10:39:03 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#ifndef PHYSICAL_H
#define PHYSICAL_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void is_p_sync(is_mtable *tab);
void is_p_unmap(is_mtable *tab);
void is_p_align(is_mtable *tab);
void is_p_remap(is_mtable *tab);
int is_p_read_partial(is_mtable *tab, is_mblock *block);
int is_p_read_full(is_mtable *tab, is_mblock *block);

#ifdef __cplusplus
}
#endif


#endif
