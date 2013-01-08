/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <yaz/xmalloc.h>
#include <su_codec.h>

int key_SU_encode (int ch, char *out)
{
    int i;

    if (ch == -1)
    {
        /* unique value .. which is different from ch >= 0 case */
        /* is used to generate queries with "null" hits, bug #1142 */
        out[0] = 129;
        return 1;
    }
    for (i = 0; ch; i++)
    {
	if (ch >= 64)
	    out[i] = 65 + (ch & 63);
	else
	    out[i] = 1 + ch;
	ch = ch >> 6;
    }
    return i;
    /* in   out
       0     1
       1     2
       63    64
       64    65, 2
       65    66, 2
       127   128, 2
       128   65, 3
       191   128, 3
       192   65, 4
    */
}

int key_SU_decode (int *ch, const unsigned char *out)
{
    int len = 1;
    int fact = 1;
    *ch = 0;
    for (len = 1; *out >= 65; len++, out++)
    {
	*ch += (*out - 65) * fact;
	fact <<= 6;
    }
    *ch += (*out - 1) * fact;
    return len;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

