/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dirs.c,v $
 * Revision 1.2  1995-11-20 11:56:23  adam
 * Work on new traversal.
 *
 * Revision 1.1  1995/11/17  15:54:42  adam
 * Started work on virtual directory structure.
 */
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#include <alexutil.h>
#include "index.h"

struct dirs_info {
    Dict dict;
    int no_read;
    int no_cur;
    int no_max;
    struct dirs_entry *entries;
    char nextpath[256];
    char prefix[256];
    int prelen;
    struct dirs_entry *last_entry;
};

static int dirs_client_proc (Dict_char *name, const char *info, int pos,
                             void *client)
{
    struct dirs_info *ci = client;
    struct dirs_entry *entry;

    if (memcmp (name, ci->prefix, ci->prelen))
        return 1;
    if (ci->no_cur < 0)
    {
        ci->no_cur = 0;
        return 0;
    }
    if (ci->no_cur == ci->no_max)
    {
        assert (0);
    }
    entry = ci->entries + ci->no_cur;
    if (info[0] == sizeof(entry->sysno)+sizeof(entry->ctime))
    {
        strcpy (entry->path, name + ci->prelen); 
        entry->kind = dirs_file;
        memcpy (&entry->sysno, info+1, sizeof(entry->sysno));
        memcpy (&entry->ctime, info+1+sizeof(entry->sysno), 
                sizeof(entry->ctime));
        ci->no_cur++;
    } 
    else if (info[0] == sizeof(entry->ctime))
    {
        strcpy (entry->path, name + ci->prelen);
        entry->kind = dirs_dir;
        memcpy (&entry->ctime, info+1, sizeof(entry->ctime));
        ci->no_cur++;
    }
    return 0;
}

struct dirs_info *dirs_open (Dict dict, const char *rep)
{
    struct dirs_info *p;
    int before = 0, after;

    logf (LOG_DEBUG, "dirs_open %s", rep);
    if (!(p = malloc (sizeof (*p))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    p->dict = dict;
    strcpy (p->prefix, rep);
    p->prelen = strlen(p->prefix);
    strcpy (p->nextpath, rep);
    p->no_read = p->no_cur = 0;
    after = p->no_max = 400;
    if (!(p->entries = malloc (sizeof(*p->entries) * (p->no_max))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    logf (LOG_DEBUG, "dirs_open first scan");
    dict_scan (p->dict, p->nextpath, &before, &after, p, dirs_client_proc);
    return p;
}

struct dirs_entry *dirs_read (struct dirs_info *p)
{
    int before = 0, after = p->no_max;

    if (p->no_read < p->no_cur)
    {
        logf (LOG_DEBUG, "dirs_read %d. returns %s", p->no_read,
              (p->entries + p->no_read)->path);
        return p->last_entry = p->entries + (p->no_read++);
    }
    if (p->no_cur < p->no_max)
        return p->last_entry = NULL;
#if 0
    strcpy (p->nextpath, p->prefix);
    strcat (p->nextpath, (p->entries + p->no_max-1)->path);
#endif
    p->no_cur = -1;
    logf (LOG_DEBUG, "dirs_read rescan");
    dict_scan (p->dict, p->nextpath, &before, &after, p, dirs_client_proc);
    p->no_read = 1;
    if (p->no_read < p->no_cur)
        return p->last_entry = p->entries;
    return p->last_entry = NULL;
}

struct dirs_entry *dirs_last (struct dirs_info *p)
{
    return p->last_entry;
}

void dirs_mkdir (struct dirs_info *p, const char *src, int ctime)
{
    char path[256];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_mkdir %s", path);
    dict_insert (p->dict, path, sizeof(ctime), &ctime);
}

void dirs_rmdir (struct dirs_info *p, const char *src)
{
    char path[256];
    char info[2];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_rmdir %s", path);
    info[0] = 'r';
    dict_insert (p->dict, path, 1, info);
}

void dirs_add (struct dirs_info *p, const char *src, int sysno, int ctime)
{
    char path[256];
    char info[16];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_add %s", path);
    memcpy (info, &sysno, sizeof(sysno));
    memcpy (info+sizeof(sysno), &ctime, sizeof(ctime));
    dict_insert (p->dict, path, sizeof(sysno)+sizeof(ctime), info);
}

void dirs_del (struct dirs_info *p, const char *src)
{
    char path[256];
    char info[2];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_del %s", path);
    info[0] = 'r';
    dict_insert (p->dict, path, 1, info);
}

void dirs_free (struct dirs_info **pp)
{
    struct dirs_info *p = *pp;

    free (p->entries);
    free (p);
    *pp = NULL;
}

