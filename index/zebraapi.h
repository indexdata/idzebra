/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: zebraapi.h,v 1.14 2002-04-04 14:14:13 adam Exp $
 */

#ifndef ZEBRAAPI_H
#define ZEBRAAPI_H

#include <yaz/odr.h>
#include <yaz/oid.h>
#include <yaz/proto.h>
#include <zebraver.h>

YAZ_BEGIN_CDECL

struct recordGroup {
    char         *groupName;
    char         *databaseName;
    char         *path;
    char         *recordId;
    char         *recordType;
    int          flagStoreData;
    int          flagStoreKeys;
    int          flagRw;
    int          fileVerboseLimit;
    int          databaseNamePath;
    int          explainDatabase;
};

/* Retrieval Record Descriptor */
typedef struct {
    int errCode;         /* non-zero if error when fetching this */
    char *errString;     /* error string */
    int position;        /* position of record in result set (1,2,..) */
    char *buf;           /* record buffer (void pointer really) */
    int len;             /* length */
    oid_value format;    /* record syntax */
    char *base; 
} ZebraRetrievalRecord;

/* Scan Term Descriptor */
typedef struct {
    int occurrences;     /* scan term occurrences */
    char *term;          /* scan term string */
} ZebraScanEntry;

typedef struct zebra_session *ZebraHandle;
typedef struct zebra_service *ZebraService;

/* Open Zebra using file 'configName' (usually zebra.cfg) */
YAZ_EXPORT ZebraHandle zebra_open (ZebraService zs);

/* Search using RPN-Query */
YAZ_EXPORT void zebra_search_rpn (ZebraHandle zh, ODR input, ODR output,
                                  Z_RPNQuery *query,
                                  const char *setname, int *hits);

/* Retrieve record(s) */
YAZ_EXPORT void zebra_records_retrieve (ZebraHandle zh, ODR stream,
		       const char *setname, Z_RecordComposition *comp,
		       oid_value input_format,
		       int num_recs, ZebraRetrievalRecord *recs);

/* Browse */
YAZ_EXPORT void zebra_scan (ZebraHandle zh, ODR stream,
			    Z_AttributesPlusTerm *zapt,
			    oid_value attributeset,
			    int *position, int *num_entries,
			    ZebraScanEntry **list,
			    int *is_partial);
    
/* Delete Result Set(s) */
YAZ_EXPORT int zebra_deleleResultSet(ZebraHandle zh, int function,
				     int num_setnames, char **setnames,
				     int *statuses);

/* Close zebra and destroy handle */
YAZ_EXPORT void zebra_close (ZebraHandle zh);

/* last error code */
YAZ_EXPORT int zebra_errCode (ZebraHandle zh);
/* string representatio of above */
YAZ_EXPORT const char *zebra_errString (ZebraHandle zh);

/* extra information associated with error */
YAZ_EXPORT char *zebra_errAdd (ZebraHandle zh);

/* do authentication */
YAZ_EXPORT int zebra_auth (ZebraHandle zh, const char *user, const char *pass);

/* Character normalisation on specific register .
   This routine is subject to change - do not use. */
YAZ_EXPORT int zebra_string_norm (ZebraHandle zh, unsigned reg_id,
				  const char *input_str, int input_len,
				  char *output_str, int output_len);

YAZ_EXPORT void zebra_admin_create (ZebraHandle zh, const char *db);

YAZ_EXPORT ZebraService zebra_start (const char *configName);
YAZ_EXPORT void zebra_stop (ZebraService zs);

YAZ_EXPORT void zebra_admin_shutdown (ZebraHandle zh);
YAZ_EXPORT void zebra_admin_start (ZebraHandle zh);

YAZ_EXPORT void zebra_shutdown (ZebraService zs);

YAZ_EXPORT void zebra_admin_import_begin (ZebraHandle zh, const char *database);

YAZ_EXPORT void zebra_admin_import_segment (ZebraHandle zh,
					    Z_Segment *segment);

void zebra_admin_import_end (ZebraHandle zh);

void zebra_begin_trans (ZebraHandle zh);
void zebra_end_trans (ZebraHandle zh);

int zebra_commit (ZebraHandle zh);

int zebra_init (ZebraHandle zh);
int zebra_compact (ZebraHandle zh);
void zebra_repository_update (ZebraHandle zh);
void zebra_repository_delete (ZebraHandle zh);
void zebra_repository_show (ZebraHandle zh);
int zebra_record_insert (ZebraHandle zh, const char *buf, int len);

YAZ_EXPORT void zebra_set_group (ZebraHandle zh, struct recordGroup *rg);

YAZ_EXPORT void zebra_result (ZebraHandle zh, int *code, char **addinfo);

YAZ_EXPORT const char *zebra_resultSetTerms (ZebraHandle zh,
                                             const char *setname, 
                                             int no, int *count, int *no_max);

YAZ_EXPORT void zebra_sort (ZebraHandle zh, ODR stream,
                            int num_input_setnames,
                            const char **input_setnames,
                            const char *output_setname,
                            Z_SortKeySpecList *sort_sequence,
                            int *sort_status);


YAZ_EXPORT
int zebra_select_databases (ZebraHandle zh, int num_bases, 
                            const char **basenames);

YAZ_EXPORT
int zebra_select_database (ZebraHandle zh, const char *basename);



YAZ_END_CDECL				      
#endif
