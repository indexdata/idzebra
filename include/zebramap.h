/* $Id: zebramap.h,v 1.17 2004-09-14 14:38:07 quinn Exp $
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

#ifndef ZEBRAMAP_H
#define ZEBRAMAP_H

#include <yaz/proto.h>
#include <idzebra/res.h>

YAZ_BEGIN_CDECL

typedef struct zebra_maps *ZebraMaps;
ZebraMaps zebra_maps_open (Res res, const char *base);

void zebra_maps_close (ZebraMaps zm);

const char **zebra_maps_input (ZebraMaps zms, unsigned reg_id,
			       const char **from, int len, int first);
const char *zebra_maps_output(ZebraMaps, unsigned reg_id, const char **from);

int zebra_maps_attr (ZebraMaps zms, Z_AttributesPlusTerm *zapt,
		     unsigned *reg_id, char **search_type, char *rank_type,
		     int *complete_flag, int *sort_flag);

int zebra_maps_sort (ZebraMaps zms, Z_SortAttributes *sortAttributes,
                     int *numerical);

int zebra_maps_is_complete (ZebraMaps zms, unsigned reg_id);
int zebra_maps_is_sort (ZebraMaps zms, unsigned reg_id);
int zebra_maps_is_positioned (ZebraMaps zms, unsigned reg_id);

WRBUF zebra_replace(ZebraMaps zms, unsigned reg_id, const char *ex_list,
		    const char *input_str, int input_len);

YAZ_END_CDECL

#endif
