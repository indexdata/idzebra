/* $Id: isam-codec.h,v 1.5 2006-10-29 17:20:00 adam Exp $
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

#ifndef ISAM_CODEC_H
#define ISAM_CODEC_H

typedef struct {
    void *(*start)(void);
    void (*stop)(void *p);
    void (*decode)(void *p, char **dst, const char **src);
    void (*encode)(void *p, char **dst, const char **src);
    void (*reset)(void *p);
} ISAM_CODEC;

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

