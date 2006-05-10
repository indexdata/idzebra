/* $Id: zint.c,v 1.2 2006-05-10 08:13:46 adam Exp $
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

#include <idzebra/util.h>

void zebra_zint_encode(char **dst, zint pos)
{
    unsigned char *bp = (unsigned char*) *dst;

    while (pos > 127)
    {
         *bp++ = (unsigned char) (128 | (pos & 127));
         pos = pos >> 7;
    }
    *bp++ = (unsigned char) pos;
    *dst = (char *) bp;
}

void zebra_zint_decode(const char **src, zint *pos)
{
    const unsigned char **bp = (const unsigned char **) src;
    zint d = 0;
    unsigned char c;
    unsigned r = 0;

    while (((c = *(*bp)++) & 128))
    {
        d += ((zint) (c & 127) << r);
	r += 7;
    }
    d += ((zint) c << r);
    *pos = d;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

