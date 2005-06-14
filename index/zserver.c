/* $Id: zserver.c,v 1.136 2005-06-14 20:28:54 adam Exp $
   Copyright (C) 1995-2005
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#include <sys/locking.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <yaz/log.h>
#include <yaz/ill.h>
#include <yaz/yaz-util.h>
#include <yaz/diagbib1.h>

#include <sys/types.h>

#include "zserver.h"

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

    q->implementation_name = "Zebra Information Server";
    q->implementation_version = "Zebra " ZEBRAVER;

    yaz_log (YLOG_DEBUG, "bend_init");

    sob = statserv_getcontrol ();
    if (!(zh = zebra_open (sob->handle)))
    {
	yaz_log (YLOG_WARN, "Failed to read config `%s'", sob->configname);
	r->errcode = YAZ_BIB1_PERMANENT_SYSTEM_ERROR;
	return r;
    }
    r->handle = zh;
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
	else if (q->auth->which == Z_IdAuthentication_idPass)
	{
	    Z_IdPass *idPass = q->auth->u.idPass;

	    user = idPass->userId;
	    passwd = idPass->password;
	}
    }
    if (zebra_auth(zh, user, passwd) != ZEBRA_OK)
    {
	r->errcode = YAZ_BIB1_INIT_AC_BAD_USERID_AND_OR_PASSWORD;
	r->errstring = user;
	return r;
    }
    if (q->charneg_request) /* characater set and langauge negotiation? */
    {
        char **charsets = 0;
        int num_charsets;
        char **langs = 0;
        int num_langs = 0;
        int selected = 0;
        int i;
        NMEM nmem = nmem_create();

        yaz_log (YLOG_LOG, "character set and language negotiation");

        yaz_get_proposal_charneg (nmem, q->charneg_request,
                                  &charsets, &num_charsets,
                                  &langs, &num_langs, &selected);
        
        for (i = 0; i < num_charsets; i++)
        {
            const char *right_name = "";
    	    /*
	     * FIXME! It is like rudiment :-))
    	     * We have to support this short names of character sets,
    	     * because a lot servers in Russia to use own in during
    	     * character set and language negotiation still.
    	     */
            
            if (!yaz_matchstr(charsets[i], "win")) {
                right_name = "WINDOWS-1251";
            } else if (!yaz_matchstr(charsets[i], "koi")) {
                right_name = "KOI8-R";
            } else if (!yaz_matchstr(charsets[i], "iso")) {
                right_name = "ISO-8859-5";
            } else if (!yaz_matchstr(charsets[i], "dos")) {
                right_name = "CP866";
            } else if (!yaz_matchstr(charsets[i], "uni")) {
                right_name = "UTF-8";
            } else {
                right_name = charsets[i];
            }
            if (odr_set_charset (q->decode, "UTF-8", right_name) == 0)
            {
                yaz_log (YLOG_LOG, "charset %d %s (proper name %s): OK", i,
                         charsets[i], right_name);
                odr_set_charset (q->stream, right_name, "UTF-8");
                if (selected)
                    zebra_record_encoding(zh, right_name);
		zebra_octet_term_encoding(zh, right_name);
        	q->charneg_response =
        	    yaz_set_response_charneg (q->stream, charsets[i],
                                              0, selected);
        	break;
            } else {
                yaz_log (YLOG_LOG, "charset %d %s (proper name %s): unsupported", i,
                         charsets[i], right_name);
            }
        }
        nmem_destroy(nmem);
    }
    return r;
}

