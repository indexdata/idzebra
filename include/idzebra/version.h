/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

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

#ifndef IDZEBRA_VERSION_H
#define IDZEBRA_VERSION_H

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

#define ZEBRAVER "2.0.30"

#define ZEBRA_FILEVERSION 2,0,30,1

/** \brief Returns Zebra version and system info.
    \param version_str buffer for version (at least 16 bytes)
    \param sys_str buffer for system info (at least 80 bytes)
    \returns version as integer
*/
YAZ_EXPORT
void zebra_get_version(char *version_str, char *sys_str);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

