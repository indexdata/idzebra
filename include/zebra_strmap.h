/* $Id: zebra_strmap.h,v 1.2 2007-12-03 09:12:38 adam Exp $
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

#ifndef ZEBRA_STRMAP_H
#define ZEBRA_STRMAP_H

#include <yaz/yconfig.h>
#include <stddef.h>
YAZ_BEGIN_CDECL

typedef struct zebra_strmap *zebra_strmap_t;
typedef struct zebra_strmap_it_s *zebra_strmap_it;

YAZ_EXPORT
zebra_strmap_t zebra_strmap_create(void);

YAZ_EXPORT
void zebra_strmap_destroy(zebra_strmap_t st);

YAZ_EXPORT
void zebra_strmap_add(zebra_strmap_t st, const char *name,
                      void *data_buf, size_t data_len);

YAZ_EXPORT
void *zebra_strmap_lookup(zebra_strmap_t st, const char *name, int no,
                          size_t *data_len);

YAZ_EXPORT
int zebra_strmap_remove(zebra_strmap_t st, const char *name);

YAZ_EXPORT
zebra_strmap_it zebra_strmap_it_create(zebra_strmap_t st);

YAZ_EXPORT
void zebra_strmap_it_destroy(zebra_strmap_it it);

YAZ_EXPORT
const char *zebra_strmap_it_next(zebra_strmap_it it, void **data_buf,
                                 size_t *data_len);


YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

