/* $Id: isam-codec.h,v 1.1 2004-08-04 08:35:23 adam Exp $
   Copyright (C) 2004
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
