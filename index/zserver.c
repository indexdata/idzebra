/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.c,v $
 * Revision 1.9  1995-10-02 15:18:52  adam
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

#include <recctrl.h>
#include <backend.h>
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

    server_info.sets = NULL;
    if (!(server_info.sys_idx_fd = open (FNAME_SYS_IDX, O_RDONLY)))
    {
        r.errcode = 1;
        r.errstring = "dict_open fail: filedict";
        return &r;
    }
    if (!(server_info.fileDict = dict_open (FNAME_FILE_DICT, 10, 0)))
    {
        r.errcode = 1;
        r.errstring = "dict_open fail: filedict";
        return &r;
    }    
    if (!(server_info.wordDict = dict_open (FNAME_WORD_DICT, 40, 0)))
    {
        dict_close (server_info.fileDict);
        r.errcode = 1;
        r.errstring = "dict_open fail: worddict";
        return &r;
    }    
    if (!(server_info.wordIsam = is_open (FNAME_WORD_ISAM, key_compare, 0,
                                          sizeof (struct it_key))))
    {
        dict_close (server_info.wordDict);
        dict_close (server_info.fileDict);
        r.errcode = 1;
        r.errstring = "is_open fail: wordisam";
        return &r;
    }
    server_info.recordBuf = NULL;
    return &r;
}

bend_searchresult *bend_search (void *handle, bend_searchrequest *q, int *fd)
{
    static bend_searchresult r;

    r.errcode = 0;
    r.errstring = 0;
    r.hits = 0;

    switch (q->query->which)
    {
    case Z_Query_type_1:
        r.errcode = rpn_search (&server_info, q->query->u.type_1,
                                q->num_bases, q->basenames, q->setname,
                                &r.hits);
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

static int record_fetch (ZServerInfo *zi, int sysno, ODR stream,
                          oid_value input_format, oid_value *output_format,
                          char **rec_bufp, int *rec_lenp)
{
    char record_info[SYS_IDX_ENTRY_LEN];
    char *fname, *file_type;
    RecType rt;
    struct recRetrieveCtrl retrieveCtrl;

    if (lseek (zi->sys_idx_fd, sysno * SYS_IDX_ENTRY_LEN,
               SEEK_SET) == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Retrieve: lseek of sys_idx");
        exit (1);
    }
    if (read (zi->sys_idx_fd, record_info, SYS_IDX_ENTRY_LEN) == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Retrieve: read of sys_idx");
        exit (1);
    }
    file_type = record_info;
    fname = record_info + strlen(record_info) + 1;
    if (!(rt = recType_byName (file_type)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "Retrieve: Cannot handle type %s", 
              file_type);
        exit (1);
    }
    if ((retrieveCtrl.fd = open (fname, O_RDONLY)) == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Retrieve: Open record file %s", fname);
        exit (1);
    }
    retrieveCtrl.odr = stream;
    retrieveCtrl.readf = record_read;
    retrieveCtrl.input_format = input_format;
    retrieveCtrl.diagnostic = 0;
    (*rt->retrieve)(&retrieveCtrl);
    *output_format = retrieveCtrl.output_format;
    *rec_bufp = retrieveCtrl.rec_buf;
    *rec_lenp = retrieveCtrl.rec_len;
    close (retrieveCtrl.fd);
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

    xfree (server_info.recordBuf);
    server_info.recordBuf = NULL;
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
    r.errcode = record_fetch (&server_info, records[0].sysno, q->stream,
                              q->format, &r.format, &r.record, &r.len);
    return &r;
}

bend_deleteresult *bend_delete (void *handle, bend_deleterequest *q, int *num)
{
    return 0;
}

bend_scanresult *bend_scan (void *handle, bend_scanrequest *q, int *num)
{
    static struct scan_entry list[200];
    static char buf[200][200];
    static bend_scanresult r;
    int i;

    r.term_position = q->term_position;
    r.num_entries = q->num_entries;
    r.entries = list;
    for (i = 0; i < r.num_entries; i++)
    {
    	list[i].term = buf[i];
	sprintf(list[i].term, "term-%d", i+1);
	list[i].occurrences = rand() % 100000;
    }
    r.errcode = 0;
    r.errstring = 0;
    return &r;
}

void bend_close (void *handle)
{
    dict_close (server_info.fileDict);
    dict_close (server_info.wordDict);
    is_close (server_info.wordIsam);
    close (server_info.sys_idx_fd);
    xfree (server_info.recordBuf);
    server_info.recordBuf = NULL;
    return;
}

int main (int argc, char **argv)
{
    char *base_name = "base";

    if (!(common_resource = res_open (base_name)))
    {
        logf (LOG_FATAL, "Cannot open resource `%s'", base_name);
        exit (1);
    }
    return statserv_main (argc, argv);
}
