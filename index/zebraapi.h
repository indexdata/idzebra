/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebraapi.h,v $
 * Revision 1.1  1998-06-12 12:22:13  adam
 * Work on Zebra API.
 *
 */

#include <odr.h>
#include <oid.h>
#include <proto.h>

typedef struct {
    int errCode;
    char *errString;
    int position;
    char *buf;
    int len;
    oid_value format;
    char *base;
} ZebraRetrievalRecord;

typedef struct {
    int occurrences;
    char *term;
} ZebraScanEntry;

typedef struct zebra_info *ZebraHandle;

YAZ_EXPORT ZebraHandle zebra_open (const char *host);

YAZ_EXPORT void zebra_search_rpn (ZebraHandle zh, ODR stream,
		       Z_RPNQuery *query, int num_bases, char **basenames, 
		       const char *setname);

YAZ_EXPORT void zebra_records_retrieve (ZebraHandle zh, ODR stream,
			     const char *setname, Z_RecordComposition *comp,
			     oid_value input_format,
			     int num_recs, ZebraRetrievalRecord *recs);

YAZ_EXPORT void zebra_scan (ZebraHandle zh, ODR stream,
			    Z_AttributesPlusTerm *zapt,
			    oid_value attributeset,
			    int num_bases, char **basenames,
			    int *position, int *num_entries,
			    ZebraScanEntry **list,
			    int *is_partial);

YAZ_EXPORT void zebra_close (ZebraHandle zh);

YAZ_EXPORT int zebra_errCode (ZebraHandle zh);
YAZ_EXPORT const char *zebra_errString (ZebraHandle zh);
YAZ_EXPORT char *zebra_errAdd (ZebraHandle zh);
YAZ_EXPORT int zebra_hits (ZebraHandle zh);