static void search_terms(ZebraHandle zh, bend_search_rr *r)
{
    int no_terms;
    int i;
    int type = Z_Term_general;
    struct Z_External *ext;
    Z_SearchInfoReport *sr;

    zebra_result_set_term_no(zh, r->setname, &no_terms);
    if (!no_terms)
        return;

    r->search_info = odr_malloc (r->stream, sizeof(*r->search_info));

    r->search_info->num_elements = 1;
    r->search_info->list =
        odr_malloc (r->stream, sizeof(*r->search_info->list));
    r->search_info->list[0] =
        odr_malloc (r->stream, sizeof(**r->search_info->list));
    r->search_info->list[0]->category = 0;
    r->search_info->list[0]->which = Z_OtherInfo_externallyDefinedInfo;
    ext = odr_malloc (r->stream, sizeof(*ext));
    r->search_info->list[0]->information.externallyDefinedInfo = ext;
    ext->direct_reference =
        yaz_oidval_to_z3950oid (r->stream, CLASS_USERINFO, VAL_SEARCHRES1);
    ext->indirect_reference = 0;
    ext->descriptor = 0;
    ext->which = Z_External_searchResult1;
    sr = odr_malloc (r->stream, sizeof(Z_SearchInfoReport));
    ext->u.searchResult1 = sr;
    sr->num = no_terms;
    sr->elements = odr_malloc (r->stream, sr->num *
                               sizeof(*sr->elements));
    for (i = 0; i<no_terms; i++)
    {
        Z_Term *term;
	zint count;
	int approx;
        char outbuf[1024];
        size_t len = sizeof(outbuf);

	zebra_result_set_term_info(zh, r->setname, i,
				   &count, &approx, outbuf, &len);

        sr->elements[i] = odr_malloc (r->stream, sizeof(**sr->elements));
        sr->elements[i]->subqueryId = 0;
        sr->elements[i]->fullQuery = odr_malloc (r->stream, 
                                                 sizeof(bool_t));
        *sr->elements[i]->fullQuery = 0;
        sr->elements[i]->subqueryExpression = 
            odr_malloc (r->stream, sizeof(Z_QueryExpression));
        sr->elements[i]->subqueryExpression->which = 
            Z_QueryExpression_term;
        sr->elements[i]->subqueryExpression->u.term =
            odr_malloc (r->stream, sizeof(Z_QueryExpressionTerm));
        term = odr_malloc (r->stream, sizeof(Z_Term));
        sr->elements[i]->subqueryExpression->u.term->queryTerm = term;
        switch (type)
        {
        case Z_Term_characterString:
            yaz_log (YLOG_DEBUG, "term as characterString");
            term->which = Z_Term_characterString;
            term->u.characterString = odr_strdup (r->stream, outbuf);
            break;
        case Z_Term_general:
            yaz_log (YLOG_DEBUG, "term as general");
            term->which = Z_Term_general;
            term->u.general = odr_malloc (r->stream, sizeof(*term->u.general));
            term->u.general->size = term->u.general->len = len;
            term->u.general->buf = odr_malloc (r->stream, len);
            memcpy (term->u.general->buf, outbuf, len);
            break;
        default:
            term->which = Z_Term_general;
            term->u.null = odr_nullval();
        }
        sr->elements[i]->subqueryExpression->u.term->termComment = 0;
        sr->elements[i]->subqueryInterpretation = 0;
        sr->elements[i]->subqueryRecommendation = 0;
	if (count > 2000000000)
	    count = 2000000000;
        sr->elements[i]->subqueryCount = odr_intdup (r->stream, (int) count);
        sr->elements[i]->subqueryWeight = 0;
        sr->elements[i]->resultsByDB = 0;
    }
}

