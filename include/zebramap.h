/* $Id: zebramap.h,v 1.23 2007-10-29 16:57:51 adam Exp $
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

#ifndef ZEBRAMAP_H
#define ZEBRAMAP_H

#include <yaz/proto.h>
#include <idzebra/res.h>

YAZ_BEGIN_CDECL

typedef struct zebra_maps *ZebraMaps;

YAZ_EXPORT
ZebraMaps zebra_maps_open (Res res, const char *base_path,
			   const char *profile_path);
YAZ_EXPORT
ZEBRA_RES zebra_maps_read_file(ZebraMaps zms, const char *fname);

YAZ_EXPORT
void zebra_maps_close (ZebraMaps zm);

YAZ_EXPORT
const char **zebra_maps_input (ZebraMaps zms, unsigned reg_id,
			       const char **from, int len, int first);

YAZ_EXPORT
const char **zebra_maps_search (ZebraMaps zms, unsigned reg_id,
				const char **from, int len, int *q_map_match);

YAZ_EXPORT
const char *zebra_maps_output(ZebraMaps, unsigned reg_id, const char **from);

YAZ_EXPORT
int zebra_maps_attr (ZebraMaps zms, Z_AttributesPlusTerm *zapt,
		     const char **reg_id, char **search_type, char *rank_type,
		     int *complete_flag, int *sort_flag);

YAZ_EXPORT
int zebra_maps_sort (ZebraMaps zms, Z_SortAttributes *sortAttributes,
                     int *numerical);

YAZ_EXPORT
int zebra_maps_is_complete (ZebraMaps zms, unsigned reg_id);

YAZ_EXPORT
int zebra_maps_is_sort (ZebraMaps zms, unsigned reg_id);

YAZ_EXPORT
int zebra_maps_is_index (ZebraMaps zms, unsigned reg_id);

YAZ_EXPORT
int zebra_maps_is_staticrank (ZebraMaps zms, unsigned reg_id);

YAZ_EXPORT
int zebra_maps_is_alwaysmatches (ZebraMaps zms, unsigned reg_id);

YAZ_EXPORT
int zebra_maps_is_positioned (ZebraMaps zms, unsigned reg_id);

YAZ_EXPORT
int zebra_maps_is_first_in_field (ZebraMaps zms, unsigned reg_id);

YAZ_EXPORT
WRBUF zebra_replace(ZebraMaps zms, unsigned reg_id, const char *ex_list,
		    const char *input_str, int input_len);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

