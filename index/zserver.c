/*
 * Copyright (C) 1995-2000, Index Data 
 * All rights reserved.
 *
 * $Log: zserver.c,v $
 * Revision 1.78  2000-04-05 09:49:35  adam
 * On Unix, zebra/z'mbol uses automake.
 *
 * Revision 1.77  2000/03/20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.76  2000/03/15 15:00:31  adam
 * First work on threaded version.
 *
 * Revision 1.75  1999/11/30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.74  1999/11/29 15:13:26  adam
 * Server sets implementationName - and Version.
 *
 * Revision 1.73  1999/11/04 15:00:45  adam
 * Implemented delete result set(s).
 *
 * Revision 1.71  1999/07/14 10:59:26  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.70  1999/06/10 12:14:56  adam
 * Fixed to use bend_start instead of pre_init.
 *
 * Revision 1.69  1999/06/10 09:20:03  adam
 * Minor change to pre_init handler.
 *
 * Revision 1.68  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.67  1999/02/02 14:51:14  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.66  1998/10/28 10:54:41  adam
 * SDRKit integration.
 *
 * Revision 1.65  1998/10/18 07:54:54  adam
 * Additional info added for diagnostics 114 (Unsupported use attribute) and
 * 121 (Unsupported attribute set).
 *
 * Revision 1.64  1998/09/22 10:48:21  adam
 * Minor changes in search API.
 *
 * Revision 1.63  1998/09/02 13:53:21  adam
 * Extra parameter decode added to search routines to implement
 * persistent queries.
 *
 * Revision 1.62  1998/08/06 14:35:28  adam
 * Routine bend_deleterequest removed.
 *
 * Revision 1.61  1998/06/24 12:16:15  adam
 * Support for relations on text operands. Open range support in
 * DFA module (i.e. [-j], [g-]).
 *
 * Revision 1.60  1998/06/22 11:36:49  adam
 * Added authentication check facility to zebra.
 *
 * Revision 1.59  1998/06/12 12:22:13  adam
 * Work on Zebra API.
 *
 * Revision 1.58  1998/05/27 16:57:46  adam
 * Zebra returns surrogate diagnostic for single records when
 * appropriate.
 *
 * Revision 1.57  1998/04/03 14:45:18  adam
 * Fixed setting of last_in_set in bend_fetch.
 *
 * Revision 1.56  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.55  1998/02/10 12:03:06  adam
 * Implemented Sort.
 *
 * Revision 1.54  1998/01/29 13:39:13  adam
 * Compress ISAM is default.
 *
 * Revision 1.53  1998/01/12 15:04:09  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.52  1997/11/18 10:05:08  adam
 * Changed character map facility so that admin can specify character
 * mapping files for each register type, w, p, etc.
 *
 * Revision 1.51  1997/10/27 14:33:06  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.50  1997/09/29 09:08:36  adam
 * Revised locking system to be thread safe for the server.
 *
 * Revision 1.49  1997/09/25 14:57:23  adam
 * Windows NT port.
 *
 * Revision 1.48  1997/09/17 12:19:19  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.47  1997/09/04 13:58:36  adam
 * New retrieve/extract method tellf (added).
 * Added O_BINARY for open calls.
 *
 * Revision 1.46  1997/07/28 08:30:47  adam
 * Server returns diagnostic 14 when record doesn't exist.
 *
 * Revision 1.45  1996/12/23 15:30:45  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.44  1996/12/11 12:08:01  adam
 * Added better compression.
 *
 * Revision 1.43  1996/11/15 15:03:58  adam
 * Logging of execution speed by using the times(2) call.
 *
 * Revision 1.42  1996/11/08  11:10:36  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.41  1996/10/29 14:09:56  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.40  1996/06/04 10:19:02  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.39  1996/05/31  09:07:05  quinn
 * Work on character-set handling
 *
 * Revision 1.38  1996/05/14  11:34:01  adam
 * Scan support in multiple registers/databases.
 *
 * Revision 1.37  1996/05/14  06:16:48  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.36  1996/05/01 13:46:37  adam
 * First work on multiple records in one file.
 * New option, -offset, to the "unread" command in the filter module.
 *
 * Revision 1.35  1996/03/26  16:01:14  adam
 * New setting lockPath: directory of various lock files.
 *
 * Revision 1.34  1996/03/20  09:36:46  adam
 * Function dict_lookup_grep got extra parameter, init_pos, which marks
 * from which position in pattern approximate pattern matching should occur.
 * Approximate pattern matching is used in relevance=re-2.
 *
 * Revision 1.33  1996/01/17  14:57:56  adam
 * Prototype changed for reader functions in extract/retrieve. File
 *  is identified by 'void *' instead of 'int.
 *
 * Revision 1.32  1995/12/11  09:12:58  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.31  1995/12/08  16:22:56  adam
 * Work on update while servers are running. Three lock files introduced.
 * The servers reload their registers when necessary, but they don't
 * reestablish result sets yet.
 *
 * Revision 1.30  1995/12/07  17:38:48  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.29  1995/12/04  14:22:32  adam
 * Extra arg to recType_byName.
 * Started work on new regular expression parsed input to
 * structured records.
 *
 * Revision 1.28  1995/11/28  09:09:48  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.27  1995/11/27  13:58:54  adam
 * New option -t. storeStore data implemented in server.
 *
 * Revision 1.26  1995/11/25  10:24:07  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.25  1995/11/21  15:29:13  adam
 * Config file 'base' read by default by both indexer and server.
 *
 * Revision 1.24  1995/11/20  16:59:47  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.23  1995/11/16  17:00:56  adam
 * Better logging of rpn query.
 *
 * Revision 1.22  1995/11/16  15:34:55  adam
 * Uses new record management system in both indexer and server.
 *
 * Revision 1.21  1995/11/01  16:25:52  quinn
 * *** empty log message ***
 *
 * Revision 1.20  1995/10/27  14:00:12  adam
 * Implemented detection of database availability.
 *
 * Revision 1.19  1995/10/17  18:02:11  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.18  1995/10/16  14:03:09  quinn
 * Changes to support element set names and espec1
 *
 * Revision 1.17  1995/10/16  09:32:40  adam
 * More work on relational op.
 *
 * Revision 1.16  1995/10/13  12:26:44  adam
 * Optimization of truncation.
 *
 * Revision 1.15  1995/10/12  12:40:55  adam
 * Bug fixes in rpn_prox.
 *
 * Revision 1.14  1995/10/09  16:18:37  adam
 * Function dict_lookup_grep got extra client data parameter.
 *
 * Revision 1.13  1995/10/06  14:38:00  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.12  1995/10/06  13:52:06  adam
 * Bug fixes. Handler may abort further scanning.
 *
 * Revision 1.11  1995/10/06  10:43:57  adam
 * Scan added. 'occurrences' in scan entries not set yet.
 *
 * Revision 1.10  1995/10/02  16:43:32  quinn
 * Set default resulting record type in fetch.
 *
 * Revision 1.9  1995/10/02  15:18:52  adam
 * New member in recRetrieveCtrl: diagnostic.
 *
 * Revision 1.8  1995/09/28  09:19:47  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.7  1995/09/27  16:17:32  adam
 * More work on retrieve.
 *
 * Revision 1.6  1995/09/08  08:53:22  adam
 * Record buffer maintained in server_info.
 *
 * Revision 1.5  1995/09/06  16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.4  1995/09/06  10:33:04  adam
 * More work on present. Some log messages removed.
 *
 * Revision 1.3  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 * Revision 1.2  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/04  09:10:41  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include <yaz/data1.h>
#ifdef ASN_COMPILED
#include <yaz/ill.h>
#endif

#include "zserver.h"

#ifndef ZEBRASDR
#define ZEBRASDR 0
#endif
#if ZEBRASDR
#include "zebrasdr.h"
#endif

static int bend_sort (void *handle, bend_sort_rr *rr);
static int bend_delete (void *handle, bend_delete_rr *rr);
static int bend_esrequest (void *handle, bend_esrequest_rr *rr);
static int bend_segment (void *handle, bend_segment_rr *rr);
static int bend_search (void *handle, bend_search_rr *r);
static int bend_fetch (void *handle, bend_fetch_rr *r);
static int bend_scan (void *handle, bend_scan_rr *r);

bend_initresult *bend_init (bend_initrequest *q)
{
    bend_initresult *r = (bend_initresult *)
	odr_malloc (q->stream, sizeof(*r));
    ZebraHandle zh;
    struct statserv_options_block *sob;
    char *user = NULL;
    char *passwd = NULL;

    r->errcode = 0;
    r->errstring = 0;
    q->bend_sort = bend_sort;
    q->bend_delete = bend_delete;
    q->bend_esrequest = bend_esrequest;
    q->bend_segment = bend_segment;
    q->bend_search = bend_search;
    q->bend_fetch = bend_fetch;
    q->bend_scan = bend_scan;

    q->implementation_name = "Z'mbol Information Server";
    q->implementation_version = "Z'mbol 1.0";

    logf (LOG_DEBUG, "bend_init");

    sob = statserv_getcontrol ();
    if (!(zh = zebra_open (sob->handle)))
    {
	logf (LOG_FATAL, "Failed to open Zebra `%s'", sob->configname);
	r->errcode = 1;
	return r;
    }
    if (q->auth)
    {
	if (q->auth->which == Z_IdAuthentication_open)
	{
	    char *openpass = xstrdup (q->auth->u.open);
	    char *cp = strchr (openpass, '/');
	    if (cp)
	    {
		*cp = '\0';
		user = nmem_strdup (odr_getmem (q->stream), openpass);
		passwd = nmem_strdup (odr_getmem (q->stream), cp+1);
	    }
	    xfree (openpass);
	}
    }
    if (zebra_auth (zh->service, user, passwd))
    {
	r->errcode = 222;
	r->errstring = user;
	zebra_close (zh);
	return r;
    }
    r->handle = zh;
    return r;
}

int bend_search (void *handle, bend_search_rr *r)
{
    ZebraHandle zh = (ZebraHandle) handle;

    r->hits = 0;
    r->errcode = 0;
    r->errstring = NULL;
    
    logf (LOG_LOG, "ResultSet '%s'", r->setname);
    switch (r->query->which)
    {
    case Z_Query_type_1: case Z_Query_type_101:
	zebra_search_rpn (zh, r->decode, r->stream, r->query->u.type_1,
			  r->num_bases, r->basenames, r->setname);
	r->errcode = zh->errCode;
	r->errstring = zh->errString;
	r->hits = zh->hits;
        break;
    case Z_Query_type_2:
	r->errcode = 107;
	r->errstring = "type-2";
	break;
    default:
        r->errcode = 107;
    }
    return 0;
}

int bend_fetch (void *handle, bend_fetch_rr *r)
{
    ZebraHandle zh = (ZebraHandle) handle;
    ZebraRetrievalRecord retrievalRecord;

    retrievalRecord.position = r->number;
    
    r->last_in_set = 0;
    zebra_records_retrieve (zh, r->stream, r->setname, r->comp,
			    r->request_format, 1, &retrievalRecord);
    if (zh->errCode)                  /* non Surrogate Diagnostic */
    {
	r->errcode = zh->errCode;
	r->errstring = zh->errString;
    }
    else if (retrievalRecord.errCode) /* Surrogate Diagnostic */
    {
	r->surrogate_flag = 1;
	r->errcode = retrievalRecord.errCode;
	r->errstring = retrievalRecord.errString;
	r->basename = retrievalRecord.base;
    }
    else                              /* Database Record */
    {
	r->errcode = 0;
	r->basename = retrievalRecord.base;
	r->record = retrievalRecord.buf;
	r->len = retrievalRecord.len;
	r->output_format = retrievalRecord.format;
    }
    return 0;
}

