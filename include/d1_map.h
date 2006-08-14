/* $Id: d1_map.h,v 1.2.2.1 2006-08-14 10:38:55 adam Exp $
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

#ifndef D1_MAP_H
#define D1_MAP_H

YAZ_BEGIN_CDECL

typedef struct data1_maptag
{
    int new_field;
    int type;
#define D1_MAPTAG_numeric 1
#define D1_MAPTAG_string 2
    int which;
    union
    {
	int numeric;
	char *string;
    } value;
    struct data1_maptag *next;
} data1_maptag;

typedef struct data1_mapunit
{
    int no_data;
    char *source_element_name;
    data1_maptag *target_path;
    struct data1_mapunit *next;
} data1_mapunit;

typedef struct data1_maptab
{
    char *name;
    oid_value target_absyn_ref;
    char *target_absyn_name;
    data1_mapunit *map;
    struct data1_maptab *next;
} data1_maptab;

YAZ_END_CDECL

#endif
