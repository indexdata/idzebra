/* $Id: dict.h,v 1.7 2006-08-29 09:27:56 adam Exp $
   Copyright (C) 1995-2006
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

/** \file dict.h
    \brief Zebra dictionary
    
    The dictionary is a hash that maps a string to a value.
    The value is opaque and is defined as a sequence of bytes
    with a length in the range 0 to 255.
*/

#ifndef DICT_H
#define DICT_H

#include <yaz/yconfig.h>
#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

/** \var Dict
 * \brief Dictionary type
 *
 * Most dictionary functions operatate on a Dict type (object).
 */
typedef struct Dict_struct *Dict;

typedef unsigned Dict_ptr;

YAZ_EXPORT 
Dict dict_open (BFiles bfs, const char *name, int cache, int rw,
		      int compact_flag, int page_size);

YAZ_EXPORT
int dict_close (Dict dict);

YAZ_EXPORT
int dict_insert (Dict dict, const char *p, int userlen, void *userinfo);

YAZ_EXPORT
int dict_delete (Dict dict, const char *p);

YAZ_EXPORT
int dict_delete_subtree (Dict dict, const char *p, void *client,
				int (*f)(const char *info, void *client));
YAZ_EXPORT
char *dict_lookup (Dict dict, const char *p);

YAZ_EXPORT
int dict_lookup_ec (Dict dict, char *p, int range, int (*f)(char *name));

YAZ_EXPORT
int dict_lookup_grep (Dict dict, const char *p, int range, void *client,
		      int *max_pos, int init_pos,
		      int (*f)(char *name, const char *info, void *client));

YAZ_EXPORT
int dict_scan (Dict dict, char *str, 
	       int *before, int *after, void *client,
	       int (*f)(char *name, const char *info, int pos, void *client));

YAZ_EXPORT 
void dict_grep_cmap (Dict dict, void *vp,
		     const char **(*cmap)(void *vp,
					  const char **from, int len));

YAZ_EXPORT
int dict_copy_compact (BFiles bfs, const char *from, const char *to);

YAZ_END_CDECL
   
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

