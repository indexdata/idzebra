/* $Id: recindex.h,v 1.20.2.1 2006-08-14 10:38:59 adam Exp $
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



#ifndef RECINDEX_H
#define RECINDEX_H

#include <zebrautl.h>
#include <bfile.h>

YAZ_BEGIN_CDECL

#define REC_NO_INFO 8

typedef struct record_info {
    int sysno;
    int newFlag;
    char *info[REC_NO_INFO];
    size_t size[REC_NO_INFO];
    char buf_size[REC_NO_INFO][6];
    size_t size_size[REC_NO_INFO];
    Zebra_mutex mutex;
} *Record;

typedef struct records_info *Records;

Record rec_cp (Record rec);
void rec_del (Records p, Record *recpp);
void rec_rm (Record *recpp);
void rec_put (Records p, Record *recpp);
Record rec_new (Records p);
Record rec_get (Records p, int sysno);
void rec_close (Records *p);
Records rec_open (BFiles bfs, int rw, int compression_method);
char *rec_strdup (const char *s, size_t *len);
void rec_prstat (Records p);

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
