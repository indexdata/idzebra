/* $Id: charmap.h,v 1.9.2.3 2006-08-14 10:38:55 adam Exp $
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

struct chr_t_entry;
typedef struct chr_t_entry chr_t_entry;

typedef struct chrmaptab_info *chrmaptab;

YAZ_EXPORT chrmaptab chrmaptab_create(const char *tabpath, const char *name,
				      int map_only, const char *tabroot);
YAZ_EXPORT void chrmaptab_destroy (chrmaptab tab);

YAZ_EXPORT const char **chr_map_input(chrmaptab t, const char **from, int len, int first);
YAZ_EXPORT const char **chr_map_input_x(chrmaptab t,
					const char **from, int *len, int first);
YAZ_EXPORT const char **chr_map_q_input(chrmaptab maptab,
					const char **from, int len, int first);
    
YAZ_EXPORT const char *chr_map_output(chrmaptab t, const char **from, int len);

YAZ_EXPORT unsigned char zebra_prim(char **s);

YAZ_END_CDECL

#endif
