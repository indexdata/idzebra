/*
 * Copyright (C) 1995-2000, Index Data 
 * All rights reserved.
 *
 * $Id: zserver.c,v 1.83 2002-02-20 17:30:01 adam Exp $
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

#if ZMBOL
    q->implementation_name = "Z'mbol Information Server";
    q->implementation_version = "Z'mbol " ZEBRAVER;
#else
    q->implementation_name = "Zebra Information Server";
    q->implementation_version = "Zebra " ZEBRAVER;
#endif

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
    xmalloc_trav("bend_close");
    nmem_print_list();
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
                                odr_dumpBER(yaz_log_file(),
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
                                odr_dumpBER(yaz_log_file(),
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
		    case Z_External_octet:
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