static int bend_scan (void *handle, bend_scan_rr *r)
{
    ZebraScanEntry *entries;
    ZebraHandle zh = (ZebraHandle) handle;
    int is_partial, i;
    
    r->entries = (struct scan_entry *)
	odr_malloc (r->stream, sizeof(*r->entries) * r->num_entries);
    zebra_scan (zh, r->stream, r->term,
		r->attributeset,
		r->num_bases, r->basenames,
		&r->term_position,
		&r->num_entries, &entries, &is_partial);
    if (is_partial)
	r->status = BEND_SCAN_PARTIAL;
    else
	r->status = BEND_SCAN_SUCCESS;
    for (i = 0; i < r->num_entries; i++)
    {
	r->entries[i].term = entries[i].term;
	r->entries[i].occurrences = entries[i].occurrences;
    }
    r->errcode = zh->errCode;
    r->errstring = zh->errString;
    return 0;
}

void bend_close (void *handle)
{
    zebra_close ((ZebraHandle) handle);
}

int bend_sort (void *handle, bend_sort_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;

    zebra_sort (zh, rr->stream,
                rr->num_input_setnames, (const char **) rr->input_setnames,
		rr->output_setname, rr->sort_sequence, &rr->sort_status);
    rr->errcode = zh->errCode;
    rr->errstring = zh->errString;
    return 0;
}

