/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zsets.c,v $
 * Revision 1.2  1995-09-06 10:33:04  adam
 * More work on present. Some log messages removed.
 *
 * Revision 1.1  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "zserver.h"
#include <rstemp.h>

ZServerSet *resultSetAdd (ZServerInfo *zi, const char *name, int ov, RSET rset)
{
    ZServerSet *s;

    for (s = zi->sets; s; s = s->next)
        if (!strcmp (s->name, name))
        {
            if (!ov)
                return NULL;
            rset_delete (s->rset);
            s->rset = rset;
            return s;
        }
    s = xmalloc (sizeof(*s));
    s->next = zi->sets;
    zi->sets = s;
    s->name = xmalloc (strlen(name)+1);
    strcpy (s->name, name);
    s->rset = rset;
    return s;
}

ZServerSet *resultSetGet (ZServerInfo *zi, const char *name)
{
    ZServerSet *s;

    for (s = zi->sets; s; s = s->next)
        if (!strcmp (s->name, name))
            return s;
    return NULL;
}

ZServerRecord *resultSetRecordGet (ZServerInfo *zi, const char *name, 
                                   int num, int *positions)
{
    ZServerSet *sset;
    ZServerRecord *sr;
    RSET rset;
    int num_i = 0;
    int position = 0;
    int psysno = 0;
    struct it_key key;

    if (!(sset = resultSetGet (zi, name)))
        return NULL;
    if (!(rset = sset->rset))
        return NULL;
    logf (LOG_DEBUG, "resultSetRecordGet");
    sr = xmalloc (sizeof(*sr) * num);
    rset_open (rset, 0);
    while (rset_read (rset, &key))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
            position++;
            if (position == positions[num_i])
            {
                FILE *inf;
                char fname[SYS_IDX_ENTRY_LEN];

                sr[num_i].buf = NULL;
                if (lseek (zi->sys_idx_fd, psysno * SYS_IDX_ENTRY_LEN,
                           SEEK_SET) == -1)
                {
                    logf (LOG_FATAL|LOG_ERRNO, "lseek of sys_idx");
                    exit (1);
                }
                if (read (zi->sys_idx_fd, fname, SYS_IDX_ENTRY_LEN) == -1)
                {
                    logf (LOG_FATAL|LOG_ERRNO, "read of sys_idx");
                    exit (1);
                }
                if (!(inf = fopen (fname, "r")))
                    logf (LOG_WARN, "fopen: %s", fname);
                else
                {
                    long size;

                    fseek (inf, 0L, SEEK_END);
                    size = ftell (inf);
                    fseek (inf, 0L, SEEK_SET);
                    logf (LOG_DEBUG, "get sysno=%d, fname=%s, size=%ld",
                          psysno, fname, (long) size);
                    sr[num_i].buf = xmalloc (size+1);
                    sr[num_i].size = size;
                    sr[num_i].buf[size] = '\0';
                    if (fread (sr[num_i].buf, size, 1, inf) != 1)
                    {
                        logf (LOG_FATAL|LOG_ERRNO, "fread %s", fname);
                        exit (1);
                    }
                    fclose (inf);
                }
                num_i++;
                if (num_i == num)
                    break;
            }
        }
    }
    rset_close (rset);
    while (num_i < num)
    {
        sr[num_i].buf = NULL;
        sr[num_i].size = 0;
        num_i++;
    }
    return sr;
}

void resultSetRecordDel (ZServerInfo *zi, ZServerRecord *records, int num)
{
    int i;

    for (i = 0; i<num; i++)
        free (records[i].buf);
    free (records);
}
