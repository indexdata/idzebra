/* $Id: zebraapi.h,v 1.27 2003-02-28 20:12:18 pop Exp $
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#ifndef ZEBRAAPI_H
#define ZEBRAAPI_H

#include <yaz/odr.h>
#include <yaz/oid.h>
#include <yaz/proto.h>
#include <zebraver.h>

/* Fixme! Compare string (ignore case) */
#ifdef WIN32
#define STRCASECMP	stricmp
#else
#define STRCASECMP	strcasecmp
#endif

YAZ_BEGIN_CDECL

struct recordGroup {
    char  *groupName;
    char  *databaseName;
    char  *path;
    char  *recordId;
    char  *recordType;
    int   flagStoreData;
    int   flagStoreKeys;
    int   flagRw;
    int   fileVerboseLimit;
    int   databaseNamePath;
    int   explainDatabase;
    int   followLinks;
};

typedef struct {
  int processed;
  int inserted;
  int updated;
  int deleted;
  long utime;
  long stime;
} ZebraTransactionStatus;

/* Retrieval Record Descriptor */
typedef struct {
    int errCode;         /* non-zero if error when fetching this */
    char *errString;     /* error string */
    int position;        /* position of record in result set (1,2,..) */
    char *buf;           /* record buffer (void pointer really) */
    int len;             /* length */
    oid_value format;    /* record syntax */
    char *base; 
    int  sysno;
    int  score;
} ZebraRetrievalRecord;

/* Scan Term Descriptor */
typedef struct {
    int occurrences;     /* scan term occurrences */
    char *term;          /* scan term string */
} ZebraScanEntry;

typedef struct zebra_session *ZebraHandle;
typedef struct zebra_service *ZebraService;


/******
 * Starting and stopping 
 */

/* Start Zebra using file 'configName' (usually zebra.cfg) */
/* There should be exactly one ZebraService */
YAZ_EXPORT ZebraService zebra_start (const char *configName);

/* Close the whole Zebra */
YAZ_EXPORT void zebra_stop (ZebraService zs);


/* Open a ZebraHandle */
/* There should be one handle for each thred doing something */
/* with zebra, be that searching or indexing. In simple apps */
/* one handle is sufficient */
YAZ_EXPORT ZebraHandle zebra_open (ZebraService zs);

/* Close handle */
YAZ_EXPORT void zebra_close (ZebraHandle zh);

/*********
 * Error handling 
 */

/* last error code */
YAZ_EXPORT int zebra_errCode (ZebraHandle zh);

/* string representatio of above */
YAZ_EXPORT const char *zebra_errString (ZebraHandle zh);

/* extra information associated with error */
YAZ_EXPORT char *zebra_errAdd (ZebraHandle zh);

/* get the result code and addinfo from zh */
YAZ_EXPORT void zebra_result (ZebraHandle zh, int *code, char **addinfo);
/* FIXME - why is this needed?? -H */


/**************
 * Searching 
 */

/* Search using RPN-Query */
YAZ_EXPORT void zebra_search_rpn (ZebraHandle zh, ODR input, ODR output,
                                  Z_RPNQuery *query,
                                  const char *setname, int *hits);

/* Retrieve record(s) */
YAZ_EXPORT void zebra_records_retrieve (ZebraHandle zh, ODR stream,
		       const char *setname, Z_RecordComposition *comp,
		       oid_value input_format,
		       int num_recs, ZebraRetrievalRecord *recs);

/* Delete Result Set(s) */
YAZ_EXPORT int zebra_deleleResultSet(ZebraHandle zh, int function,
				     int num_setnames, char **setnames,
				     int *statuses);


/* Browse */
YAZ_EXPORT void zebra_scan (ZebraHandle zh, ODR stream,
			    Z_AttributesPlusTerm *zapt,
			    oid_value attributeset,
			    int *position, int *num_entries,
			    ZebraScanEntry **list,
			    int *is_partial);

   
          
/*********
 * Other 
 */
                      
/* do authentication */
YAZ_EXPORT int zebra_auth (ZebraHandle zh, const char *user, const char *pass);

/* Character normalisation on specific register .
   This routine is subject to change - do not use. */
YAZ_EXPORT int zebra_string_norm (ZebraHandle zh, unsigned reg_id,
				  const char *input_str, int input_len,
				  char *output_str, int output_len);


/******
 * Admin 
 */                   
          
YAZ_EXPORT void zebra_admin_create (ZebraHandle zh, const char *db);


YAZ_EXPORT void zebra_admin_shutdown (ZebraHandle zh);
YAZ_EXPORT void zebra_admin_start (ZebraHandle zh);

YAZ_EXPORT void zebra_shutdown (ZebraService zs);

YAZ_EXPORT void zebra_admin_import_begin (ZebraHandle zh, const char *database,
                                          const char *record_type);

YAZ_EXPORT void zebra_admin_import_segment (ZebraHandle zh,
					    Z_Segment *segment);

void zebra_admin_import_end (ZebraHandle zh);

int zebra_admin_exchange_record (ZebraHandle zh,
                                 const char *database,
                                 const char *rec_buf,
                                 size_t rec_len,
                                 const char *recid_buf, size_t recid_len,
                                 int action);

void zebra_begin_trans (ZebraHandle zh);
void zebra_end_trans (ZebraHandle zh);
void zebra_end_transaction (ZebraHandle zh, ZebraTransactionStatus *stat);

int zebra_commit (ZebraHandle zh);

int zebra_init (ZebraHandle zh);
int zebra_compact (ZebraHandle zh);
void zebra_repository_update (ZebraHandle zh);
void zebra_repository_delete (ZebraHandle zh);
void zebra_repository_show (ZebraHandle zh);
int zebra_record_insert (ZebraHandle zh, const char *buf, int len);

YAZ_EXPORT void zebra_set_group (ZebraHandle zh, struct recordGroup *rg);


YAZ_EXPORT int zebra_resultSetTerms (ZebraHandle zh, const char *setname, 
                                     int no, int *count, 
                                     int *type, char *out, size_t *len);

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

YAZ_EXPORT
void zebra_shadow_enable (ZebraHandle zh, int value);

YAZ_EXPORT
void zebra_register_statistics (ZebraHandle zh, int dumpdict);

YAZ_EXPORT
int zebra_record_encoding (ZebraHandle zh, const char *encoding);

/* Resources */
YAZ_EXPORT
void zebra_set_resource(ZebraHandle zh, const char *name, const char *value);
YAZ_EXPORT
const char *zebra_get_resource(ZebraHandle zh, 
		const char *name, const char *defaultvalue);


YAZ_END_CDECL				      
#endif
