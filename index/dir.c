/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <direntz.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#include "index.h"


int zebra_file_stat(const char *file_name, struct stat *buf,
                     int follow_links)
{
#ifndef WIN32
    if (!follow_links)
        return lstat(file_name, buf);
#endif
    return stat(file_name, buf);
}

struct dir_entry *dir_open(const char *rep, const char *base,
                            int follow_links)
{
    DIR *dir;
    char path[1024];
    char full_rep[1024];
    size_t pathpos;
    struct dirent dent_s, *dent = &dent_s;
    size_t entry_max = 500;
    size_t idx = 0;
    struct dir_entry *entry;

    if (base && !yaz_is_abspath(rep))
    {
        strcpy(full_rep, base);
        strcat(full_rep, "/");
    }
    else
        *full_rep = '\0';
    strcat(full_rep, rep);

    yaz_log(YLOG_DEBUG, "dir_open %s", full_rep);
    if (!(dir = opendir(full_rep)))
    {
        yaz_log(YLOG_WARN|YLOG_ERRNO, "opendir %s", rep);
        return NULL;
    }
    entry = (struct dir_entry *) xmalloc(sizeof(*entry) * entry_max);
    strcpy(path, rep);
    pathpos = strlen(path);
    if (!pathpos || path[pathpos-1] != '/')
        path[pathpos++] = '/';
    while ( (dent = readdir(dir)) )
    {
        struct stat finfo;
        if (strcmp(dent->d_name, ".") == 0 ||
            strcmp(dent->d_name, "..") == 0)
            continue;
        if (idx == entry_max-1)
        {
            struct dir_entry *entry_n;

            entry_n = (struct dir_entry *)
		xmalloc(sizeof(*entry) * (entry_max += 1000));
            memcpy(entry_n, entry, idx * sizeof(*entry));
            xfree(entry);
            entry = entry_n;
        }
        strcpy(path + pathpos, dent->d_name);

        if (base && !yaz_is_abspath(path))
        {
            strcpy(full_rep, base);
            strcat(full_rep, "/");
            strcat(full_rep, path);
            zebra_file_stat(full_rep, &finfo, follow_links);
        }
        else
            zebra_file_stat(path, &finfo, follow_links);
        switch (finfo.st_mode & S_IFMT)
        {
        case S_IFREG:
            entry[idx].kind = dirs_file;
            entry[idx].mtime = finfo.st_mtime;
            entry[idx].name = (char *) xmalloc(strlen(dent->d_name)+1);
            strcpy(entry[idx].name, dent->d_name);
            idx++;
            break;
        case S_IFDIR:
            entry[idx].kind = dirs_dir;
            entry[idx].mtime = finfo.st_mtime;
            entry[idx].name = (char *) xmalloc(strlen(dent->d_name)+2);
            strcpy(entry[idx].name, dent->d_name);
	    strcat(entry[idx].name, "/");
            idx++;
            break;
        }
    }
    entry[idx].name = NULL;
    closedir(dir);
    yaz_log(YLOG_DEBUG, "dir_close");
    return entry;
}

static int dir_cmp(const void *p1, const void *p2)
{
    return strcmp(((struct dir_entry *) p1)->name,
                  ((struct dir_entry *) p2)->name);
}

void dir_sort(struct dir_entry *e)
{
    size_t nmemb = 0;
    while (e[nmemb].name)
        nmemb++;
    qsort(e, nmemb, sizeof(*e), dir_cmp); 
}

void dir_free(struct dir_entry **e_p)
{
    size_t i = 0;
    struct dir_entry *e = *e_p;

    assert(e);
    while (e[i].name)
        xfree(e[i++].name);
    xfree(e);
    *e_p = NULL;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

