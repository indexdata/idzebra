/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dir.c,v $
 * Revision 1.22  2002-04-04 20:50:36  adam
 * Multi register works with record paths and data1 profile path
 *
 * Revision 1.21  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.20  1999/02/02 14:50:50  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.19  1998/11/03 10:16:11  adam
 * Uses stat and not lstat so that file traversal follows symbolic links.
 *
 * Revision 1.18  1997/09/25 14:55:33  adam
 * Windows port uses stat and not lstat.
 *
 * Revision 1.17  1997/09/09 13:38:06  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.16  1996/10/29 14:06:45  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.15  1996/06/04 10:18:58  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.14  1996/04/09  06:49:18  adam
 * Traversal of directories doesn't follow symbolic links.
 *
 * Revision 1.13  1996/03/21 14:50:08  adam
 * File update uses modify-time instead of change-time.
 *
 * Revision 1.12  1996/02/05  12:29:55  adam
 * Logging reduced a bit.
 * The remaining running time is estimated during register merge.
 *
 * Revision 1.11  1995/11/20  16:59:44  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.10  1995/11/20  11:56:22  adam
 * Work on new traversal.
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
#include <string.h>
#include <assert.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <direntz.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#include "index.h"

struct dir_entry *dir_open (const char *rep, const char *base)
{
    DIR *dir;
    char path[1024];
    char full_rep[1024];
    size_t pathpos;
    struct dirent *dent;
    size_t entry_max = 500;
    size_t idx = 0;
    struct dir_entry *entry;

    if (base && !yaz_is_abspath(rep))
    {
        strcpy (full_rep, base);
        strcat (full_rep, "/");
    }
    else
        *full_rep = '\0';
    strcat (full_rep, rep);

    logf (LOG_LOG, "dir_open %s", full_rep);
    if (!(dir = opendir(full_rep)))
    {
        logf (LOG_WARN|LOG_ERRNO, "opendir %s", rep);
        if (errno != ENOENT && errno != EACCES)
            exit (1);
        return NULL;
    }
    entry = (struct dir_entry *) xmalloc (sizeof(*entry) * entry_max);
    strcpy (path, rep);
    pathpos = strlen(path);
    if (!pathpos || path[pathpos-1] != '/')
        path[pathpos++] = '/';
    while ((dent = readdir (dir)))
    {
        struct stat finfo;
        if (strcmp (dent->d_name, ".") == 0 ||
            strcmp (dent->d_name, "..") == 0)
            continue;
        if (idx == entry_max-1)
        {
            struct dir_entry *entry_n;

            entry_n = (struct dir_entry *)
		xmalloc (sizeof(*entry) * (entry_max += 1000));
            memcpy (entry_n, entry, idx * sizeof(*entry));
            xfree (entry);
            entry = entry_n;
        }
        strcpy (path + pathpos, dent->d_name);

        if (base && !yaz_is_abspath (path))
        {
            strcpy (full_rep, base);
            strcat (full_rep, "/");
            strcat (full_rep, path);
            stat (full_rep, &finfo);
        }
        else
            stat (path, &finfo);
        switch (finfo.st_mode & S_IFMT)
        {
        case S_IFREG:
            entry[idx].kind = dirs_file;
            entry[idx].mtime = finfo.st_mtime;
            entry[idx].name = (char *) xmalloc (strlen(dent->d_name)+1);
            strcpy (entry[idx].name, dent->d_name);
            idx++;
            break;
        case S_IFDIR:
            entry[idx].kind = dirs_dir;
            entry[idx].mtime = finfo.st_mtime;
            entry[idx].name = (char *) xmalloc (strlen(dent->d_name)+2);
            strcpy (entry[idx].name, dent->d_name);
	    strcat (entry[idx].name, "/");
            idx++;
            break;
        }
    }
    entry[idx].name = NULL;
    closedir (dir);
    logf (LOG_DEBUG, "dir_close");
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
        xfree (e[i++].name);
    xfree (e);
    *e_p = NULL;
}
