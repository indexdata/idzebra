/* $Id: flock.h,v 1.3 2006-05-10 08:13:20 adam Exp $
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

#include <yaz/yconfig.h>

#ifndef FLOCK_H
#define FLOCK_H

YAZ_BEGIN_CDECL

typedef struct zebra_lock_info *ZebraLockHandle;

YAZ_EXPORT
ZebraLockHandle zebra_lock_create(const char *dir, const char *file);

YAZ_EXPORT
void zebra_lock_destroy (ZebraLockHandle h);

YAZ_EXPORT
int zebra_unlock (ZebraLockHandle h);
YAZ_EXPORT
char *zebra_mk_fname (const char *dir, const char *name);

YAZ_EXPORT
int zebra_lock_w (ZebraLockHandle h);
YAZ_EXPORT
int zebra_lock_r (ZebraLockHandle h);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

