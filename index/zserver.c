/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.c,v $
 * Revision 1.25  1995-11-21 15:29:13  adam
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
#include <unistd.h>
#include <fcntl.h>

#include <common.h>
#include <data1.h>
#include <recctrl.h>
#include <dmalloc.h>
#include "zserver.h"

ZServerInfo server_info;

bend_initresult *bend_init (bend_initrequest *q)
{
    static bend_initresult r;
    static char *name = "zserver";

    r.errcode = 0;
    r.errstring = 0;
    r.handle = name;

    logf (LOG_DEBUG, "bend_init");

    if (!common_resource)
    {
        struct statserv_options_block *sob;

        sob = statserv_getcontrol ();
        logf (LOG_LOG, "Reading resources from %s", sob->configname);
        if (!(common_resource = res_open (sob->configname)))
        {
            logf (LOG_FATAL, "Cannot open resource `%s'", sob->configname);
            exit (1);
        }
    }

    data1_tabpath = res_get(common_resource, "data1_tabpath");
    server_info.sets = NULL;

    server_info.records = rec_open (0);
    if (!(server_info.wordDict = dict_open (FNAME_WORD_DICT, 40, 0)))
    {
        logf (LOG_WARN, "dict_open fail: word dict");
        r.errcode = 1;
        r.errstring = "dict_open fail: word dict";
        return &r;
    }    
    if (!(server_info.wordIsam = is_open (FNAME_WORD_ISAM, key_compare, 0,
                                          sizeof (struct it_key))))
    {
        logf (LOG_WARN, "is_open fail: word isam");
        dict_close (server_info.wordDict);
        r.errcode = 1;
        r.errstring = "is_open fail: word isam";
        return &r;
    }
    server_info.odr = odr_createmem (ODR_ENCODE);
    return &r;
}

bend_searchresult *bend_search (void *handle, bend_searchrequest *q, int *fd)
{
    static bend_searchresult r;

    r.errcode = 0;
    r.errstring = 0;
    r.hits = 0;


    odr_reset (server_info.odr);
    server_info.errCode = 0;
    server_info.errString = NULL;

    logf (LOG_LOG, "ResultSet '%s'", q->setname);
    switch (q->query->which)
    {
    case Z_Query_type_1:
        r.errcode = rpn_search (&server_info, q->query->u.type_1,
                                q->num_bases, q->basenames, q->setname,
                                &r.hits);
        r.errstring = server_info.errString;
        break;
    default:
        r.errcode = 107;
    }
    return &r;
}

static int record_read (int fd, char *buf, size_t count)
{
    return read (fd, buf, count);
}

static int record_fetch (ZServerInfo *zi, int sysno, int score, ODR stream,
                          oid_value input_format, Z_RecordComposition *comp,
			  oid_value *output_format, char **rec_bufp,
			  int *rec_lenp)
{
    Record rec;
    char *fname, *file_type;
    RecType rt;
    struct recRetrieveCtrl retrieveCtrl;

    rec = rec_get (zi->records, sysno);
    file_type = rec->info[0];
    fname = rec->info[1];

    if (!(rt = recType_byName (file_type)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "Retrieve: Cannot handle type %s", 
              file_type);
        exit (1);
    }
    logf (LOG_DEBUG, "retrieve localno=%d score=%d", sysno, score);
    if ((retrieveCtrl.fd = open (fname, O_RDONLY)) == -1)
    {
        char *msg = "Record doesn't exist";
        logf (LOG_WARN|LOG_ERRNO, "Retrieve: Open record file %s", fname);
        *output_format = VAL_SUTRS;
        *rec_bufp = msg;
        *rec_lenp = strlen (msg);
        rec_rm (&rec);
        return 0;     /* or 14: System error in presenting records */
    }
    retrieveCtrl.localno = sysno;
    retrieveCtrl.score = score;
    retrieveCtrl.odr = stream;
    retrieveCtrl.readf = record_read;
    retrieveCtrl.input_format = retrieveCtrl.output_format = input_format;
    retrieveCtrl.comp = comp;
    retrieveCtrl.diagnostic = 0;
    (*rt->retrieve)(&retrieveCtrl);
    *output_format = retrieveCtrl.output_format;
    *rec_bufp = retrieveCtrl.rec_buf;
    *rec_lenp = retrieveCtrl.rec_len;
    close (retrieveCtrl.fd);
    rec_rm (&rec);

    return retrieveCtrl.diagnostic;
}

bend_fetchresult *bend_fetch (void *handle, bend_fetchrequest *q, int *num)
{
    static bend_fetchresult r;
    int positions[2];
    ZServerSetSysno *records;

    r.errstring = 0;
    r.last_in_set = 0;
    r.basename = "base";

    odr_reset (server_info.odr);
    server_info.errCode = 0;

    positions[0] = q->number;
    records = resultSetSysnoGet (&server_info, q->setname, 1, positions);
    if (!records)
    {
        logf (LOG_DEBUG, "resultSetRecordGet, error");
        r.errcode = 13;
        return &r;
    }
    if (!records[0].sysno)
    {
        r.errcode = 13;
        logf (LOG_DEBUG, "Out of range. pos=%d", q->number);
        return &r;
    }
    r.errcode = record_fetch (&server_info, records[0].sysno,
                              records[0].score, q->stream, q->format,
                              q->comp, &r.format, &r.record, &r.len);
    resultSetSysnoDel (&server_info, records, 1);
    return &r;
}

bend_deleteresult *bend_delete (void *handle, bend_deleterequest *q, int *num)
{
    return 0;
}

bend_scanresult *bend_scan (void *handle, bend_scanrequest *q, int *num)
{
    static bend_scanresult r;
    int status;

    odr_reset (server_info.odr);
    server_info.errCode = 0;
    server_info.errString = 0;

    r.term_position = q->term_position;
    r.num_entries = q->num_entries;
    r.errcode = rpn_scan (&server_info, q->term,
                          q->num_bases, q->basenames,
                          &r.term_position,
                          &r.num_entries, &r.entries, &status);
    r.errstring = server_info.errString;
    r.status = status;
    return &r;
}

void bend_close (void *handle)
{
    dict_close (server_info.wordDict);
    is_close (server_info.wordIsam);
    rec_close (&server_info.records);
    return;
}

int main (int argc, char **argv)
{
    struct statserv_options_block *sob;

    sob = statserv_getcontrol ();
    strcpy (sob->configname, "base");
    statserv_setcontrol (sob);

    return statserv_main (argc, argv);
}
