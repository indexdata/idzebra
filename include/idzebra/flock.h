/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include <yaz/yconfig.h>

#ifndef FLOCK_H
#define FLOCK_H

YAZ_BEGIN_CDECL

typedef struct zebra_lock_handle *ZebraLockHandle;

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

YAZ_EXPORT 
void zebra_flock_init(void);

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

