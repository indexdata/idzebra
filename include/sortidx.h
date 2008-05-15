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

#ifndef SORTIDX_H
#define SORTIDX_H

#include <yaz/yconfig.h>
#include <idzebra/version.h>
#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

#define SORT_IDX_ENTRYSIZE 64

/** \var zebra_sort_index_t
    \brief sort index handle
*/
typedef struct zebra_sort_index *zebra_sort_index_t;

#define ZEBRA_SORT_TYPE_FLAT 1
#define ZEBRA_SORT_TYPE_ISAMB 2


/** \brief creates sort handle
    \param bfs block files handle
    \param write_flag (0=read-only, 1=write and read)
    \param sort_type one of ZEBRA_SORT_TYPE_..
    \return sort index handle
*/
zebra_sort_index_t zebra_sort_open(BFiles bfs, int write_flag, int sort_type);

/** \brief frees sort handle
*/
void zebra_sort_close(zebra_sort_index_t si);

/** \brief sets type for sort usage
    \param si sort index handle
    \param type opaque type .. A sort file for each type is created
*/
int zebra_sort_type(zebra_sort_index_t si, int type);

/** \brief sets sort system number for read / add / delete
    \param si sort index handle
    \param sysno system number
*/
void zebra_sort_sysno(zebra_sort_index_t si, zint sysno);

/** \brief adds content to sort file
    \param si sort index handle
    \param buf buffer content
    \param len length

    zebra_sort_type and zebra_sort_sysno must be called prior to this
*/
void zebra_sort_add(zebra_sort_index_t si, const char *buf, int len);


/** \brief delete sort entry
    \param si sort index handle

    zebra_sort_type and zebra_sort_sysno must be called prior to this
*/
void zebra_sort_delete(zebra_sort_index_t si);

/** \brief reads sort entry
    \param si sort index handle
    \param buf resulting buffer
    \retval 0 could not be read
    \retval 1 could be read (found)
*/
int zebra_sort_read(zebra_sort_index_t si, char *buf);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

