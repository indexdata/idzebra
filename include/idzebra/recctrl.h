/* $Id: recctrl.h,v 1.21 2006-06-07 10:14:40 adam Exp $
   Copyright (C) 1995-2006
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#ifndef IDZEBRA_RECCTRL_H
#define IDZEBRA_RECCTRL_H

#include <sys/types.h>
#include <yaz/proto.h>
#include <yaz/oid.h>
#include <yaz/odr.h>
#include <idzebra/res.h>
#include <idzebra/data1.h>
#include <idzebra/snippet.h>
#include <idzebra/zebramap.h>

YAZ_BEGIN_CDECL

/* 1 */
#define ZEBRA_XPATH_ELM_BEGIN "_XPATH_BEGIN"

/* 2 */
#define ZEBRA_XPATH_ELM_END   "_XPATH_END"

/* 1016 */
#define ZEBRA_XPATH_CDATA     "_XPATH_CDATA"

/* 3 */
#define ZEBRA_XPATH_ATTR_NAME       "_XPATH_ATTR_NAME"

/* 1015 */
#define ZEBRA_XPATH_ATTR_CDATA      "_XPATH_ATTR_CDATA"

/* single word entity */
typedef struct {
    unsigned index_type;
    const char *index_name;
    const char *term_buf;
    int  term_len;
    zint seqno;
    zint record_id;
    zint section_id;
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
    void      (*setStoreData)(struct recExtractCtrl *p, void *buf, size_t size);
    ZebraMaps zebra_maps;
    int       first_record;
    int       flagShowRecords;
    int       seqno[256];
    char      match_criteria[256];
    zint      staticrank;
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
    int       staticrank;             /* static rank >= 0,  0 if none */
    int       recordSize;             /* size of record in bytes */
    char      *fname;                 /* name of file (or NULL if internal) */
    data1_handle dh;
    zebra_snippets *hit_snippet;
    zebra_snippets *doc_snippet;
    
    /* response */
    oid_value  output_format;
    void       *rec_buf;
    int        rec_len;
    int        diagnostic;
    char       *addinfo;
};

typedef struct recType *RecType;

struct recType
{
    int version;
    char *name;                           /* Name of record type */
    void *(*init)(Res res, RecType recType);  /* Init function - called once */
    ZEBRA_RES (*config)(void *clientData, Res res, const char *args); /* Config */
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

YAZ_EXPORT
RecTypeClass recTypeClass_create (Res res, NMEM nmem);

YAZ_EXPORT
void recTypeClass_load_modules(RecTypeClass *rts, NMEM nmem,
			       const char *module_path);

YAZ_EXPORT
RecTypeClass recTypeClass_add_modules(Res res, NMEM nmem,
				      const char *module_path);

YAZ_EXPORT
void recTypeClass_destroy(RecTypeClass rtc);

YAZ_EXPORT
void recTypeClass_info(RecTypeClass rtc, void *cd,
		       void (*cb)(void *cd, const char *s));

YAZ_EXPORT
RecTypes recTypes_init(RecTypeClass rtc, data1_handle dh);

YAZ_EXPORT
void recTypes_destroy(RecTypes recTypes);

YAZ_EXPORT
void recTypes_default_handlers(RecTypes recTypes, Res res);

YAZ_EXPORT
RecType recType_byName(RecTypes rts, Res res, const char *name,
		       void **clientDataP);


YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