int bend_delete (void *handle, bend_delete_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;

    rr->delete_status =	zebra_deleleResultSet(zh, rr->function,
					      rr->num_setnames, rr->setnames,
					      rr->statuses);
    return 0;
}

static int es_admin_request (ZebraHandle zh, Z_AdminEsRequest *r)
{
    switch (r->toKeep->which)
    {
    case Z_ESAdminOriginPartToKeep_reIndex:
	yaz_log(LOG_LOG, "adm-reindex");
	break;
    case Z_ESAdminOriginPartToKeep_truncate:
	yaz_log(LOG_LOG, "adm-truncate");
	break;
    case Z_ESAdminOriginPartToKeep_drop:
	yaz_log(LOG_LOG, "adm-drop");
	break;
    case Z_ESAdminOriginPartToKeep_create:
	yaz_log(LOG_LOG, "adm-create");
	zebra_admin_create (zh, r->toKeep->databaseName);
	break;
    case Z_ESAdminOriginPartToKeep_import:
	yaz_log(LOG_LOG, "adm-import");
	zebra_admin_import_begin (zh, r->toKeep->databaseName);
	break;
    case Z_ESAdminOriginPartToKeep_refresh:
	yaz_log(LOG_LOG, "adm-refresh");
	break;
    case Z_ESAdminOriginPartToKeep_commit:
	yaz_log(LOG_LOG, "adm-commit");
	break;
    case Z_ESAdminOriginPartToKeep_shutdown:
	yaz_log(LOG_LOG, "shutdown");
	zebra_admin_shutdown(zh);
	break;
    case Z_ESAdminOriginPartToKeep_start:
	yaz_log(LOG_LOG, "start");
	zebra_admin_start(zh);
	break;
    default:
	yaz_log(LOG_LOG, "unknown admin");
	zh->errCode = 1001;
    }
    if (r->toKeep->databaseName)
    {
	yaz_log(LOG_LOG, "database %s", r->toKeep->databaseName);
    }
    return 0;
}

