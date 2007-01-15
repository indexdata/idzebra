/* $Id: recindex.h,v 1.31 2007-01-15 20:08:25 adam Exp $
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

#ifndef RECINDEX_H
#define RECINDEX_H

#include <idzebra/util.h>
#include <zebra-lock.h>
#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

#define REC_NO_INFO 8

typedef struct record_info {
    zint sysno;
    int newFlag;
    char *info[REC_NO_INFO];
    size_t size[REC_NO_INFO];
    char buf_size[REC_NO_INFO][6];
    size_t size_size[REC_NO_INFO];
    Zebra_mutex mutex;
} *Record;

typedef struct records_info *Records;

/** \brief marks record for deletion (on file storage)
    \param p records handle
    \param recpp record pointer
    \returns ZEBRA_OK / ZEBRA_FAIL
*/
ZEBRA_RES rec_del(Records p, Record *recpp);

/** \brief frees record (from memory)
    \param recpp record pointer
*/
void rec_free(Record *recpp);

/** \brief puts record (writes into file storage)
    \param p records handle
    \param recpp record pointer
    \returns ZEBRA_OK / ZEBRA_FAIL
*/
ZEBRA_RES rec_put(Records p, Record *recpp);

/** \brief creates new record (to be written to file storage)
    \param p records handle
    \returns record pointer (or NULL on error)
*/
Record rec_new(Records p);
/** \brief gets record - with given system number
    \param p records handle
    \param sysno system ID (external number)
    \returns record pointer (or NULL on error)
*/
Record rec_get(Records p, zint sysno);

/** \brief gets root record
    \param p records handle
    \returns record pointer (or NULL on error)
*/
Record rec_get_root(Records p);
ZEBRA_RES rec_close (Records *p);

/** \brief opens records system
    \param bfs block file storage
    \param rw read-write flag(0=read only, 1=write)
    \param compression_method REC_COMPRESS_ type 
*/
Records rec_open(BFiles bfs, int rw, int compression_method);

char *rec_strdup(const char *s, size_t *len);
void rec_prstat(Records p);

zint rec_sysno_to_int(zint sysno);

/** \brief compression types */
#define REC_COMPRESS_NONE   0
#define REC_COMPRESS_BZIP2  1

enum { 
    recInfo_fileType, 
    recInfo_filename, 
    recInfo_delKeys, 
    recInfo_databaseName,
    recInfo_storeData,
    recInfo_attr,
    recInfo_sortKeys
};

YAZ_END_CDECL
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

