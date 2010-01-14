/* This file is part of the Zebra server.
   Copyright (C) 1994-2010 Index Data

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

typedef struct zebra_maps_s *zebra_maps_t;
typedef struct zebra_map *zebra_map_t;

YAZ_EXPORT
zebra_maps_t zebra_maps_open(Res res, const char *base_path,
                          const char *profile_path);
YAZ_EXPORT
ZEBRA_RES zebra_maps_read_file(zebra_maps_t zms, const char *fname);

YAZ_EXPORT
void zebra_maps_define_default_sort(zebra_maps_t zms);

YAZ_EXPORT
void zebra_maps_close(zebra_maps_t zm);

YAZ_EXPORT
const char **zebra_maps_input(zebra_map_t zm,
                              const char **from, int len, int first);

YAZ_EXPORT
const char **zebra_maps_search(zebra_map_t zm,
                               const char **from, int len, int *q_map_match);

YAZ_EXPORT
const char *zebra_maps_output(zebra_map_t zm, const char **from);

YAZ_EXPORT
int zebra_maps_attr(zebra_maps_t zms, Z_AttributesPlusTerm *zapt,
                    const char **reg_id, char **search_type, char *rank_type,
                    int *complete_flag, int *sort_flag);

YAZ_EXPORT
int zebra_maps_sort(zebra_maps_t zms, Z_SortAttributes *sortAttributes,
                    int *numerical);

YAZ_EXPORT
int zebra_maps_is_complete(zebra_map_t zm);

YAZ_EXPORT
int zebra_maps_is_sort(zebra_map_t zm);

YAZ_EXPORT
int zebra_maps_is_index(zebra_map_t zm);

YAZ_EXPORT
int zebra_maps_is_staticrank(zebra_map_t zm);

YAZ_EXPORT
int zebra_maps_is_alwaysmatches(zebra_map_t zm);

YAZ_EXPORT
int zebra_maps_is_positioned(zebra_map_t zm);

YAZ_EXPORT
int zebra_maps_is_icu(zebra_map_t zm);

YAZ_EXPORT
int zebra_maps_is_first_in_field(zebra_map_t zm);

YAZ_EXPORT
WRBUF zebra_replace(zebra_map_t zm, const char *ex_list,
		    const char *input_str, int input_len);

YAZ_EXPORT
zebra_map_t zebra_map_get(zebra_maps_t zms, const char *id);

YAZ_EXPORT
zebra_map_t zebra_map_get_or_add(zebra_maps_t zms, const char *id);

YAZ_EXPORT
int zebra_map_tokenize_start(zebra_map_t zm,
                             const char *buf, size_t len);

YAZ_EXPORT
int zebra_map_tokenize_next(zebra_map_t zm,
                            const char **result_buf, size_t *result_len,
                            const char **display_buf, size_t *display_len);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

