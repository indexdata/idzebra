/*
 * Copyright (C) 1995-1998, Index Data 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.c,v $
 * Revision 1.63  1998-09-02 13:53:21  adam
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
#ifdef WINDOWS
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include <data1.h>
#include <dmalloc.h>

#include "zserver.h"

static int bend_sort (void *handle, bend_sort_rr *rr);

bend_initresult *bend_init (bend_initrequest *q)
{
    bend_initresult *r = odr_malloc (q->stream, sizeof(*r));
    ZebraHandle zh;
    struct statserv_options_block *sob;
    char *user = NULL;
    char *passwd = NULL;

    r->errcode = 0;
    r->errstring = 0;
    q->bend_sort = bend_sort;

    logf (LOG_DEBUG, "bend_init");

    sob = statserv_getcontrol ();
    if (!(zh = zebra_open (sob->configname)))
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
    if (zebra_auth (zh, user, passwd))
    {
	r->errcode = 222;
	r->errstring = user;
	zebra_close (zh);
	return r;
    }
    r->handle = zh;
    return r;
}

bend_searchresult *bend_search (void *handle, bend_searchrequest *q, int *fd)
{
    ZebraHandle zh = handle;
    bend_searchresult *r = odr_malloc (q->stream, sizeof(*r));

    r->hits = 0;
    r->errcode = 0;
    r->errstring = NULL;
    
    logf (LOG_LOG, "ResultSet '%s'", q->setname);
    switch (q->query->which)
    {
    case Z_Query_type_1: case Z_Query_type_101:
	zebra_search_rpn (zh, q->stream, q->decode, q->query->u.type_1,
			  q->num_bases, q->basenames, q->setname);
	r->errcode = zh->errCode;
	r->errstring = zh->errString;
	r->hits = zh->hits;
        break;
    default:
        r->errcode = 107;
    }
    return r;
}

bend_fetchresult *bend_fetch (void *handle, bend_fetchrequest *q, int *num)
{
    ZebraHandle zh = handle;
    bend_fetchresult *r = odr_malloc (q->stream, sizeof(*r));
    ZebraRetrievalRecord retrievalRecord;

    retrievalRecord.position = q->number;
    
    r->last_in_set = 0;
    zebra_records_retrieve (zh, q->stream, q->setname, q->comp,
			    q->format, 1, &retrievalRecord);
    if (zh->errCode)                  /* non Surrogate Diagnostic */
    {
	r->errcode = zh->errCode;
	r->errstring = zh->errString;
    }
    else if (retrievalRecord.errCode) /* Surrogate Diagnostic */
    {
	q->surrogate_flag = 1;
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
	r->format = retrievalRecord.format;
    }
    return r;
}

bend_scanresult *bend_scan (void *handle, bend_scanrequest *q, int *num)
{
    ZebraScanEntry *entries;
    ZebraHandle zh = handle;
    bend_scanresult *r = odr_malloc (q->stream, sizeof(*r));
    int is_partial, i;
    
    r->term_position = q->term_position;
    r->num_entries = q->num_entries;

    r->entries = odr_malloc (q->stream, sizeof(*r->entries) * q->num_entries);
    zebra_scan (zh, q->stream, q->term,
		q->attributeset,
		q->num_bases, q->basenames,
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
    return r;
}

void bend_close (void *handle)
{
    zebra_close ((ZebraHandle) handle);
}

int bend_sort (void *handle, bend_sort_rr *rr)
{
    ZebraHandle zh = handle;

    zebra_sort (zh, rr->stream,
                rr->num_input_setnames, (const char **) rr->input_setnames,
		rr->output_setname, rr->sort_sequence, &rr->sort_status);
    rr->errcode = zh->errCode;
    rr->errstring = zh->errString;
    return 0;
}

#ifndef WINDOWS
static void pre_init (struct statserv_options_block *sob)
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

int main (int argc, char **argv)
{
    struct statserv_options_block *sob;

    sob = statserv_getcontrol ();
    strcpy (sob->configname, FNAME_CONFIG);
#ifndef WINDOWS
    sob->pre_init = pre_init;
#endif
    statserv_setcontrol (sob);

    return statserv_main (argc, argv);
}
