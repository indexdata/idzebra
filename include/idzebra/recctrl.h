/* $Id: recctrl.h,v 1.2 2004-09-28 12:39:54 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#ifndef RECCTRL_H
#define RECCTRL_H

#include <yaz/proto.h>
#include <yaz/oid.h>
#include <yaz/odr.h>
#include <idzebra/res.h>
#include <idzebra/data1.h>
#include <idzebra/zebramap.h>

#ifdef __cplusplus
extern "C" {
#endif

/* single word entity */
typedef struct {
    int  attrSet;
    int  attrUse;
    unsigned reg_type;
    char *string;
    int  length;
    int  seqno;
    ZebraMaps zebra_maps;
    struct recExtractCtrl *extractCtrl;
} RecWord;

/* Extract record control */
struct recExtractCtrl {
    void      *fh;                    /* File handle and read function     */
    int       (*readf)(void *fh, char *buf, size_t count);
    off_t     (*seekf)(void *fh, off_t offset);  /* seek function          */
    off_t     (*tellf)(void *fh);                /* tell function          */
    void      (*endf)(void *fh, off_t offset);   /* end of record position */
    off_t     offset;                            /* start offset           */
    void      (*init)(struct recExtractCtrl *p, RecWord *w);
    void      *clientData;
    void      (*tokenAdd)(RecWord *w);
    ZebraMaps zebra_maps;
    int       flagShowRecords;
    int       seqno[256];
    void      (*schemaAdd)(struct recExtractCtrl *p, Odr_oid *oid);
    data1_handle dh;
    void      *handle;
};

/* Retrieve record control */
struct recRetrieveCtrl {
    /* Input parameters ... */
    Res       res;		      /* Resource pool                     */
    ODR       odr;                    /* ODR used to create response       */
    void     *fh;                     /* File descriptor and read function */
    int       (*readf)(void *fh, char *buf, size_t count);
    off_t     (*seekf)(void *fh, off_t offset);
    off_t     (*tellf)(void *fh);
    oid_value input_format;           /* Preferred record syntax           */
    Z_RecordComposition *comp;        /* formatting instructions           */
    char      *encoding;              /* preferred character encoding      */
    zint      localno;                /* local id of record                */
    int       score;                  /* score 0-1000 or -1 if none        */
    int       recordSize;             /* size of record in bytes */
    char      *fname;                 /* name of file (or NULL if internal) */
    data1_handle dh;
    
    /* response */
    oid_value  output_format;
    void       *rec_buf;
    int        rec_len;
    int        diagnostic;
    char *message;
};

typedef struct recType *RecType;

struct recType
{
    char *name;                           /* Name of record type */
    void *(*init)(Res res, RecType recType);  /* Init function - called once */
    void (*config)(void *clientData, Res res, const char *args); /* Config */
    void (*destroy)(void *clientData);    /* Destroy function */
    int  (*extract)(void *clientData,
		    struct recExtractCtrl *ctrl);   /* Extract proc */
    int  (*retrieve)(void *clientData,
		     struct recRetrieveCtrl *ctrl); /* Retrieve proc */
};

#define RECCTRL_EXTRACT_OK    0
#define RECCTRL_EXTRACT_EOF   1
#define RECCTRL_EXTRACT_ERROR_GENERIC 2
#define RECCTRL_EXTRACT_ERROR_NO_SUCH_FILTER 3

typedef struct recTypeClass *RecTypeClass;
typedef struct recTypes *RecTypes;

RecTypeClass recTypeClass_create (Res res, NMEM nmem);
void recTypeClass_destroy(RecTypeClass rtc);
void recTypeClass_info(RecTypeClass rtc, void *cd,
		       void (*cb)(void *cd, const char *s));

RecTypes recTypes_init(RecTypeClass rtc, data1_handle dh);
void recTypes_destroy(RecTypes recTypes);
void recTypes_default_handlers(RecTypes recTypes, Res res);

RecType recType_byName(RecTypes rts, Res res, const char *name,
		       void **clientDataP);


#ifdef __cplusplus
}
#endif

#endif
