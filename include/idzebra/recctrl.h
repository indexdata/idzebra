/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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

#ifndef IDZEBRA_RECCTRL_H
#define IDZEBRA_RECCTRL_H

#include <sys/types.h>
#include <yaz/proto.h>
#include <yaz/odr.h>
#include <idzebra/res.h>
#include <idzebra/data1.h>
#include <idzebra/snippet.h>

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

/** Indexing token */
typedef struct {
    /** index type, e.g. "w", "p", .. */
    const char *index_type;
    /** index name, e.g. "title" */
    const char *index_name;
    /** token char data */
    const char *term_buf;
    /** length of term_buf */
    int  term_len;
    /** sequence number */
    zint seqno;
    /** segment number */
    zint segment;
    /** record ID */
    zint record_id;
    /** section ID */
    zint section_id;
    struct recExtractCtrl *extractCtrl;
} RecWord;

/** \brief record reader stream */
struct ZebraRecStream {
    /** client data */
    void      *fh;
    /** \brief read function */
    int       (*readf)(struct ZebraRecStream *s, char *buf, size_t count);
    /** \brief seek function */
    off_t     (*seekf)(struct ZebraRecStream *s, off_t offset);
    /** \brief tell function */
    off_t     (*tellf)(struct ZebraRecStream *s);
    /** \brief set and get of record position */
    off_t     (*endf)(struct ZebraRecStream *s, off_t *offset);
    /** \brief close and destroy stream */
    void      (*destroy)(struct ZebraRecStream *s);
};

/** record update action */
enum zebra_recctrl_action_t {
    /** insert record (fail if it exists already) */
    action_insert = 1,
    /** replace record (fail it it does not exist) */
    action_replace,
    /** delete record (fail if it does not exist) */
    action_delete,
    /** insert or replace */
    action_update,
    /** delete record (ignore if it does not exist) */
    action_a_delete
};

/** \brief record extract for indexing */
struct recExtractCtrl {
    struct ZebraRecStream *stream;
    void      (*init)(struct recExtractCtrl *p, RecWord *w);
    void      *clientData;
    void      (*tokenAdd)(RecWord *w);
    void      (*setStoreData)(struct recExtractCtrl *p, void *buf, size_t size);
    int       first_record;
    int       flagShowRecords;
    char      match_criteria[256];
    zint      staticrank;
    void      (*schemaAdd)(struct recExtractCtrl *p, Odr_oid *oid);
    data1_handle dh;
    void      *handle;
    enum zebra_recctrl_action_t action;
};

/* Retrieve record control */
struct recRetrieveCtrl {
    struct ZebraRecStream *stream;
    /* Input parameters ... */
    Res       res;		      /* Resource pool                     */
    ODR       odr;                    /* ODR used to create response       */
    const Odr_oid * input_format;     /* Preferred record syntax OID       */
    Z_RecordComposition *comp;        /* formatting instructions           */
    char      *encoding;              /* preferred character encoding      */
    zint      localno;                /* local id of record                */
    int       score;                  /* score 0-1000 or -1 if none        */
    zint      staticrank;             /* static rank >= 0,  0 if none */
    int       recordSize;             /* size of record in bytes */
    char      *fname;                 /* name of file (or NULL if internal) */
    data1_handle dh;

    /* response */
    const Odr_oid * output_format;    /* output format OID */
    void *     rec_buf;
    int        rec_len;
    int        diagnostic;
    char *     addinfo;

    /* special fetch to be included in retrieved response (say snippets) */
    void *handle;
    int (*special_fetch)(void *handle, const char *esn,
                         const Odr_oid *input_format,
                         const Odr_oid **output_format,
                         WRBUF result, WRBUF addinfo);
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
#define RECCTRL_EXTRACT_SKIP  4

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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

