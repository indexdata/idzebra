/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zinfo.h,v $
 * Revision 1.5  1998-06-08 14:43:16  adam
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

#include <data1.h>
#include "recindex.h"

typedef struct zebraExplainInfo *ZebraExplainInfo;
typedef struct zebDatabaseInfo ZebDatabaseInfo;
ZebraExplainInfo zebraExplain_open (Records records, data1_handle dh,
				    Res res,
				    int writeFlag,
				    void *updateHandle,
				    int (*updateFunc)(void *handle,
						      Record drec,
						      data1_node *n));
void zebraExplain_close (ZebraExplainInfo zei, int writeFlag,
			 int (*updateH)(Record drec, data1_node *n));
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

typedef struct {
    int recordSize;
    int recordOffset;
    int runNumber;
} RecordAttr;
RecordAttr *rec_init_attr (ZebraExplainInfo zei, Record rec);

#endif
