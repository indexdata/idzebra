/* $Id: res.h,v 1.3 2005-01-15 19:38:24 adam Exp $
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

#ifndef RES_H
#define RES_H

#include <yaz/yaz-version.h>

YAZ_BEGIN_CDECL

typedef struct res_struct *Res;

YAZ_EXPORT
Res res_open (const char *name, Res res_def, Res over_res);

YAZ_EXPORT
void res_close (Res r);

YAZ_EXPORT
const char *res_get (Res r, const char *name);

YAZ_EXPORT
const char *res_get_def (Res r, const char *name, const char *def);

YAZ_EXPORT
int res_get_match (Res r, const char *name, const char *value, const char *s);

YAZ_EXPORT
void res_set (Res r, const char *name, const char *value);

YAZ_EXPORT
int res_trav (Res r, const char *prefix, void *p,
	      void (*f)(void *p, const char *name, const char *value));

YAZ_EXPORT
int res_write (Res r);

YAZ_EXPORT
const char *res_get_prefix (Res r, const char *name, const char *prefix,
			    const char *def);

YAZ_END_CDECL

#endif
