/* $Id: charmap.h,v 1.8 2002-08-02 19:26:55 adam Exp $
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



#ifndef CHARMAP_H
#define CHARMAP_H

#include <yaz/yconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

YAZ_EXPORT extern const char *CHR_UNKNOWN;
YAZ_EXPORT extern const char *CHR_SPACE;
YAZ_EXPORT extern const char *CHR_BASE;

struct chr_t_entry;
typedef struct chr_t_entry chr_t_entry;

typedef struct chrmaptab_info *chrmaptab;

YAZ_EXPORT chrmaptab chrmaptab_create(const char *tabpath, const char *name,
				      int map_only, const char *tabroot);
YAZ_EXPORT void chrmaptab_destroy (chrmaptab tab);

YAZ_EXPORT const char **chr_map_input(chrmaptab t, const char **from, int len);
YAZ_EXPORT const char **chr_map_input_x(chrmaptab t,
					const char **from, int *len);
YAZ_EXPORT const char **chr_map_input_q(chrmaptab maptab,
					const char **from, int len,
					const char **qmap);
    
YAZ_EXPORT const char *chr_map_output(chrmaptab t, const char **from, int len);

YAZ_EXPORT unsigned char zebra_prim(char **s);

#ifdef __cplusplus
}
#endif

#endif
