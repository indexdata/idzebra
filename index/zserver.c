/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.c,v $
 * Revision 1.5  1995-09-06 16:11:18  adam
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

#include "zserver.h"

#include <backend.h>
#include <dmalloc.h>

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

bend_fetchresult *bend_fetch (void *handle, bend_fetchrequest *q, int *num)
{
    static bend_fetchresult r;
    int positions[2];
    ZServerRecord *records;

    r.errstring = 0;
    r.last_in_set = 0;
    r.basename = "base";

    positions[0] = q->number;
    records = resultSetRecordGet (&server_info, q->setname, 1, positions);
    if (!records)
    {
        logf (LOG_DEBUG, "resultSetRecordGet, error");
        r.errcode = 13;
        return &r;
    }
    if (!records[0].buf)
    {
        r.errcode = 13;
        logf (LOG_DEBUG, "Out of range. pos=%d", q->number);
        return &r;
    }
    r.len = records[0].size;
    r.record = malloc (r.len+1);
    strcpy (r.record, records[0].buf);
    resultSetRecordDel (&server_info, records, 1);
    r.format = VAL_SUTRS;
    r.errcode = 0;
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
