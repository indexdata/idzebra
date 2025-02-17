/* This file is part of the Zebra server.
   Copyright (C) Index Data

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


#ifndef ZINFO_H
#define ZINFO_H

#include <d1_absyn.h>
#include <idzebra/api.h>
#include "recindex.h"

YAZ_BEGIN_CDECL

typedef enum {
    zinfo_index_category_index,
    zinfo_index_category_sort,
    zinfo_index_category_alwaysmatches,
    zinfo_index_category_anchor
} zinfo_index_category_t;

typedef ZEBRA_RES ZebraExplainUpdateFunc(void *handle,
                                         Record drec,
                                         data1_node *n);

typedef struct zebraExplainInfo *ZebraExplainInfo;
typedef struct zebDatabaseInfo ZebDatabaseInfo;
ZebraExplainInfo zebraExplain_open(Records records, data1_handle dh,
                                   Res res,
                                   int writeFlag,
                                   void *updateHandle,
                                   ZebraExplainUpdateFunc *);

void zebraExplain_close(ZebraExplainInfo zei);
int zebraExplain_curDatabase(ZebraExplainInfo zei, const char *database);
int zebraExplain_newDatabase(ZebraExplainInfo zei, const char *database,
                              int explain_database);
int zebraExplain_add_attr_su(ZebraExplainInfo zei, int index_type,
                             int set, int use);

/** \brief lookup ordinal from string index + index type
    \param zei explain info
    \param cat category
    \param index_type index type
    \param str index string
    \returns  -1 no such index+type exist; ordinal otherwise
*/

int zebraExplain_lookup_attr_str(ZebraExplainInfo zei,
                                 zinfo_index_category_t cat,
                                 const char *index_type,
                                 const char *str);
int zebraExplain_add_attr_str(ZebraExplainInfo zei,
                              zinfo_index_category_t cat,
                              const char *index_type,
                              const char *str);
void zebraExplain_addSchema(ZebraExplainInfo zei, Odr_oid *oid);
void zebraExplain_recordCountIncrement(ZebraExplainInfo zei, int adjust_num);
void zebraExplain_recordBytesIncrement(ZebraExplainInfo zei, int adjust_num);
zint zebraExplain_runNumberIncrement(ZebraExplainInfo zei, int adjust_num);
void zebraExplain_loadAttsets(data1_handle dh, Res res);
void zebraExplain_flush(ZebraExplainInfo zei, void *updateHandle);

int zebraExplain_lookup_ord(ZebraExplainInfo zei, int ord,
                             const char **index_type, const char **db,
                             const char **string_index);

int zebraExplain_ord_adjust_occurrences(ZebraExplainInfo zei, int ord,
                                        int term_delta, int doc_delta);

zint zebraExplain_ord_get_term_occurrences(ZebraExplainInfo zei, int ord);
zint zebraExplain_ord_get_doc_occurrences(ZebraExplainInfo zei, int ord);

int zebraExplain_trav_ord(ZebraExplainInfo zei, void *handle,
                          int (*f)(void *handle, int ord,
                                   const char *index_type,
                                   const char *string_index,
                                   zinfo_index_category_t cat));

int zebraExplain_get_database_ord(ZebraExplainInfo zei);
int zebraExplain_removeDatabase(ZebraExplainInfo zei, void *updateHandle);

typedef struct {
    int recordSize;
    off_t recordOffset;
    zint runNumber;
    zint staticrank;
} RecordAttr;
RecordAttr *rec_init_attr(ZebraExplainInfo zei, Record rec);

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

