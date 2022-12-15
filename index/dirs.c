/* This file is part of the Zebra server.
   Copyright (C) Index Data

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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include <yaz/snprintf.h>
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
    int nextpath_deleted;
};

static int dirs_client_proc(char *name, const char *info, int pos,
                             void *client)
{
    struct dirs_info *ci = (struct dirs_info *) client;
    struct dirs_entry *entry;

    if (memcmp(name, ci->prefix, ci->prelen))
        return 1;
    if (ci->no_cur < 0)
    {
        ci->no_cur = 0;
        return 0;
    }
    assert(ci->no_cur < ci->no_max);
    entry = ci->entries + ci->no_cur;
    if (info[0] == sizeof(entry->sysno)+sizeof(entry->mtime))
    {
        strcpy(entry->path, name + ci->prelen);
        entry->kind = dirs_file;
        memcpy(&entry->sysno, info+1, sizeof(entry->sysno));
        memcpy(&entry->mtime, info+1+sizeof(entry->sysno),
                sizeof(entry->mtime));
        ci->no_cur++;
    }
    else if (info[0] == sizeof(entry->mtime))
    {
        strcpy(entry->path, name + ci->prelen);
        entry->kind = dirs_dir;
        memcpy(&entry->mtime, info+1, sizeof(entry->mtime));
        ci->no_cur++;
    }
    return 0;
}

struct dirs_info *dirs_open(Dict dict, const char *rep, int rw)
{
    struct dirs_info *p;
    int before = 0, after;

    yaz_log(YLOG_DEBUG, "dirs_open %s", rep);
    p = (struct dirs_info *) xmalloc(sizeof(*p));
    p->dict = dict;
    p->rw = rw;
    strcpy(p->prefix, rep);
    p->prelen = strlen(p->prefix);
    strcpy(p->nextpath, rep);
    p->nextpath_deleted = 0;
    p->no_read = p->no_cur = 0;
    after = p->no_max = 100;
    p->entries = (struct dirs_entry *)
        xmalloc(sizeof(*p->entries) * (p->no_max));
    yaz_log(YLOG_DEBUG, "dirs_open first scan");
    dict_scan(p->dict, p->nextpath, &before, &after, p, dirs_client_proc);
    return p;
}

struct dirs_info *dirs_fopen(Dict dict, const char *path, int rw)
{
    struct dirs_info *p;
    struct dirs_entry *entry;
    char *info;

    p = (struct dirs_info *) xmalloc(sizeof(*p));
    p->dict = dict;
    p->rw = rw;
    *p->prefix = '\0';
    p->entries = (struct dirs_entry *) xmalloc(sizeof(*p->entries));
    p->no_read = 0;
    p->no_cur = 0;
    p->no_max = 2;

    entry = p->entries;
    info = dict_lookup(dict, path);
    if (info && info[0] == sizeof(entry->sysno)+sizeof(entry->mtime))
    {
        strcpy(entry->path, path);
        entry->kind = dirs_file;
        memcpy(&entry->sysno, info+1, sizeof(entry->sysno));
        memcpy(&entry->mtime, info+1+sizeof(entry->sysno),
                sizeof(entry->mtime));
        p->no_cur++;
    }
    return p;
}

struct dirs_entry *dirs_read(struct dirs_info *p)
{
    int before = 0, after = p->no_max+1;

    if (p->no_read < p->no_cur)
    {
        yaz_log(YLOG_DEBUG, "dirs_read %d. returns %s", p->no_read,
              (p->entries + p->no_read)->path);
        return p->last_entry = p->entries + (p->no_read++);
    }
    if (p->no_cur < p->no_max)
        return p->last_entry = NULL;
    if (p->nextpath_deleted)
    {
        p->no_cur = 0;
        after = p->no_max;
    }
    else
    {
        p->no_cur = -1;
        after = p->no_max + 1;
    }
    p->no_read = 1;
    p->nextpath_deleted = 0;
    yaz_log(YLOG_DEBUG, "dirs_read rescan %s", p->nextpath);
    dict_scan(p->dict, p->nextpath, &before, &after, p, dirs_client_proc);
    if (p->no_read <= p->no_cur)
        return p->last_entry = p->entries;
    return p->last_entry = NULL;
}

struct dirs_entry *dirs_last(struct dirs_info *p)
{
    return p->last_entry;
}

void dirs_mkdir(struct dirs_info *p, const char *src, time_t mtime)
{
    char path[DIRS_MAX_PATH];

    yaz_snprintf(path, sizeof(path), "%s%s", p->prefix, src);
    yaz_log(YLOG_DEBUG, "dirs_mkdir %s", path);
    if (p->rw)
        dict_insert(p->dict, path, sizeof(mtime), &mtime);
}

void dirs_rmdir(struct dirs_info *p, const char *src)
{
    char path[DIRS_MAX_PATH];

    yaz_snprintf(path, sizeof(path), "%s%s", p->prefix, src);
    yaz_log(YLOG_DEBUG, "dirs_rmdir %s", path);
    if (p->rw)
        dict_delete(p->dict, path);
}

void dirs_add(struct dirs_info *p, const char *src, zint sysno, time_t mtime)
{
    char path[DIRS_MAX_PATH];
    char info[16];

    yaz_snprintf(path, sizeof(path), "%s%s", p->prefix, src);
    yaz_log(YLOG_DEBUG, "dirs_add %s", path);
    memcpy(info, &sysno, sizeof(sysno));
    memcpy(info+sizeof(sysno), &mtime, sizeof(mtime));
    if (p->rw)
        dict_insert(p->dict, path, sizeof(sysno)+sizeof(mtime), info);
}

void dirs_del(struct dirs_info *p, const char *src)
{
    char path[DIRS_MAX_PATH];

    yaz_snprintf(path, sizeof(path), "%s%s", p->prefix, src);
    yaz_log(YLOG_DEBUG, "dirs_del %s", path);
    if (p->rw)
    {
        if (!strcmp(path, p->nextpath))
             p->nextpath_deleted = 1;
        dict_delete(p->dict, path);
    }
}

void dirs_free(struct dirs_info **pp)
{
    struct dirs_info *p = *pp;

    xfree(p->entries);
    xfree(p);
    *pp = NULL;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

