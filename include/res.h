/* $Id: res.h,v 1.13 2004-01-22 11:27:21 adam Exp $
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



#ifndef RES_H
#define RES_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct res_struct *Res;

    Res res_open (const char *name, Res res_def, Res over_res);
    void res_close (Res r);
    const char *res_get (Res r, const char *name);
    const char *res_get_def (Res r, const char *name, const char *def);
    int res_get_match (Res r, const char *name, const char *value, const char *s);
    void res_set (Res r, const char *name, const char *value);
    int res_trav (Res r, const char *prefix, void *p,
		  void (*f)(void *p, const char *name, const char *value));
    int res_write (Res r);
    const char *res_get_prefix (Res r, const char *name, const char *prefix,
				const char *def);

#ifdef __cplusplus
}
#endif

#endif
