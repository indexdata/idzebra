/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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


/** \brief gets next record - with given records
    \param p records handle
    \param rec record
    \returns record pointer (or NULL on error)
*/
Record rec_get_next(Records p, Record rec);

ZEBRA_RES rec_close (Records *p);

/** \brief opens records system
    \param bfs block file storage
    \param rw read-write flag(0=read only, 1=write)
    \param compression_method REC_COMPRESS_ type 
*/
Records rec_open(BFiles bfs, int rw, int compression_method);

/** \brief check whether a compression method is supported
    \param compression_method (REC_COMPRESS_..)
    \retval 0 if method is unsupported
    \retval 1 if method is supported
*/
int rec_check_compression_method(int compression_method);

char *rec_strdup(const char *s, size_t *len);
void rec_prstat(Records p, int verbose);

zint rec_sysno_to_int(zint sysno);


/** \brief No compression ("none") */
#define REC_COMPRESS_NONE   0
/** \brief BZIP2 compression (slow and requires big chunks) */
#define REC_COMPRESS_BZIP2  1
/** \brief zlib compression (faster and works off small chunks) */
#define REC_COMPRESS_ZLIB   2


enum { 
    recInfo_fileType, 
    recInfo_filename, 
    recInfo_delKeys, 
    recInfo_databaseName,
    recInfo_storeData,
    recInfo_attr,
    recInfo_sortKeys
};

typedef struct recindex *recindex_t;

/** \brief opens record index handle
    \param bfs Block files handle
    \param rw 1 for read and write; 0 for read-only
    \param use_isamb 1 if ISAMB is to used for record index; 0 for flat (old)
*/
recindex_t recindex_open(BFiles bfs, int rw, int use_isamb);

/** \brief closes record index handle
    \param p records handle
*/
void recindex_close(recindex_t p);
int recindex_read_head(recindex_t p, void *buf);
const char *recindex_get_fname(recindex_t p);
ZEBRA_RES recindex_write_head(recindex_t p, const void *buf, size_t len);
int recindex_read_indx(recindex_t p, zint sysno, void *buf, int itemsize, 
                       int ignoreError);
void recindex_write_indx(recindex_t p, zint sysno, void *buf, int itemsize);

YAZ_END_CDECL
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

