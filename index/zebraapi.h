/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebraapi.h,v $
 * Revision 1.3  1998-06-22 11:36:48  adam
 * Added authentication check facility to zebra.
 *
 * Revision 1.2  1998/06/13 00:14:09  adam
 * Minor changes.
 *
 * Revision 1.1  1998/06/12 12:22:13  adam
 * Work on Zebra API.
 *
 */

#include <odr.h>
#include <oid.h>
#include <proto.h>

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

typedef struct zebra_info *ZebraHandle;

/* Open Zebra using file 'configName' (usually zebra.cfg) */
YAZ_EXPORT ZebraHandle zebra_open (const char *configName);

/* Search using RPN-Query */
YAZ_EXPORT void zebra_search_rpn (ZebraHandle zh, ODR stream,
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
YAZ_EXPORT int zebra_auth (ZebraHandle zh, const char *user, const char *pass);

