/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dirs.c,v $
 * Revision 1.1  1995-11-17 15:54:42  adam
 * Started work on virtual directory structure.
 *
 * Revision 1.9  1995/10/30  13:42:12  adam
 * Added errno.h
 *
 * Revision 1.8  1995/10/10  13:59:23  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.7  1995/09/28  09:19:40  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.6  1995/09/08  14:52:26  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.5  1995/09/06  16:11:16  adam
 * Option: only one word key per file.
 *
 * Revision 1.4  1995/09/04  12:33:41  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.3  1995/09/01  14:06:35  adam
 * Split of work into more files.
 *
 * Revision 1.2  1995/09/01  10:57:07  adam
 * Minor changes.
 *
 * Revision 1.1  1995/09/01  10:34:51  adam
 * Added dir.c
 *
 */
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#include <alexutil.h>
#include "index.h"

struct dirs_entry {
    char path[160];
    int sysno;
};

struct dirs_info {
    int no;
    struct dirs_entry *entries;
};

struct dirs_client_info {
    struct dirs_info *di;
    int no_max;
    char *prefix;
    int prelen;
};

static int dirs_client_proc (Dict_char *name, const char *info, int pos,
                             void *client)
{
    struct dirs_client_info *ci = client;

    if (memcmp (name, ci->prefix, ci->prelen))
        return 1;
    if (ci->di->no == ci->no_max)
    {
        if (ci->no_max > 0)
        {
            struct dirs_entry *arn;

            ci->no_max += 1000;
            arn = malloc (sizeof(*arn) * (ci->no_max));
            if (!arn)
            {
                logf (LOG_FATAL|LOG_ERRNO, "malloc");
                exit (1);
            }
            memcpy (arn, ci->di->entries, ci->di->no * sizeof(*arn));
            free (ci->di->entries);
            ci->di->entries = arn;
        }
    }
    strcpy ((ci->di->entries + ci->di->no)->path, name); 
    memcpy (&(ci->di->entries + ci->di->no)->sysno, info+1, *info);
    assert (*info == sizeof(ci->di->entries->sysno));
    ++(ci->di->no);
    return 0;
}

struct dirs_info *dirs_open (Dict dict, const char *rep)
{
    static char wname[200];
    static char pname[200];
    int dirid;
    char *dinfo;
    struct dirs_client_info dirs_client_info;
    struct dirs_info *p;
    int before, after;

    sprintf (wname, "d%s", rep);
    dinfo = dict_lookup (dict, wname);
    if (!dinfo)
        return NULL;
    if (!(p = malloc (sizeof (*p))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    memcpy (&dirid, dinfo+1, sizeof(dirid));
    sprintf (wname, "%d.", dirid);
    strcpy (pname, wname);

    p->no = 0;
    dirs_client_info.di = p;
    dirs_client_info.no_max = 0;
    dirs_client_info.prefix = wname;
    dirs_client_info.prelen = strlen(wname);
    strcpy (pname, wname);
    dict_scan (dict, pname, &before, &after, &dirs_client_info,
               dirs_client_proc);

    return p;
}

void dirs_free (struct dirs_info **pp)
{
    struct dirs_info *p = *pp;

    free (p->entries);
    free (p);
    *pp = NULL;
}
