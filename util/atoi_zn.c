/* $Id: atoi_zn.c,v 1.2 2005-01-15 19:38:40 adam Exp $
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

#include <string.h>
#include <ctype.h>
#include <zebrautl.h>

zint atoi_zn (const char *buf, zint len)
{
    zint val = 0;

    while (--len >= 0)
    {
        if (isdigit (*buf))
            val = val*10 + (*buf - '0');
	buf++;
    }
    return val;
}

