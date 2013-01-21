/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
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
#include <yaz/yaz-version.h>
#include <yaz/diagbib1.h>
#include <yaz/querytowrbuf.h>
#include <yaz/pquery.h>
#include <sys/types.h>

#include <yaz/backend.h>
#include <yaz/charneg.h>
#include <idzebra/api.h>

static int bend_sort(void *handle, bend_sort_rr *rr);
static int bend_delete(void *handle, bend_delete_rr *rr);
static int bend_esrequest(void *handle, bend_esrequest_rr *rr);
static int bend_segment(void *handle, bend_segment_rr *rr);
static int bend_search(void *handle, bend_search_rr *r);
static int bend_fetch(void *handle, bend_fetch_rr *r);
static int bend_scan(void *handle, bend_scan_rr *r);

bend_initresult *bend_init(bend_initrequest *q)
{
    bend_initresult *r = (bend_initresult *)
	odr_malloc(q->stream, sizeof(*r));
    ZebraHandle zh;
    struct statserv_options_block *sob;
    char *user = NULL;
    char *passwd = NULL;
    char version_str[16];

    r->errcode = 0;
    r->errstring = 0;
    q->bend_sort = bend_sort;
    q->bend_delete = bend_delete;
    q->bend_esrequest = bend_esrequest;
    q->bend_segment = bend_segment;
    q->bend_search = bend_search;
    q->bend_fetch = bend_fetch;
    q->bend_scan = bend_scan;

    zebra_get_version(version_str, 0);

    q->implementation_name = "Zebra Information Server";
    q->implementation_version = odr_strdup(q->stream, version_str);

    yaz_log(YLOG_DEBUG, "bend_init");

    sob = statserv_getcontrol();
    if (!(zh = zebra_open(sob->handle, 0)))
    {
	yaz_log(YLOG_WARN, "Failed to read config `%s'", sob->configname);
	r->errcode = YAZ_BIB1_PERMANENT_SYSTEM_ERROR;
	return r;
    }
    r->handle = zh;

    q->query_charset = odr_strdup(q->stream, zebra_get_encoding(zh));
    if (q->auth)
    {
	if (q->auth->which == Z_IdAuthentication_open)
	{
	    char *openpass = xstrdup(q->auth->u.open);
	    char *cp = strchr(openpass, '/');
	    if (cp)
	    {
		*cp = '\0';
		user = nmem_strdup(odr_getmem(q->stream), openpass);
		passwd = nmem_strdup(odr_getmem(q->stream), cp+1);
	    }
	    xfree(openpass);
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
    if (q->charneg_request) /* characater set and language negotiation? */
    {
        char **charsets = 0;
        int num_charsets;
        char **langs = 0;
        int num_langs = 0;
        int selected = 0;
        int i;
        NMEM nmem = nmem_create();

        yaz_get_proposal_charneg(nmem, q->charneg_request,
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
            if (odr_set_charset(q->decode, "UTF-8", right_name) == 0)
            {
                yaz_log(YLOG_LOG, "charset %d %s (proper name %s): OK", i,
                        charsets[i], right_name);
                odr_set_charset(q->stream, right_name, "UTF-8");
                if (selected)
                    zebra_record_encoding(zh, right_name);
		zebra_octet_term_encoding(zh, right_name);
        	q->charneg_response =
        	    yaz_set_response_charneg(q->stream, charsets[i],
                                             0, selected);
        	break;
            } else {
                yaz_log(YLOG_LOG, "charset %d %s (proper name %s): unsupported", i,
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

    r->search_info = odr_malloc(r->stream, sizeof(*r->search_info));

    r->search_info->num_elements = 1;
    r->search_info->list =
        odr_malloc(r->stream, sizeof(*r->search_info->list));
    r->search_info->list[0] =
        odr_malloc(r->stream, sizeof(**r->search_info->list));
    r->search_info->list[0]->category = 0;
    r->search_info->list[0]->which = Z_OtherInfo_externallyDefinedInfo;
    ext = odr_malloc(r->stream, sizeof(*ext));
    r->search_info->list[0]->information.externallyDefinedInfo = ext;
    ext->direct_reference = odr_oiddup(r->stream,
                                       yaz_oid_userinfo_searchresult_1);
    ext->indirect_reference = 0;
    ext->descriptor = 0;
    ext->which = Z_External_searchResult1;
    sr = odr_malloc(r->stream, sizeof(Z_SearchInfoReport));
    ext->u.searchResult1 = sr;
    sr->num = no_terms;
    sr->elements = odr_malloc(r->stream, sr->num *
                               sizeof(*sr->elements));
    for (i = 0; i < no_terms; i++)
    {
	Z_SearchInfoReport_s *se;
        Z_Term *term;
	zint count;
	int approx;
        char outbuf[1024];
        size_t len = sizeof(outbuf);
	const char *term_ref_id = 0;

	zebra_result_set_term_info(zh, r->setname, i,
				   &count, &approx, outbuf, &len,
				   &term_ref_id);
        se = sr->elements[i] = odr_malloc(r->stream, sizeof(**sr->elements));
        se->subqueryId = term_ref_id ?
	    odr_strdup(r->stream, term_ref_id) : 0;

        se->fullQuery = odr_booldup(r->stream, 0);
        se->subqueryExpression =
            odr_malloc(r->stream, sizeof(Z_QueryExpression));
        se->subqueryExpression->which =
            Z_QueryExpression_term;
        se->subqueryExpression->u.term =
            odr_malloc(r->stream, sizeof(Z_QueryExpressionTerm));
        term = odr_malloc(r->stream, sizeof(Z_Term));
        se->subqueryExpression->u.term->queryTerm = term;
        switch (type)
        {
        case Z_Term_characterString:
            term->which = Z_Term_characterString;
            term->u.characterString = odr_strdup(r->stream, outbuf);
            break;
        case Z_Term_general:
            term->which = Z_Term_general;
            term->u.general = odr_malloc(r->stream, sizeof(*term->u.general));
            term->u.general->size = term->u.general->len = len;
            term->u.general->buf = odr_malloc(r->stream, len);
            memcpy(term->u.general->buf, outbuf, len);
            break;
        default:
            term->which = Z_Term_general;
            term->u.null = odr_nullval();
        }
        se->subqueryExpression->u.term->termComment = 0;
        se->subqueryInterpretation = 0;
        se->subqueryRecommendation = 0;
        se->subqueryCount = odr_intdup(r->stream, count);
        se->subqueryWeight = 0;
        se->resultsByDB = 0;
    }
}

static int break_handler(void *client_data)
{
    bend_association assoc =(bend_association) client_data;
    if (!bend_assoc_is_alive(assoc))
        return 1;
    return 0;
}

static Z_RPNQuery *query_add_sortkeys(ODR o, Z_RPNQuery *query,
                                      const char *sortKeys)
{
#if YAZ_VERSIONL >= 0x40200
    /* sortkey layour: path,schema,ascending,caseSensitive,missingValue */
    /* see cql_sortby_to_sortkeys of YAZ. */
    char **sortspec;
    int num_sortspec = 0;
    int i;

    if (sortKeys)
        nmem_strsplit_blank(odr_getmem(o), sortKeys, &sortspec, &num_sortspec);
    if (num_sortspec > 0)
    {
        Z_RPNQuery *nquery;
        WRBUF w = wrbuf_alloc();

        /* left operand 'd' will be replaced by original query */
        wrbuf_puts(w, "@or d");
        for (i = 0; i < num_sortspec; i++)
        {
            char **arg;
            int num_arg;
            int ascending = 1;
            nmem_strsplitx(odr_getmem(o), ",", sortspec[i], &arg, &num_arg, 0);

            if (num_arg > 5 || num_arg < 1)
            {
                yaz_log(YLOG_WARN, "Invalid sort spec '%s' num_arg=%d",
                        sortspec[i], num_arg);
                break;
            }
            if (num_arg > 2 && arg[2][0])
                ascending = atoi(arg[2]);

            if (i < num_sortspec-1)
                wrbuf_puts(w, " @or");
            wrbuf_puts(w, " @attr 1=");
            yaz_encode_pqf_term(w, arg[0], strlen(arg[0]));
            wrbuf_printf(w, "@attr 7=%d %d", ascending ? 1 : 2, i);
        }
        nquery = p_query_rpn(o, wrbuf_cstr(w));
        if (!nquery)
        {
            yaz_log(YLOG_WARN, "query_add_sortkeys: bad RPN: '%s'",
                    wrbuf_cstr(w));
        }
        else
        {
            Z_RPNStructure *s = nquery->RPNStructure;

            if (s->which != Z_RPNStructure_complex)
            {
                yaz_log(YLOG_WARN, "query_add_sortkeys: not complex operand");
            }
            else
            {
                /* left operand 'd' is replaced by origial query */
                s->u.complex->s1 = query->RPNStructure;
                /* and original query points to our new or+sort things */
                query->RPNStructure = s;
            }
        }
        wrbuf_destroy(w);
    }
#else
    if (sortKeys)
    {
        yaz_log(YLOG_WARN, "sortkeys ignored because YAZ version < 4.2.0");
    }
#endif
    return query;
}

int bend_search(void *handle, bend_search_rr *r)
{
    ZebraHandle zh = (ZebraHandle) handle;
    zint zhits = 0;
    ZEBRA_RES res;
    Z_RPNQuery *q2;

    res = zebra_select_databases(zh, r->num_bases,
				  (const char **) r->basenames);
    if (res != ZEBRA_OK)
    {
        zebra_result(zh, &r->errcode, &r->errstring);
        return 0;
    }
    zebra_set_break_handler(zh, break_handler, r->association);
    yaz_log(YLOG_DEBUG, "ResultSet '%s'", r->setname);
    switch (r->query->which)
    {
    case Z_Query_type_1: case Z_Query_type_101:
        q2 = query_add_sortkeys(r->stream, r->query->u.type_1, r->srw_sortKeys);
	res = zebra_search_RPN_x(zh, r->stream, q2,
                                 r->setname, &zhits,
                                 &r->estimated_hit_count,
                                 &r->partial_resultset);
	if (res != ZEBRA_OK)
	    zebra_result(zh, &r->errcode, &r->errstring);
	else
	{
            r->hits = zhits;
            search_terms(zh, r);
	}
        break;
    case Z_Query_type_2:
	r->errcode = YAZ_BIB1_QUERY_TYPE_UNSUPP;
	r->errstring = "type-2";
	break;
    default:
        r->errcode = YAZ_BIB1_QUERY_TYPE_UNSUPP;
    }
    zebra_set_break_handler(zh, 0, 0);
    return 0;
}


int bend_fetch(void *handle, bend_fetch_rr *r)
{
    ZebraHandle zh = (ZebraHandle) handle;
    ZebraRetrievalRecord retrievalRecord;
    ZEBRA_RES res;

    retrievalRecord.position = r->number;

    r->last_in_set = 0;
    res = zebra_records_retrieve(zh, r->stream, r->setname, r->comp,
				  r->request_format, 1, &retrievalRecord);
    if (res != ZEBRA_OK)
    {
	/* non-surrogate diagnostic */
	zebra_result(zh, &r->errcode, &r->errstring);
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
	r->output_format = odr_oiddup(r->stream, retrievalRecord.format);
    }
    return 0;
}

static int bend_scan(void *handle, bend_scan_rr *r)
{
    ZebraScanEntry *entries;
    ZebraHandle zh = (ZebraHandle) handle;
    int is_partial, i;
    ZEBRA_RES res;

    res = zebra_select_databases(zh, r->num_bases,
				 (const char **) r->basenames);
    if (res != ZEBRA_OK)
    {
        zebra_result(zh, &r->errcode, &r->errstring);
        return 0;
    }
    if (r->step_size != 0 && *r->step_size != 0) {
	r->errcode = YAZ_BIB1_ONLY_ZERO_STEP_SIZE_SUPPORTED_FOR_SCAN;
	r->errstring = 0;
        return 0;
    }
    res = zebra_scan(zh, r->stream, r->term,
		     r->attributeset,
		     &r->term_position,
		     &r->num_entries, &entries, &is_partial,
		     0 /* setname */);
    if (res == ZEBRA_OK)
    {
	if (is_partial)
	    r->status = BEND_SCAN_PARTIAL;
	else
	    r->status = BEND_SCAN_SUCCESS;
	for (i = 0; i < r->num_entries; i++)
	{
	    r->entries[i].term = entries[i].term;
	    r->entries[i].display_term = entries[i].display_term;
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

void bend_close(void *handle)
{
    zebra_close((ZebraHandle) handle);
    xmalloc_trav("bend_close");
}

int bend_sort(void *handle, bend_sort_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;

    if (zebra_sort(zh, rr->stream,
		    rr->num_input_setnames, (const char **) rr->input_setnames,
		    rr->output_setname, rr->sort_sequence, &rr->sort_status)
	!= ZEBRA_OK)
	zebra_result(zh, &rr->errcode, &rr->errstring);
    return 0;
}

int bend_delete(void *handle, bend_delete_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;

    rr->delete_status =	zebra_deleteResultSet(zh, rr->function,
					      rr->num_setnames, rr->setnames,
					      rr->statuses);
    return 0;
}

static void es_admin_request(bend_esrequest_rr *rr, ZebraHandle zh, Z_AdminEsRequest *r)
{
    ZEBRA_RES res = ZEBRA_OK;
    if (r->toKeep->databaseName)
    {
	yaz_log(YLOG_LOG, "adm request database %s", r->toKeep->databaseName);
    }
    switch (r->toKeep->which)
    {
    case Z_ESAdminOriginPartToKeep_reIndex:
	yaz_log(YLOG_LOG, "adm-reindex");
	rr->errcode = YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
	rr->errstring = "adm-reindex not implemented yet";
	break;
    case Z_ESAdminOriginPartToKeep_truncate:
	rr->errcode = YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
	rr->errstring = "adm-reindex not implemented yet";
	yaz_log(YLOG_LOG, "adm-truncate");
	break;
    case Z_ESAdminOriginPartToKeep_drop:
	yaz_log(YLOG_LOG, "adm-drop");
	res = zebra_drop_database(zh, r->toKeep->databaseName);
	break;
    case Z_ESAdminOriginPartToKeep_create:
	yaz_log(YLOG_LOG, "adm-create %s", r->toKeep->databaseName);
	res = zebra_create_database(zh, r->toKeep->databaseName);
	break;
    case Z_ESAdminOriginPartToKeep_import:
	yaz_log(YLOG_LOG, "adm-import");
	res = zebra_admin_import_begin(zh, r->toKeep->databaseName,
					r->toKeep->u.import->recordType);
	break;
    case Z_ESAdminOriginPartToKeep_refresh:
	yaz_log(YLOG_LOG, "adm-refresh");
	break;
    case Z_ESAdminOriginPartToKeep_commit:
	yaz_log(YLOG_LOG, "adm-commit");
	if (r->toKeep->databaseName)
	{
	    if (zebra_select_database(zh, r->toKeep->databaseName) !=
		ZEBRA_OK)
		yaz_log(YLOG_WARN, "zebra_select_database failed in "
			"adm-commit");
	}
	zebra_commit(zh);
	break;
    case Z_ESAdminOriginPartToKeep_shutdown:
	yaz_log(YLOG_LOG, "shutdown");
	res = zebra_admin_shutdown(zh);
	break;
    case Z_ESAdminOriginPartToKeep_start:
	yaz_log(YLOG_LOG, "start");
	zebra_admin_start(zh);
	break;
    default:
	yaz_log(YLOG_LOG, "unknown admin");
	rr->errcode = YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
	rr->errstring = "adm-reindex not implemented yet";
    }
    if (res != ZEBRA_OK)
	zebra_result(zh, &rr->errcode, &rr->errstring);
}

static void es_admin(bend_esrequest_rr *rr, ZebraHandle zh, Z_Admin *r)
{
    switch (r->which)
    {
    case Z_Admin_esRequest:
	es_admin_request(rr, zh, r->u.esRequest);
	return;
    default:
	break;
    }
	yaz_log(YLOG_WARN, "adm taskpackage (unhandled)");
    rr->errcode = YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
    rr->errstring = "adm-task package (unhandled)";
}

int bend_segment(void *handle, bend_segment_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;
    Z_Segment *segment = rr->segment;

    if (segment->num_segmentRecords)
	zebra_admin_import_segment(zh, rr->segment);
    else
	zebra_admin_import_end(zh);
    return 0;
}

int bend_esrequest(void *handle, bend_esrequest_rr *rr)
{
    ZebraHandle zh = (ZebraHandle) handle;

    yaz_log(YLOG_LOG, "function: " ODR_INT_PRINTF, *rr->esr->function);
    if (rr->esr->packageName)
    	yaz_log(YLOG_LOG, "packagename: %s", rr->esr->packageName);
    yaz_log(YLOG_LOG, "Waitaction: " ODR_INT_PRINTF, *rr->esr->waitAction);

    if (!rr->esr->taskSpecificParameters)
    {
        yaz_log(YLOG_WARN, "No task specific parameters");
    }
    else if (rr->esr->taskSpecificParameters->which == Z_External_ESAdmin)
    {
	es_admin(rr, zh, rr->esr->taskSpecificParameters->u.adminService);
    }
    else if (rr->esr->taskSpecificParameters->which == Z_External_update)
    {
    	Z_IUUpdate *up = rr->esr->taskSpecificParameters->u.update;
	yaz_log(YLOG_LOG, "Received DB Update");
	if (up->which == Z_IUUpdate_esRequest)
	{
	    Z_IUUpdateEsRequest *esRequest = up->u.esRequest;
	    Z_IUOriginPartToKeep *toKeep = esRequest->toKeep;
	    Z_IUSuppliedRecords *notToKeep = esRequest->notToKeep;

	    yaz_log(YLOG_LOG, "action");
	    if (toKeep->action)
	    {
		switch (*toKeep->action)
		{
		case Z_IUOriginPartToKeep_recordInsert:
		    yaz_log(YLOG_LOG, "recordInsert");
		    break;
		case Z_IUOriginPartToKeep_recordReplace:
		    yaz_log(YLOG_LOG, "recordUpdate");
		    break;
		case Z_IUOriginPartToKeep_recordDelete:
		    yaz_log(YLOG_LOG, "recordDelete");
		    break;
		case Z_IUOriginPartToKeep_elementUpdate:
		    yaz_log(YLOG_LOG, "elementUpdate");
		    break;
		case Z_IUOriginPartToKeep_specialUpdate:
		    yaz_log(YLOG_LOG, "specialUpdate");
		    break;
                case Z_ESAdminOriginPartToKeep_shutdown:
		    yaz_log(YLOG_LOG, "shutDown");
		    break;
		case Z_ESAdminOriginPartToKeep_start:
		    yaz_log(YLOG_LOG, "start");
		    break;
		default:
		    yaz_log(YLOG_LOG, " unknown (" ODR_INT_PRINTF ")",
                            *toKeep->action);
		}
	    }
	    if (toKeep->databaseName)
	    {
		yaz_log(YLOG_LOG, "database: %s", toKeep->databaseName);

                if (zebra_select_database(zh, toKeep->databaseName))
                    return 0;
	    }
            else
            {
                yaz_log(YLOG_WARN, "no database supplied for ES Update");
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
                    Odr_oct *opaque_recid = 0;
		    zint *sysno = 0;
		    zint sysno_tmp;

		    if (notToKeep->elements[i]->u.opaque)
		    {
			switch(notToKeep->elements[i]->which)
			{
			case Z_IUSuppliedRecords_elem_opaque:
			    opaque_recid = notToKeep->elements[i]->u.opaque;
			    break; /* OK, recid already set */
			case Z_IUSuppliedRecords_elem_number:
			    sysno_tmp = *notToKeep->elements[i]->u.number;
			    sysno = &sysno_tmp;
			    break;
			}
                    }
		    if (rec->direct_reference)
		    {
                        char oid_name_str[OID_STR_MAX];
                        const char *oid_name =
                            yaz_oid_to_string_buf(
                                rec->direct_reference,
                                0, oid_name_str);
                        if (oid_name)
			    yaz_log(YLOG_LOG, "record %d type %s", i,
                                     oid_name);
		    }
		    switch (rec->which)
		    {
		    case Z_External_sutrs:
			if (rec->u.octet_aligned->len > 170)
			    yaz_log(YLOG_LOG, "%d bytes:\n%.168s ...",
				     rec->u.sutrs->len,
				     rec->u.sutrs->buf);
			else
			    yaz_log(YLOG_LOG, "%d bytes:\n%s",
				     rec->u.sutrs->len,
				     rec->u.sutrs->buf);
                        break;
		    case Z_External_octet:
			if (rec->u.octet_aligned->len > 170)
			    yaz_log(YLOG_LOG, "%d bytes:\n%.168s ...",
				     rec->u.octet_aligned->len,
				     rec->u.octet_aligned->buf);
			else
			    yaz_log(YLOG_LOG, "%d bytes\n%s",
				     rec->u.octet_aligned->len,
				     rec->u.octet_aligned->buf);
		    }
                    if (rec->which == Z_External_octet)
                    {
                        enum zebra_recctrl_action_t action = action_update;
                        char recid_str[256];
                        const char *match_criteria = 0;
                        ZEBRA_RES res;

                        if (*toKeep->action ==
                            Z_IUOriginPartToKeep_recordInsert)
                            action = action_insert;
                        else if (*toKeep->action ==
                            Z_IUOriginPartToKeep_recordReplace)
                            action = action_replace;
                        else if (*toKeep->action ==
                            Z_IUOriginPartToKeep_recordDelete)
                            action = action_delete;
                        else if (*toKeep->action ==
                            Z_IUOriginPartToKeep_specialUpdate)
                            action = action_update;
                        else
                        {
                            rr->errcode =
				YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
                            rr->errstring = "unsupported ES Update action";
                            break;
                        }

                        if (opaque_recid)
			{
                            size_t l = opaque_recid->len;
                            if (l >= sizeof(recid_str))
                            {
                                rr->errcode = YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
                                rr->errstring = "opaque record ID too large";
                                break;
                            }
                            memcpy(recid_str, opaque_recid->buf, l);
                            recid_str[l] = '\0';
                            match_criteria = recid_str;
                        }
                        res = zebra_update_record(
                            zh, action,
                            0, /* recordType */
                            sysno, match_criteria, 0, /* fname */
                            (const char *) rec->u.octet_aligned->buf,
                            rec->u.octet_aligned->len);
                        if (res == ZEBRA_FAIL)
                        {
                            rr->errcode =
                                YAZ_BIB1_ES_IMMEDIATE_EXECUTION_FAILED;
                            rr->errstring = "update_record failed";
                        }
                    }
		}
                if (zebra_end_trans(zh) != ZEBRA_OK)
		{
		    yaz_log(YLOG_WARN, "zebra_end_trans failed for"
			    " extended service operation");
		}
	    }
	}
    }
    else
    {
        yaz_log(YLOG_WARN, "Unknown Extended Service(%d)",
		 rr->esr->taskSpecificParameters->which);
        rr->errcode = YAZ_BIB1_ES_EXTENDED_SERVICE_TYPE_UNSUPP;

    }
    return 0;
}

static void bend_start(struct statserv_options_block *sob)
{
    Res default_res = res_open(0, 0);

    if (sob->handle)
	zebra_stop((ZebraService) sob->handle);
    res_set(default_res, "profilePath", DEFAULT_PROFILE_PATH);
    res_set(default_res, "modulePath", DEFAULT_MODULE_PATH);
    sob->handle = zebra_start_res(sob->configname, default_res, 0);
    res_close(default_res);
    if (!sob->handle)
    {
	yaz_log(YLOG_FATAL, "Failed to read config `%s'", sob->configname);
	exit(1);
    }
#ifdef WIN32

#else
    if (!sob->inetd && !sob->background)
    {
	char pidfname[4096];
        struct flock area;
	int fd;

	zebra_pidfname(sob->handle, pidfname);

        fd = open(pidfname, O_EXCL|O_WRONLY|O_CREAT, 0666);
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
        if (fcntl(fd, F_SETLK, &area) == -1)
        {
            yaz_log(YLOG_ERRNO|YLOG_FATAL, "Zebra server already running");
            exit(1);
        }
        else
        {
	    char pidstr[30];

	    sprintf(pidstr, "%ld", (long) getpid());
	    if (write(fd, pidstr, strlen(pidstr)) != strlen(pidstr))
            {
                yaz_log(YLOG_ERRNO|YLOG_FATAL, "write fail %s", pidfname);
                exit(1);
            }
        }
    }
#endif
}

static void bend_stop(struct statserv_options_block *sob)
{
#ifdef WIN32

#else
    if (!sob->inetd && !sob->background && sob->handle)
    {
	char pidfname[4096];
	zebra_pidfname(sob->handle, pidfname);
        unlink(pidfname);
    }
#endif
    if (sob->handle)
    {
	ZebraService service = sob->handle;
	zebra_stop(service);
    }
}

int main(int argc, char **argv)
{
    struct statserv_options_block *sob;

    sob = statserv_getcontrol();
    strcpy(sob->configname, "zebra.cfg");
    sob->bend_start = bend_start;
    sob->bend_stop = bend_stop;
#ifdef WIN32
    strcpy(sob->service_name, "zebrasrv");
    strcpy(sob->service_display_name, "Zebra Server");
#endif
    statserv_setcontrol(sob);

    return statserv_main(argc, argv, bend_init, bend_close);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

