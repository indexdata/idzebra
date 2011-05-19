/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
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

zint atozint(const char *src)
{
#if HAVE_ATOLL
    return atoll(src);
#else
    return atoi(src);
#endif
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

