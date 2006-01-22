/* $Id: orddict.h,v 1.1 2006-01-19 13:31:08 adam Exp $
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

#ifndef ORDDICT_H
#define ORDDICT_H

#include <yaz/yconfig.h>
#include <idzebra/dict.h>

YAZ_BEGIN_CDECL


typedef struct zebra_ord_dict *Zebra_ord_dict;

Zebra_ord_dict zebra_ord_dict_open(Dict s);
void zebra_ord_dict_close(Zebra_ord_dict zod);
char *zebra_ord_dict_lookup (Zebra_ord_dict zod, int ord, 
			     const char *p);

YAZ_END_CDECL
#endif