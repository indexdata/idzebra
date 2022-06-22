/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#ifndef CHARMAP_H
#define CHARMAP_H

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

YAZ_EXPORT extern const char *CHR_UNKNOWN;
YAZ_EXPORT extern const char *CHR_SPACE;
YAZ_EXPORT extern const char *CHR_CUT;
YAZ_EXPORT extern const char *CHR_BASE;

/* defines first char we map to (0, 1, .. are specials) */
#define CHR_BASE_CHAR  5

struct chr_t_entry;
typedef struct chr_t_entry chr_t_entry;

typedef struct chrmaptab_info *chrmaptab;

YAZ_EXPORT chrmaptab chrmaptab_create(const char *tabpath, const char *name,
                                      const char *tabroot);
YAZ_EXPORT void chrmaptab_destroy (chrmaptab tab);

YAZ_EXPORT const char **chr_map_input(chrmaptab t, const char **from, int len, int first);
YAZ_EXPORT const char **chr_map_input_x(chrmaptab t,
                                        const char **from, int *len, int first);
YAZ_EXPORT const char **chr_map_q_input(chrmaptab maptab,
                                        const char **from, int len, int first);

YAZ_EXPORT const char *chr_map_output(chrmaptab t, const char **from, int len);

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

