/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dir.c,v $
 * Revision 1.3  1995-09-01 14:06:35  adam
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
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

#include <util.h>
#include "index.h"

struct dir_entry *dir_open (const char *rep)
{
    DIR *dir;
    struct dirent *dent;
    size_t entry_max = 20;
    size_t idx = 0;
    struct dir_entry *entry;

    log (LOG_DEBUG, "dir_open %s", rep);
    if (!(dir = opendir(rep)))
    {
        log (LOG_WARN|LOG_ERRNO, "opendir %s", rep);
        if (errno != ENOENT)
            exit (1);
        return NULL;
    }
    if (!(entry = malloc (sizeof(*entry) * entry_max)))
    {
        log (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }    
    while ((dent = readdir (dir)))
    {
        if (strcmp (dent->d_name, ".") == 0 ||
            strcmp (dent->d_name, "..") == 0)
            continue;
        if (idx == entry_max-1)
        {
            struct dir_entry *entry_n;

            if (!(entry_n = malloc (sizeof(*entry) * (entry_max + 100))))
            {
                log (LOG_FATAL|LOG_ERRNO, "malloc");
                exit (1);
            }
            memcpy (entry_n, entry, idx * sizeof(*entry));
            free (entry);
            entry = entry_n;
            entry_max += 100;
        }
        if (!(entry[idx].name = malloc (strlen(dent->d_name)+1)))
        {
            log (LOG_FATAL|LOG_ERRNO, "malloc");
            exit (1);
        }
        strcpy (entry[idx].name, dent->d_name);
        idx++;
    }
    entry[idx].name = NULL;
    closedir (dir);
    return entry;
}

static int dir_cmp (const void *p1, const void *p2)
{
    return strcmp (((struct dir_entry *) p1)->name,
                   ((struct dir_entry *) p2)->name);
}

void dir_sort (struct dir_entry *e)
{
    size_t nmemb = 0;
    while (e[nmemb].name)
        nmemb++;
    qsort (e, nmemb, sizeof(*e), dir_cmp); 
}

void dir_free (struct dir_entry **e_p)
{
    size_t i = 0;
    struct dir_entry *e = *e_p;

    assert (e);
    while (e[i].name)
        free (e[i++].name);
    free (e);
    *e_p = NULL;
}
