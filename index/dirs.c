/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dirs.c,v $
 * Revision 1.14  1998-01-12 15:04:07  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.13  1997/09/09 13:38:06  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.12  1996/11/08 11:10:13  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.11  1996/10/29 14:06:47  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.10  1996/06/04 10:18:58  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.9  1996/04/23  12:39:07  adam
 * Bug fix: In function dirs_del dict_delete is used to remove a file
 * rather than a bogus dict_insert.
 *
 * Revision 1.8  1996/04/12  07:02:21  adam
 * File update of single files.
 *
 * Revision 1.7  1996/03/21 14:50:09  adam
 * File update uses modify-time instead of change-time.
 *
 * Revision 1.6  1996/02/02  13:44:43  adam
 * The public dictionary functions simply use char instead of Dict_char
 * to represent search strings. Dict_char is used internally only.
 *
 * Revision 1.5  1996/01/17  14:54:44  adam
 * Function dirs_rmdir uses dict_delete.
 *
 * Revision 1.4  1995/11/30  08:34:27  adam
 * Started work on commit facility.
 * Changed a few malloc/free to xmalloc/xfree.
 *
 * Revision 1.3  1995/11/20  16:59:45  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.2  1995/11/20  11:56:23  adam
 * Work on new traversal.
 *
 * Revision 1.1  1995/11/17  15:54:42  adam
 * Started work on virtual directory structure.
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include "index.h"

#define DIRS_MAX_PATH 1024

struct dirs_info {
    Dict dict;
    int rw;
    int no_read;
    int no_cur;
    int no_max;
    struct dirs_entry *entries;
    char nextpath[DIRS_MAX_PATH];
    char prefix[DIRS_MAX_PATH];
    int prelen;
    struct dirs_entry *last_entry;
};

static int dirs_client_proc (char *name, const char *info, int pos,
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
    if (info[0] == sizeof(entry->sysno)+sizeof(entry->mtime))
    {
        strcpy (entry->path, name + ci->prelen); 
        entry->kind = dirs_file;
        memcpy (&entry->sysno, info+1, sizeof(entry->sysno));
        memcpy (&entry->mtime, info+1+sizeof(entry->sysno), 
                sizeof(entry->mtime));
        ci->no_cur++;
    } 
    else if (info[0] == sizeof(entry->mtime))
    {
        strcpy (entry->path, name + ci->prelen);
        entry->kind = dirs_dir;
        memcpy (&entry->mtime, info+1, sizeof(entry->mtime));
        ci->no_cur++;
    }
    return 0;
}

struct dirs_info *dirs_open (Dict dict, const char *rep, int rw)
{
    struct dirs_info *p;
    int before = 0, after;

    logf (LOG_DEBUG, "dirs_open %s", rep);
    p = xmalloc (sizeof (*p));
    p->dict = dict;
    p->rw = rw;
    strcpy (p->prefix, rep);
    p->prelen = strlen(p->prefix);
    strcpy (p->nextpath, rep);
    p->no_read = p->no_cur = 0;
    after = p->no_max = 100;
    p->entries = xmalloc (sizeof(*p->entries) * (p->no_max));
    logf (LOG_DEBUG, "dirs_open first scan");
    dict_scan (p->dict, p->nextpath, &before, &after, p, dirs_client_proc);
    return p;
}

struct dirs_info *dirs_fopen (Dict dict, const char *path)
{
    struct dirs_info *p;
    struct dirs_entry *entry;
    char *info;

    p = xmalloc (sizeof(*p));
    p->dict = dict;
    *p->prefix = '\0';
    p->entries = xmalloc (sizeof(*p->entries));
    p->no_read = 0;
    p->no_cur = 0;
    p->no_max = 2;

    entry = p->entries;
    info = dict_lookup (dict, path);
    if (info && info[0] == sizeof(entry->sysno)+sizeof(entry->mtime))
    {
        strcpy (entry->path, path); 
        entry->kind = dirs_file;
        memcpy (&entry->sysno, info+1, sizeof(entry->sysno));
        memcpy (&entry->mtime, info+1+sizeof(entry->sysno), 
                sizeof(entry->mtime));
        p->no_cur++;
    }
    return p;
}

struct dirs_entry *dirs_read (struct dirs_info *p)
{
    int before = 0, after = p->no_max+1;

    if (p->no_read < p->no_cur)
    {
        logf (LOG_DEBUG, "dirs_read %d. returns %s", p->no_read,
              (p->entries + p->no_read)->path);
        return p->last_entry = p->entries + (p->no_read++);
    }
    if (p->no_cur < p->no_max)
        return p->last_entry = NULL;
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

void dirs_mkdir (struct dirs_info *p, const char *src, time_t mtime)
{
    char path[DIRS_MAX_PATH];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_mkdir %s", path);
    if (p->rw)
	dict_insert (p->dict, path, sizeof(mtime), &mtime);
}

void dirs_rmdir (struct dirs_info *p, const char *src)
{
    char path[DIRS_MAX_PATH];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_rmdir %s", path);
    if (p->rw)
	dict_delete (p->dict, path);
}

void dirs_add (struct dirs_info *p, const char *src, int sysno, time_t mtime)
{
    char path[DIRS_MAX_PATH];
    char info[16];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_add %s", path);
    memcpy (info, &sysno, sizeof(sysno));
    memcpy (info+sizeof(sysno), &mtime, sizeof(mtime));
    if (p->rw)
	dict_insert (p->dict, path, sizeof(sysno)+sizeof(mtime), info);
}

void dirs_del (struct dirs_info *p, const char *src)
{
    char path[DIRS_MAX_PATH];

    sprintf (path, "%s%s", p->prefix, src);
    logf (LOG_DEBUG, "dirs_del %s", path);
    if (p->rw)
	dict_delete (p->dict, path);
}

void dirs_free (struct dirs_info **pp)
{
    struct dirs_info *p = *pp;

    xfree (p->entries);
    xfree (p);
    *pp = NULL;
}

