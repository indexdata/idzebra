/*
 * Copyright (C) 1994-1998, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebraapi.h,v $
 * Revision 1.12  2000-04-05 09:49:35  adam
 * On Unix, zebra/z'mbol uses automake.
 *
 * Revision 1.11  2000/03/20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.10  2000/03/15 15:00:31  adam
 * First work on threaded version.
 *
 * Revision 1.9  2000/02/24 12:31:17  adam
 * Added zebra_string_norm.
 *
 * Revision 1.8  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.7  1999/11/04 15:00:45  adam
 * Implemented delete result set(s).
 *
 * Revision 1.6  1999/02/17 11:29:57  adam
 * Fixed in record_fetch. Minor updates to API.
 *
 * Revision 1.5  1998/09/22 10:48:19  adam
 * Minor changes in search API.
 *
 * Revision 1.4  1998/09/02 13:53:18  adam
 * Extra parameter decode added to search routines to implement
 * persistent queries.
 *
 * Revision 1.3  1998/06/22 11:36:48  adam
 * Added authentication check facility to zebra.
 *
 * Revision 1.2  1998/06/13 00:14:09  adam
 * Minor changes.
 *
 * Revision 1.1  1998/06/12 12:22:13  adam
 * Work on Zebra API.
 *
 */

#include <yaz/odr.h>
#include <yaz/oid.h>
#include <yaz/proto.h>

YAZ_BEGIN_CDECL

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
                       Z_RPNQuery *query, int num_bases, char **basenames, 
		       const char *setname);

/* Retrieve record(s) */
YAZ_EXPORT void zebra_records_retrieve (ZebraHandle zh, ODR stream,
		       const char *setname, Z_RecordComposition *comp,
		       oid_value input_format,
		       int num_recs, ZebraRetrievalRecord *recs);

/* Browse */
YAZ_EXPORT void zebra_scan (ZebraHandle zh, ODR stream,
			    Z_AttributesPlusTerm *zapt,
			    oid_value attributeset,
			    int num_bases, char **basenames,
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

/* number of hits (after search) */
YAZ_EXPORT int zebra_hits (ZebraHandle zh);

/* do authentication */
YAZ_EXPORT int zebra_auth (ZebraService zh, const char *user, const char *pass);

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
YAZ_END_CDECL				      
