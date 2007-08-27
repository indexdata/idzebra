/* $Id: version.c,v 1.3 2007-08-27 17:43:21 adam Exp $
   Copyright (C) 1995-2007
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

/**
 * \file version.c
 * \brief Implements Zebra version utilities.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaz/snprintf.h>
#include <string.h>
#include <idzebra/version.h>

void zebra_get_version(char *version_str, char *sys_str)
{
    if (version_str)
        strcpy(version_str, ZEBRAVER);
    if (sys_str)
    {
        strcpy(sys_str, "unknown");

#ifdef WIN32
        strcpy(sys_str, "win32");
#ifdef _MSC_VER
        yaz_snprintf(sys_str+strlen(sys_str), 25, "; mscver %lu",
                 (unsigned long) _MSC_VER);
#endif
#endif
#ifdef HOST_TRIPLET
        strcpy(sys_str, HOST_TRIPLET);
#endif
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

