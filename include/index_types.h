/* $Id: index_types.h,v 1.1 2007-10-25 09:22:36 adam Exp $
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
    \files
    \brief Definitions for Zebra's index types
*/

#ifndef ZEBRA_INDEX_TYPES_H
#define ZEBRA_INDEX_TYPES_H

#include <yaz/yconfig.h>
#include <yaz/xmltypes.h>

YAZ_BEGIN_CDECL

/**
   \brief zebra index rules handle (ptr)
*/
typedef struct zebra_index_types_s *zebra_index_types_t;

/** \brief creates index rules handler/object from file
    \param fname filename
    \returns handle (NULL if unsuccessful)

    Config file format:
    \verbatim
    <indextypes>
      <indextype id="*:w" position="1" alwaysmatches="1" firstinfield="1"
        locale="en">
        <!-- conversion rules for words -->
      </indextype>
      <indextype id="*:p" position="0" alwaysmatches="0" firstinfield="0"
        locale="en">
        <!-- conversion rules for phrase -->
      </indextype>
      <indextype id="*:s" sort="1" 
        locale="en">
        <!-- conversion rules for phrase -->
      </indextype>
    </indextypes>
   \endverbatim
 */
zebra_index_types_t zebra_index_types_create(const char *fname);

/** \brief destroys index rules object
    \param r handle
 */
void zebra_index_types_destroy(zebra_index_types_t r);


/** \brief creates index types handler/object from xml Doc
    \param doc Libxml2 document
    \returns handle (NULL if unsuccessful)

    Similar to zebra_index_types_create
*/
zebra_index_types_t zebra_index_types_create_doc(xmlDocPtr doc);


/** \brief lookup of index type
    \param r rules
    \param id id to search for
    \returns pattern ID
*/
const char *zebra_index_type_lookup_str(zebra_index_types_t r, const char *id);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

