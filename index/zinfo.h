/* $Id: zinfo.h,v 1.31 2006-05-10 12:31:09 adam Exp $
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


#ifndef ZINFO_H
#define ZINFO_H

#include <d1_absyn.h>
#include <idzebra/api.h>
#include "recindex.h"

/* Compare string (ignore case) */
#ifdef WIN32
#define STRCASECMP	stricmp
#else
#define STRCASECMP	strcasecmp
#endif

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
int zebraExplain_lookup_attr_su(ZebraExplainInfo zei, int index_type,
				int set, int use);
int zebraExplain_lookup_attr_su_any_index(ZebraExplainInfo zei,
					  int set, int use);
int zebraExplain_add_attr_su(ZebraExplainInfo zei, int index_type,
			     int set, int use);
int zebraExplain_lookup_attr_str(ZebraExplainInfo zei, int index_type,
				 const char *str);
int zebraExplain_add_attr_str(ZebraExplainInfo zei, int index_type,
			      const char *str);
void zebraExplain_addSchema (ZebraExplainInfo zei, Odr_oid *oid);
void zebraExplain_recordCountIncrement (ZebraExplainInfo zei, int adjust_num);
void zebraExplain_recordBytesIncrement (ZebraExplainInfo zei, int adjust_num);
zint zebraExplain_runNumberIncrement (ZebraExplainInfo zei, int adjust_num);
void zebraExplain_loadAttsets (data1_handle dh, Res res);
void zebraExplain_flush (ZebraExplainInfo zei, void *updateHandle);

int zebraExplain_lookup_ord (ZebraExplainInfo zei, int ord,
			     int *index_type, const char **db,
			     int *set, int *use, const char **string_index);

int zebraExplain_ord_adjust_occurrences(ZebraExplainInfo zei, int ord,
                                        int term_delta, int doc_delta);

int zebraExplain_ord_get_occurrences(ZebraExplainInfo zei, int ord,
                                     zint *term_occurrences,
                                     zint *doc_occurrences);

int zebraExplain_trav_ord(ZebraExplainInfo zei, void *handle,
			  int (*f)(void *handle, int ord));

int zebraExplain_get_database_ord(ZebraExplainInfo zei);
int zebraExplain_removeDatabase(ZebraExplainInfo zei, void *updateHandle);

typedef struct {
    int recordSize;
    off_t recordOffset;
    zint runNumber;
    zint staticrank;
} RecordAttr;
RecordAttr *rec_init_attr (ZebraExplainInfo zei, Record rec);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

