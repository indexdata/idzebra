/* $Id: res.h,v 1.7 2005-09-13 11:51:06 adam Exp $
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

#ifndef IDZEBRA_RES_H
#define IDZEBRA_RES_H

#include <idzebra/util.h>

YAZ_BEGIN_CDECL

typedef struct res_struct *Res;

YAZ_EXPORT
Res res_open(Res res_def, Res over_res);

YAZ_EXPORT
void res_close(Res r);

YAZ_EXPORT
ZEBRA_RES res_read_file(Res r, const char *fname);

YAZ_EXPORT
ZEBRA_RES res_write_file(Res r, const char *fname);

YAZ_EXPORT
void res_clear(Res r);

YAZ_EXPORT
const char *res_get(Res r, const char *name);

YAZ_EXPORT
const char *res_get_def(Res r, const char *name, const char *def);

YAZ_EXPORT
int res_get_match(Res r, const char *name, const char *value, const char *s);

YAZ_EXPORT
void res_set(Res r, const char *name, const char *value);

YAZ_EXPORT
int res_trav(Res r, const char *prefix, void *p,
	      void (*f)(void *p, const char *name, const char *value));

YAZ_EXPORT
const char *res_get_prefix(Res r, const char *name, const char *prefix,
			   const char *def);

YAZ_EXPORT
ZEBRA_RES res_get_int(Res r, const char *name, int *val);

YAZ_END_CDECL

#endif