int bend_search (void *handle, bend_search_rr *r)
{
    ZebraHandle zh = (ZebraHandle) handle;
    zint zhits = 0;
    ZEBRA_RES res;

    res = zebra_select_databases (zh, r->num_bases,
				  (const char **) r->basenames);
    if (res != ZEBRA_OK)
    {
        zebra_result (zh, &r->errcode, &r->errstring);
        return 0;
    }
    yaz_log (YLOG_DEBUG, "ResultSet '%s'", r->setname);
    switch (r->query->which)
    {
    case Z_Query_type_1: case Z_Query_type_101:
	res = zebra_search_RPN(zh, r->stream, r->query->u.type_1,
			       r->setname, &zhits);
	if (res != ZEBRA_OK)
	    zebra_result(zh, &r->errcode, &r->errstring);
	else
	{
	    if (zhits >   2147483646)
		r->hits = 2147483647;
	    else
	    r->hits = (int) zhits;
            search_terms (zh, r);
	}
        break;
    case Z_Query_type_2:
	r->errcode = YAZ_BIB1_QUERY_TYPE_UNSUPP;
	r->errstring = "type-2";
	break;
    default:
        r->errcode = YAZ_BIB1_QUERY_TYPE_UNSUPP;
    }
    return 0;
}


int bend_fetch (void *handle, bend_fetch_rr *r)
{
    ZebraHandle zh = (ZebraHandle) handle;
    ZebraRetrievalRecord retrievalRecord;
    ZEBRA_RES res;

    retrievalRecord.position = r->number;
    
    r->last_in_set = 0;
    res = zebra_records_retrieve (zh, r->stream, r->setname, r->comp,
				  r->request_format, 1, &retrievalRecord);
    if (res != ZEBRA_OK)
    {
	/* non-surrogate diagnostic */
	zebra_result (zh, &r->errcode, &r->errstring);
    }
    else if (retrievalRecord.errCode)
    {
	/* surrogate diagnostic (diagnostic per record) */
	r->surrogate_flag = 1;
	r->errcode = retrievalRecord.errCode;
	r->errstring = retrievalRecord.errString;
	r->basename = retrievalRecord.base;
    }
    else
    {
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
    ZEBRA_RES res;

    res = zebra_select_databases(zh, r->num_bases, 
				 (const char **) r->basenames);
    if (res != ZEBRA_OK)
    {
        zebra_result (zh, &r->errcode, &r->errstring);
        return 0;
    }
    if (r->step_size != 0 && *r->step_size != 0) {
	r->errcode = YAZ_BIB1_ONLY_ZERO_STEP_SIZE_SUPPORTED_FOR_SCAN;
	r->errstring = 0;
        return 0;
    }
    r->entries = (struct scan_entry *)
	odr_malloc (r->stream, sizeof(*r->entries) * r->num_entries);
    res = zebra_scan(zh, r->stream, r->term,
		     r->attributeset,
		     &r->term_position,
		     &r->num_entries, &entries, &is_partial);
    if (res == ZEBRA_OK)
    {
	if (is_partial)
	    r->status = BEND_SCAN_PARTIAL;
	else
	    r->status = BEND_SCAN_SUCCESS;
	for (i = 0; i < r->num_entries; i++)
	{
	    r->entries[i].term = entries[i].term;
	    r->entries[i].occurrences = entries[i].occurrences;
	}
    }
    else
    {
	r->status = BEND_SCAN_PARTIAL;
	zebra_result(zh, &r->errcode, &r->errstring);
    }
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
    zebra_result (zh, &rr->errcode, &rr->errstring);
    return 0;
}

int bend_delete (void *handle, bend_delete_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;

    rr->delete_status =	zebra_deleteResultSet(zh, rr->function,
					      rr->num_setnames, rr->setnames,
					      rr->statuses);
    return 0;
}

static int es_admin_request (ZebraHandle zh, Z_AdminEsRequest *r)
{
    if (r->toKeep->databaseName)
    {
	yaz_log(YLOG_LOG, "adm request database %s", r->toKeep->databaseName);
    }
    switch (r->toKeep->which)
    {
    case Z_ESAdminOriginPartToKeep_reIndex:
	yaz_log(YLOG_LOG, "adm-reindex");
	break;
    case Z_ESAdminOriginPartToKeep_truncate:
	yaz_log(YLOG_LOG, "adm-truncate");
	break;
    case Z_ESAdminOriginPartToKeep_drop:
	yaz_log(YLOG_LOG, "adm-drop");
	zebra_drop_database (zh, r->toKeep->databaseName);
	break;
    case Z_ESAdminOriginPartToKeep_create:
	yaz_log(YLOG_LOG, "adm-create %s", r->toKeep->databaseName);
	zebra_create_database (zh, r->toKeep->databaseName);
	break;
    case Z_ESAdminOriginPartToKeep_import:
	yaz_log(YLOG_LOG, "adm-import");
	zebra_admin_import_begin (zh, r->toKeep->databaseName,
			r->toKeep->u.import->recordType);
	break;
    case Z_ESAdminOriginPartToKeep_refresh:
	yaz_log(YLOG_LOG, "adm-refresh");
	break;
    case Z_ESAdminOriginPartToKeep_commit:
	yaz_log(YLOG_LOG, "adm-commit");
	if (r->toKeep->databaseName)
	    zebra_select_database(zh, r->toKeep->databaseName);
	zebra_commit(zh);
	break;
    case Z_ESAdminOriginPartToKeep_shutdown:
	yaz_log(YLOG_LOG, "shutdown");
	zebra_admin_shutdown(zh);
	break;
    case Z_ESAdminOriginPartToKeep_start:
	yaz_log(YLOG_LOG, "start");
	zebra_admin_start(zh);
	break;
    default:
	yaz_log(YLOG_LOG, "unknown admin");
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
	yaz_log (YLOG_LOG, "adm taskpackage (unhandled)");
	break;
    default:
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
    
    yaz_log(YLOG_LOG, "function: %d", *rr->esr->function);
    if (rr->esr->packageName)
    	yaz_log(YLOG_LOG, "packagename: %s", rr->esr->packageName);
    yaz_log(YLOG_LOG, "Waitaction: %d", *rr->esr->waitAction);

    if (!rr->esr->taskSpecificParameters)
    {
        yaz_log (YLOG_WARN, "No task specific parameters");
    }
    else if (rr->esr->taskSpecificParameters->which == Z_External_ESAdmin)
    {
	es_admin (zh, rr->esr->taskSpecificParameters->u.adminService);

        zebra_result (zh, &rr->errcode, &rr->errstring);
    }
    else if (rr->esr->taskSpecificParameters->which == Z_External_update)
    {
    	Z_IUUpdate *up = rr->esr->taskSpecificParameters->u.update;
	yaz_log (YLOG_LOG, "Received DB Update");
	if (up->which == Z_IUUpdate_esRequest)
	{
	    Z_IUUpdateEsRequest *esRequest = up->u.esRequest;
	    Z_IUOriginPartToKeep *toKeep = esRequest->toKeep;
	    Z_IUSuppliedRecords *notToKeep = esRequest->notToKeep;
	    
	    yaz_log (YLOG_LOG, "action");
	    if (toKeep->action)
	    {
		switch (*toKeep->action)
		{
		case Z_IUOriginPartToKeep_recordInsert:
		    yaz_log (YLOG_LOG, "recordInsert");
		    break;
		case Z_IUOriginPartToKeep_recordReplace:
		    yaz_log (YLOG_LOG, "recordUpdate");
		    break;
		case Z_IUOriginPartToKeep_recordDelete:
		    yaz_log (YLOG_LOG, "recordDelete");
		    break;
		case Z_IUOriginPartToKeep_elementUpdate:
		    yaz_log (YLOG_LOG, "elementUpdate");
		    break;
		case Z_IUOriginPartToKeep_specialUpdate:
		    yaz_log (YLOG_LOG, "specialUpdate");
		    break;
                case Z_ESAdminOriginPartToKeep_shutdown:
		    yaz_log (YLOG_LOG, "shutDown");
		    break;
		case Z_ESAdminOriginPartToKeep_start:
		    yaz_log (YLOG_LOG, "start");
		    break;
		default:
		    yaz_log (YLOG_LOG, " unknown (%d)", *toKeep->action);
		}
	    }
	    if (toKeep->databaseName)
	    {
		yaz_log (YLOG_LOG, "database: %s", toKeep->databaseName);

                if (zebra_select_database(zh, toKeep->databaseName))
                    return 0;
	    }
            else
            {
                yaz_log (YLOG_WARN, "no database supplied for ES Update");
                rr->errcode =
		    YAZ_BIB1_ES_MISSING_MANDATORY_PARAMETER_FOR_SPECIFIED_FUNCTION_;
                rr->errstring = "database";
                return 0;
            }
	    if (zebra_begin_trans(zh, 1) != ZEBRA_OK)
	    {
		zebra_result(zh, &rr->errcode, &rr->errstring);
	    }
	    else
	    {
		int i;
		for (i = 0; notToKeep && i < notToKeep->num; i++)
		{
		    Z_External *rec = notToKeep->elements[i]->record;
                    struct oident *oident = 0;
                    Odr_oct *opaque_recid = 0;
		    SYSNO sysno = 0;

		    if (notToKeep->elements[i]->u.opaque)
		    {
			switch(notToKeep->elements[i]->which)
			{
			case Z_IUSuppliedRecords_elem_opaque:
			    opaque_recid = notToKeep->elements[i]->u.opaque;
			    break; /* OK, recid already set */
			case Z_IUSuppliedRecords_elem_number:
			    sysno = *notToKeep->elements[i]->u.number;
			    break;
			}
                    }
		    if (rec->direct_reference)
		    {
			oident = oid_getentbyoid(rec->direct_reference);
			if (oident)
			    yaz_log (YLOG_LOG, "record %d type %s", i,
				     oident->desc);
		    }
		    switch (rec->which)
		    {
		    case Z_External_sutrs:
			if (rec->u.octet_aligned->len > 170)
			    yaz_log (YLOG_LOG, "%d bytes:\n%.168s ...",
				     rec->u.sutrs->len,
				     rec->u.sutrs->buf);
			else
			    yaz_log (YLOG_LOG, "%d bytes:\n%s",
				     rec->u.sutrs->len,
				     rec->u.sutrs->buf);
                        break;
		    case Z_External_octet:
			if (rec->u.octet_aligned->len > 170)
			    yaz_log (YLOG_LOG, "%d bytes:\n%.168s ...",
				     rec->u.octet_aligned->len,
				     rec->u.octet_aligned->buf);
			else
			    yaz_log (YLOG_LOG, "%d bytes\n%s",
				     rec->u.octet_aligned->len,
				     rec->u.octet_aligned->buf);
		    }
                    if (oident && oident->value != VAL_TEXT_XML)
                    {
                        rr->errcode = YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
                        rr->errstring = "only XML update supported";
                        break;
                    }
                    if (rec->which == Z_External_octet)
                    {
                        int action = 0;

                        if (*toKeep->action ==
                            Z_IUOriginPartToKeep_recordInsert)
                            action = 1;
                        if (*toKeep->action ==
                            Z_IUOriginPartToKeep_recordReplace)
                            action = 2;
                        if (*toKeep->action ==
                            Z_IUOriginPartToKeep_recordDelete)
                            action = 3;
                        if (*toKeep->action ==
                            Z_IUOriginPartToKeep_specialUpdate)
                            action = 4;

                        if (!action)
                        {
                            rr->errcode =
				YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
                            rr->errstring = "unsupported ES Update action";
                            break;
                        }
                        else if (opaque_recid)
			{
                            int r = zebra_admin_exchange_record (
                                zh,
                                rec->u.octet_aligned->buf,
                                rec->u.octet_aligned->len,
                                opaque_recid->buf, opaque_recid->len,
                                action);
                            if (r)
                            {
                                rr->errcode =
				    YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
                                rr->errstring = "record exchange failed";
                                break;
                            }
			}
			else
			{
			    ZEBRA_RES r = ZEBRA_FAIL;
			    switch(action) {
			    case 1:
				r = zebra_insert_record(
				    zh,
				    0, /* recordType */
				    &sysno,
				    0, /* match */
				    0, /* fname */
				    rec->u.octet_aligned->buf,
				    rec->u.octet_aligned->len,
				    0);
				if (r == ZEBRA_FAIL)
				{
				    rr->errcode =
					YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
				    rr->errstring = "insert_record failed";
				}
				break;
			    case 2:
			    case 4:
				r = zebra_update_record(
				    zh,
				    0, /* recordType */
				    &sysno,
				    0, /* match */
				    0, /* fname */
				    rec->u.octet_aligned->buf,
				    rec->u.octet_aligned->len,
				    1);
				if (r == ZEBRA_FAIL)
				{
				    rr->errcode =
					YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
				    rr->errstring = "update_record failed";
				}
				break;
			    case 3:
				r = zebra_delete_record(
				    zh,
				    0, /* recordType */
				    &sysno,
				    0, /* match */
				    0, /* fname */
				    rec->u.octet_aligned->buf,
				    rec->u.octet_aligned->len,
				    0);
				if (r == ZEBRA_FAIL)
				{
				    rr->errcode =
					YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
				    rr->errstring = "delete_record failed";
				}
				break;
			    }				
			}
                    }
		}
                zebra_end_trans (zh);
	    }
	}
    }
    else
    {
        yaz_log (YLOG_WARN, "Unknown Extended Service(%d)",
		 rr->esr->taskSpecificParameters->which);
        rr->errcode = YAZ_BIB1_ES_EXTENDED_SERVICE_TYPE_UNSUPP;
	
    }
    return 0;
}

static void bend_start (struct statserv_options_block *sob)
{
    if (sob->handle)
	zebra_stop((ZebraService) sob->handle);
    sob->handle = zebra_start(sob->configname);
    if (!sob->handle)
    {
	yaz_log (YLOG_FATAL, "Failed to read config `%s'", sob->configname);
	exit (1);
    }
#ifdef WIN32
    
#else
    if (!sob->inetd) 
    {
	char pidfname[4096];
        struct flock area;
	int fd;

	zebra_pidfname(sob->handle, pidfname);

        fd = open (pidfname, O_EXCL|O_WRONLY|O_CREAT, 0666);
        if (fd == -1)
        {
            if (errno != EEXIST)
            {
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "lock file %s", pidfname);
                exit(1);
            }
            fd = open(pidfname, O_RDWR, 0666);
            if (fd == -1)
            {
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "lock file %s", pidfname);
                exit(1);
            }
        }
        area.l_type = F_WRLCK;
        area.l_whence = SEEK_SET;
        area.l_len = area.l_start = 0L;
        if (fcntl (fd, F_SETLK, &area) == -1)
        {
            yaz_log(YLOG_ERRNO|YLOG_FATAL, "Zebra server already running");
            exit(1);
        }
        else
        {
	    char pidstr[30];
	
	    sprintf (pidstr, "%ld", (long) getpid ());
	    write (fd, pidstr, strlen(pidstr));
        }
    }
#endif
}

static void bend_stop(struct statserv_options_block *sob)
{
#ifdef WIN32

#else
    if (!sob->inetd && sob->handle) 
    {
	char pidfname[4096];
	zebra_pidfname(sob->handle, pidfname);
        unlink (pidfname);
    }
#endif
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
    strcpy (sob->configname, "zebra.cfg");
    sob->bend_start = bend_start;
    sob->bend_stop = bend_stop;
#ifdef WIN32
    strcpy (sob->service_display_name, "Zebra Server");
#endif
    statserv_setcontrol (sob);

    return statserv_main (argc, argv, bend_init, bend_close);
}
