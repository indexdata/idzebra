/* $Id: isutil.c,v 1.5.2.1 2006-08-14 10:39:03 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



/*
 * Small utilities needed by the isam system. Some or all of these
 * may move to util/ along the way.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <isam.h>
#include "isutil.h"

char *strconcat(const char *s1, ...)
{
    va_list ap;
    static char buf[512];
    char *p;

    va_start(ap, s1);
    strcpy(buf, s1);
    while ((p = va_arg(ap, char *)))
    	strcat(buf, p);
    va_end(ap);
    
    return buf;
}

int is_default_cmp(const void *k1, const void *k2)
{
    int b1, b2;

    memcpy(&b1, k1, sizeof(b1));
    memcpy(&b2, k2, sizeof(b2));
    return b1 - b2;
}