static int es_admin (ZebraHandle zh, Z_Admin *r)
{
    switch (r->which)
    {
    case Z_Admin_esRequest:
	es_admin_request (zh, r->u.esRequest);
	break;
    case Z_Admin_taskPackage:
	yaz_log (LOG_LOG, "adm taskpackage (unhandled)");
	break;
    default:
	zh->errCode = 1001;
	break;
    }

    return 0;
}

int bend_segment (void *handle, bend_segment_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;
    Z_Segment *segment = rr->segment;

    if (segment->num_segmentRecords)
	zebra_admin_import_segment (zh, rr->segment);
    else
	zebra_admin_import_end (zh);
    return 0;
}

int bend_esrequest (void *handle, bend_esrequest_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;
    
    yaz_log(LOG_LOG, "function: %d", *rr->esr->function);
    if (rr->esr->packageName)
    	yaz_log(LOG_LOG, "packagename: %s", rr->esr->packageName);
    yaz_log(LOG_LOG, "Waitaction: %d", *rr->esr->waitAction);

    if (!rr->esr->taskSpecificParameters)
    {
        yaz_log (LOG_WARN, "No task specific parameters");
    }
    else if (rr->esr->taskSpecificParameters->which == Z_External_ESAdmin)
    {
	es_admin (zh, rr->esr->taskSpecificParameters->u.adminService);
	rr->errcode = zh->errCode;
	rr->errstring = zh->errString;
    }
    else if (rr->esr->taskSpecificParameters->which == Z_External_itemOrder)
    {
    	Z_ItemOrder *it = rr->esr->taskSpecificParameters->u.itemOrder;
	yaz_log (LOG_LOG, "Received ItemOrder");
	switch (it->which)
	{
#ifdef ASN_COMPILED
	case Z_IOItemOrder_esRequest:
#else
	case Z_ItemOrder_esRequest:
#endif
	{
	    Z_IORequest *ir = it->u.esRequest;
	    Z_IOOriginPartToKeep *k = ir->toKeep;
	    Z_IOOriginPartNotToKeep *n = ir->notToKeep;
	    
	    if (k && k->contact)
	    {
	        if (k->contact->name)
		    yaz_log(LOG_LOG, "contact name %s", k->contact->name);
		if (k->contact->phone)
		    yaz_log(LOG_LOG, "contact phone %s", k->contact->phone);
		if (k->contact->email)
		    yaz_log(LOG_LOG, "contact email %s", k->contact->email);
	    }
	    if (k->addlBilling)
	    {
	        yaz_log(LOG_LOG, "Billing info (not shown)");
	    }
	    
	    if (n->resultSetItem)
	    {
	        yaz_log(LOG_LOG, "resultsetItem");
		yaz_log(LOG_LOG, "setId: %s", n->resultSetItem->resultSetId);
		yaz_log(LOG_LOG, "item: %d", *n->resultSetItem->item);
	    }
#ifdef ASN_COMPILED
	    if (n->itemRequest)
	    {
		Z_External *r = (Z_External*) n->itemRequest;
		ILL_ItemRequest *item_req = 0;
		ILL_Request *ill_req = 0;
		if (r->direct_reference)
		{
		    oident *ent = oid_getentbyoid(r->direct_reference);
		    if (ent)
			yaz_log(LOG_LOG, "OID %s", ent->desc);
		    if (ent && ent->value == VAL_ISO_ILL_1)
		    {
			yaz_log (LOG_LOG, "ItemRequest");
			if (r->which == ODR_EXTERNAL_single)
			{
			    odr_setbuf(rr->decode,
				       r->u.single_ASN1_type->buf,
				       r->u.single_ASN1_type->len, 0);
			    
			    if (!ill_ItemRequest (rr->decode, &item_req, 0, 0))
			    {
				yaz_log (LOG_LOG,
                                    "Couldn't decode ItemRequest %s near %d",
                                       odr_errmsg(odr_geterror(rr->decode)),
                                       odr_offset(rr->decode));
                                yaz_log(LOG_LOG, "PDU dump:");
                                odr_dumpBER(log_file(),
                                     r->u.single_ASN1_type->buf,
                                     r->u.single_ASN1_type->len);
                            }
			    if (rr->print)
			    {
				ill_ItemRequest (rr->print, &item_req, 0,
                                    "ItemRequest");
				odr_reset (rr->print);
 			    }
			}
			if (!item_req && r->which == ODR_EXTERNAL_single)
			{
			    yaz_log (LOG_LOG, "ILLRequest");
			    odr_setbuf(rr->decode,
				       r->u.single_ASN1_type->buf,
				       r->u.single_ASN1_type->len, 0);
			    
			    if (!ill_Request (rr->decode, &ill_req, 0, 0))
			    {
				yaz_log (LOG_LOG,
                                    "Couldn't decode ILLRequest %s near %d",
                                       odr_errmsg(odr_geterror(rr->decode)),
                                       odr_offset(rr->decode));
                                yaz_log(LOG_LOG, "PDU dump:");
                                odr_dumpBER(log_file(),
                                     r->u.single_ASN1_type->buf,
                                     r->u.single_ASN1_type->len);
                            }
			    if (rr->print)
                            {
				ill_Request (rr->print, &ill_req, 0,
                                    "ILLRequest");
				odr_reset (rr->print);
			    }
			}
		    }
		}
		if (item_req)
		{
		    yaz_log (LOG_LOG, "ILL protocol version = %d",
			     *item_req->protocol_version_num);
		}
	    }
#endif
	}
	break;
	}
    }
    else if (rr->esr->taskSpecificParameters->which == Z_External_update)
    {
    	Z_IUUpdate *up = rr->esr->taskSpecificParameters->u.update;
	yaz_log (LOG_LOG, "Received DB Update");
	if (up->which == Z_IUUpdate_esRequest)
	{
	    Z_IUUpdateEsRequest *esRequest = up->u.esRequest;
	    Z_IUOriginPartToKeep *toKeep = esRequest->toKeep;
	    Z_IUSuppliedRecords *notToKeep = esRequest->notToKeep;
	    
	    yaz_log (LOG_LOG, "action");
	    if (toKeep->action)
	    {
		switch (*toKeep->action)
		{
		case Z_IUOriginPartToKeep_recordInsert:
		    yaz_log (LOG_LOG, "recordInsert");
		    break;
		case Z_IUOriginPartToKeep_recordReplace:
		    yaz_log (LOG_LOG, "recordUpdate");
		    break;
		case Z_IUOriginPartToKeep_recordDelete:
		    yaz_log (LOG_LOG, "recordDelete");
		    break;
		case Z_IUOriginPartToKeep_elementUpdate:
		    yaz_log (LOG_LOG, "elementUpdate");
		    break;
		case Z_IUOriginPartToKeep_specialUpdate:
		    yaz_log (LOG_LOG, "specialUpdate");
		    break;
                case Z_ESAdminOriginPartToKeep_shutdown:
		    yaz_log (LOG_LOG, "shutDown");
		    break;
		case Z_ESAdminOriginPartToKeep_start:
		    yaz_log (LOG_LOG, "start");
		    break;
		default:
		    yaz_log (LOG_LOG, " unknown (%d)", *toKeep->action);
		}
	    }
	    if (toKeep->databaseName)
	    {
		yaz_log (LOG_LOG, "database: %s", toKeep->databaseName);
		if (!strcmp(toKeep->databaseName, "fault"))
		{
		    rr->errcode = 109;
		    rr->errstring = toKeep->databaseName;
		}
		if (!strcmp(toKeep->databaseName, "accept"))
		    rr->errcode = -1;
	    }
	    if (notToKeep)
	    {
		int i;
		for (i = 0; i < notToKeep->num; i++)
		{
		    Z_External *rec = notToKeep->elements[i]->record;

		    if (rec->direct_reference)
		    {
			struct oident *oident;
			oident = oid_getentbyoid(rec->direct_reference);
			if (oident)
			    yaz_log (LOG_LOG, "record %d type %s", i,
				     oident->desc);
		    }
		    switch (rec->which)
		    {
		    case Z_External_sutrs:
			if (rec->u.octet_aligned->len > 170)
			    yaz_log (LOG_LOG, "%d bytes:\n%.168s ...",
				     rec->u.sutrs->len,
				     rec->u.sutrs->buf);
			else
			    yaz_log (LOG_LOG, "%d bytes:\n%s",
				     rec->u.sutrs->len,
				     rec->u.sutrs->buf);
                        break;
		    case Z_External_octet        :
			if (rec->u.octet_aligned->len > 170)
			    yaz_log (LOG_LOG, "%d bytes:\n%.168s ...",
				     rec->u.octet_aligned->len,
				     rec->u.octet_aligned->buf);
			else
			    yaz_log (LOG_LOG, "%d bytes\n%s",
				     rec->u.octet_aligned->len,
				     rec->u.octet_aligned->buf);
		    }
		}
	    }
	}
    }
    else
    {
        yaz_log (LOG_WARN, "Unknown Extended Service(%d)",
		 rr->esr->taskSpecificParameters->which);
	
    }
    return 0;
}

static void bend_start (struct statserv_options_block *sob)
{
#ifdef WIN32
    
#else
    if (!sob->inetd) 
    {
        char *pidfile = "zebrasrv.pid";
        int fd = creat (pidfile, 0666);
        if (fd == -1)
            logf (LOG_WARN|LOG_ERRNO, "creat %s", pidfile);
        else
        {
	    char pidstr[30];
	
	    sprintf (pidstr, "%ld", (long) getpid ());
	    write (fd, pidstr, strlen(pidstr));
	    close (fd);
        }
    }
#endif
    if (sob->handle)
	zebra_stop((ZebraService) sob->handle);
    sob->handle = zebra_start(sob->configname);
}

static void bend_stop(struct statserv_options_block *sob)
{
    if (sob->handle)
    {
	ZebraService service = sob->handle;
	zebra_stop(service);
    }
}

int main (int argc, char **argv)
{
    struct statserv_options_block *sob;

    sob = statserv_getcontrol ();
    strcpy (sob->configname, FNAME_CONFIG);
    sob->bend_start = bend_start;
    sob->bend_stop = bend_stop;

    statserv_setcontrol (sob);

    return statserv_main (argc, argv, bend_init, bend_close);
}
