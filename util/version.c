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

void zebra_get_version(char *version_str, char *sha1_str)
{
    if (version_str)
        strcpy(version_str, ZEBRAVER);
    if (sha1_str)
        strcpy(sha1_str, ZEBRA_VERSION_SHA1);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

