/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zinfo.h,v $
 * Revision 1.13  2002-05-07 11:05:19  adam
 * data1 updates. Run number fix
 *
 * Revision 1.12  2002/02/20 17:30:01  adam
 * Work on new API. Locking system re-implemented
 *
 * Revision 1.11  2001/10/15 19:53:43  adam
 * POSIX thread updates. First work on term sets.
 *
 * Revision 1.10  2000/05/15 12:56:37  adam
 * Record offset of size off_t.
 *
 * Revision 1.9  2000/03/20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.8  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.7  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.6  1999/02/02 14:51:12  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.5  1998/06/08 14:43:16  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.4  1998/05/20 10:12:21  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.3  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.2  1996/05/22 08:22:00  adam
 * Added public ZebDatabaseInfo structure.
 *
 * Revision 1.1  1996/05/13 14:23:07  adam
 * Work on compaction of set/use bytes in dictionary.
 *
 */
#ifndef ZINFO_H
#define ZINFO_H

#include <yaz/data1.h>
#include "zebraapi.h"
#include "recindex.h"

YAZ_BEGIN_CDECL

typedef struct zebraExplainInfo *ZebraExplainInfo;
typedef struct zebDatabaseInfo ZebDatabaseInfo;
ZebraExplainInfo zebraExplain_open (Records records, data1_handle dh,
				    Res res,
				    int writeFlag,
				    void *updateHandle,
				    int (*updateFunc)(void *handle,
						      Record drec,
						      data1_node *n));
void zebraExplain_close (ZebraExplainInfo zei);
int zebraExplain_curDatabase (ZebraExplainInfo zei, const char *database);
int zebraExplain_newDatabase (ZebraExplainInfo zei, const char *database,
			      int explain_database);
int zebraExplain_lookupSU (ZebraExplainInfo zei, int set, int use);
int zebraExplain_addSU (ZebraExplainInfo zei, int set, int use);
void zebraExplain_addSchema (ZebraExplainInfo zei, Odr_oid *oid);
void zebraExplain_recordCountIncrement (ZebraExplainInfo zei, int adjust_num);
void zebraExplain_recordBytesIncrement (ZebraExplainInfo zei, int adjust_num);
int zebraExplain_runNumberIncrement (ZebraExplainInfo zei, int adjust_num);
void zebraExplain_loadAttsets (data1_handle dh, Res res);
void zebraExplain_flush (ZebraExplainInfo zei, void *updateHandle);

int zebraExplain_lookup_ord (ZebraExplainInfo zei, int ord,
			     const char **db, int *set, int *use);

typedef struct {
    int recordSize;
    off_t recordOffset;
    int runNumber;
} RecordAttr;
RecordAttr *rec_init_attr (ZebraExplainInfo zei, Record rec);

YAZ_END_CDECL

#endif
