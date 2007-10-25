/* $Id: index_types.h,v 1.2 2007-10-25 19:25:00 adam Exp $
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
    \file
    \brief Definitions for Zebra's index types
*/

#ifndef ZEBRA_INDEX_TYPES_H
#define ZEBRA_INDEX_TYPES_H

#include <yaz/yconfig.h>
#include <yaz/xmltypes.h>

YAZ_BEGIN_CDECL

/** \brief zebra index types handle (ptr) */
typedef struct zebra_index_types_s *zebra_index_types_t;

/** \brief zebra index type handle (ptr) */
typedef struct zebra_index_type_s *zebra_index_type_t;

/** \brief creates index types handler/object from file
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
    \param types handle
 */
void zebra_index_types_destroy(zebra_index_types_t types);


/** \brief creates index types handler/object from xml Doc
    \param doc Libxml2 document
    \returns handle (NULL if unsuccessful)

    Similar to zebra_index_types_create
*/
zebra_index_types_t zebra_index_types_create_doc(xmlDocPtr doc);


/** \brief lookup of index type
    \param types types
    \param id id to search for
    \returns pattern ID
*/
const char *zebra_index_type_lookup_str(zebra_index_types_t types, 
                                        const char *id);


/** \brief get index type of a given ID
    \param types types
    \param id ID to search for
    \returns index type handle
*/
zebra_index_type_t zebra_index_type_get(zebra_index_types_t types, 
                                        const char *id);

/** \brief check whether index type is of type 'index'
    \param type index type
    \retval 1 YES
    \retval 0 NO
*/
int zebra_index_type_is_index(zebra_index_type_t type);

/** \brief check whether index type is of type 'sort'
    \param type index type
    \retval 1 YES
    \retval 0 NO
*/
int zebra_index_type_is_sort(zebra_index_type_t type);

/** \brief check whether index type is of type 'staticrank'
    \param type index type
    \retval 1 YES
    \retval 0 NO
*/
int zebra_index_type_is_staticrank(zebra_index_type_t type);


/** \brief tokenize a term for an index type
    \param type index type
    \param buf term buffer (pass 0 to continue with previous buf)
    \param len term length
    \param result_buf resulting token buffer
    \param result_len resulting token length
    \retval 1 token read and result is in result_buf
    \retval 0 no token read (no more tokens in buf)
*/
int zebra_index_type_tokenize(zebra_index_type_t type,
                              const char *buf, size_t len,
                              const char **result_buf, size_t *result_len);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

