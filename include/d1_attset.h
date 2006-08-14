/* $Id: d1_attset.h,v 1.2.2.1 2006-08-14 10:38:55 adam Exp $
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

#ifndef D1_ATTSET_H
#define D1_ATTSET_H

#include <yaz/oid.h>

YAZ_BEGIN_CDECL

/*
 * This structure describes a attset, perhaps made up by inclusion
 * (supersetting) of other attribute sets. When indexing and searching,
 * we perform a normalisation, where we associate a given tag with
 * the set that originally defined it, rather than the superset. This
 * allows the most flexible access. Eg, the tags common to GILS and BIB-1
 * should be searchable by both names.
 */

struct data1_attset;

typedef struct data1_local_attribute
{
    int local;
    struct data1_local_attribute *next;
} data1_local_attribute;

typedef struct data1_attset data1_attset;    
typedef struct data1_att data1_att;
typedef struct data1_attset_child data1_attset_child;

struct data1_att
{
    data1_attset *parent;          /* attribute set */
    char *name;                    /* symbolic name of this attribute */
    int value;                     /* attribute value */
    data1_local_attribute *locals; /* local index values */
    data1_att *next;
};

struct data1_attset_child {
    data1_attset *child;
    data1_attset_child *next;
};

struct data1_attset
{
    char *name;          /* symbolic name */
    oid_value reference;   /* external ID of attset */
    data1_att *atts;          /* attributes */
    data1_attset_child *children;  /* included attset */
    data1_attset *next;       /* next in cache */
};

typedef struct data1_handle_info *data1_handle;

YAZ_EXPORT data1_att *data1_getattbyname(data1_handle dh, data1_attset *s,
					 char *name);
YAZ_EXPORT data1_attset *data1_read_attset(data1_handle dh, const char *file);

YAZ_EXPORT data1_attset *data1_empty_attset(data1_handle dh);

YAZ_END_CDECL

#endif
