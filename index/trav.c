/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: trav.c,v $
 * Revision 1.11  1995-11-22 17:19:19  adam
 * Record management uses the bfile system.
 *
 * Revision 1.10  1995/11/21  15:01:16  adam
 * New general match criteria implemented.
 * New feature: document groups.
 *
 * Revision 1.9  1995/11/21  09:20:32  adam
 * Yet more work on record match.
 *
 * Revision 1.8  1995/11/20  16:59:46  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.7  1995/11/20  11:56:28  adam
 * Work on new traversal.
 *
 * Revision 1.6  1995/11/17  15:54:42  adam
 * Started work on virtual directory structure.
 *
 * Revision 1.5  1995/10/17  18:02:09  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.4  1995/09/28  09:19:46  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.3  1995/09/06  16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.2  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/01  14:06:36  adam
 * Split of work into more files.
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

#include <alexutil.h>
#include "index.h"

static int repComp (const char *a, const char *b, size_t len)
{
    if (!len)
        return 0;
    return memcmp (a, b, len);
}

static void repositoryExtractR (int deleteFlag, char *rep,
                                struct recordGroup *rGroup)
{
    struct dir_entry *e;
    int i;
    size_t rep_len = strlen (rep);

    e = dir_open (rep);
    if (!e)
        return;
    if (rep[rep_len-1] != '/')
        rep[rep_len] = '/';
    else
        --rep_len;
    for (i=0; e[i].name; i++)
    {
        strcpy (rep +rep_len+1, e[i].name);
        switch (e[i].kind)
        {
        case dirs_file:
            fileExtract (NULL, rep, rGroup, deleteFlag);
            break;
        case dirs_dir:
            repositoryExtractR (deleteFlag, rep, rGroup);
            break;
        }
    }
    dir_free (&e);

}

static void repositoryUpdateR (struct dirs_info *di, struct dirs_entry *dst,
                               const char *base, char *src, 
                               struct recordGroup *rGroup)
{
    struct dir_entry *e_src;
    int i_src = 0;
    static char tmppath[256];
    size_t src_len = strlen (src);

    sprintf (tmppath, "%s%s", base, src);
    e_src = dir_open (tmppath);

#if 1
    if (!dst || repComp (dst->path, src, src_len))
#else
    if (!dst || strcmp (dst->path, src))
#endif
    {
        if (!e_src)
            return;
        if (src_len && src[src_len-1] == '/')
            --src_len;
        else
            src[src_len] = '/';
        src[src_len+1] = '\0';
        dirs_mkdir (di, src, 0);
        dst = NULL;
    }
    else if (!e_src)
    {
        /* delete tree dst */
        return;
    }
    else
    {
        if (src_len && src[src_len-1] == '/')
            --src_len;
        else
            src[src_len] = '/';
        src[src_len+1] = '\0';
        dst = dirs_read (di); 
    }
    dir_sort (e_src);

    while (1)
    {
        int sd;

        if (dst && !repComp (dst->path, src, src_len+1))
        {
            if (e_src[i_src].name)
            {
                logf (LOG_DEBUG, "dst=%s src=%s", dst->path + src_len+1, 
		      e_src[i_src].name);
                sd = strcmp (dst->path + src_len+1, e_src[i_src].name);
            }
            else
                sd = -1;
        }
        else if (e_src[i_src].name)
            sd = 1;
        else
            break;
        logf (LOG_DEBUG, "trav sd=%d", sd);
        if (sd == 0)
        {
            strcpy (src + src_len+1, e_src[i_src].name);
            sprintf (tmppath, "%s%s", base, src);
            
            switch (e_src[i_src].kind)
            {
            case dirs_file:
                if (e_src[i_src].ctime > dst->ctime)
                {
                    if (fileExtract (&dst->sysno, tmppath, rGroup, 0))
                        dirs_add (di, src, dst->sysno, e_src[i_src].ctime);
                }
                dst = dirs_read (di);
                break;
            case dirs_dir:
                repositoryUpdateR (di, dst, base, src, rGroup);
                dst = dirs_last (di);
                logf (LOG_DEBUG, "last is %s", dst ? dst->path : "null");
                break;
            default:
                dst = dirs_read (di); 
            }
            i_src++;
        }
        else if (sd > 0)
        {
            SYSNO sysno = 0;
            strcpy (src + src_len+1, e_src[i_src].name);
            sprintf (tmppath, "%s%s", base, src);

            switch (e_src[i_src].kind)
            {
            case dirs_file:
                if (fileExtract (&sysno, tmppath, rGroup, 0))
                    dirs_add (di, src, sysno, e_src[i_src].ctime);            
                break;
            case dirs_dir:
                repositoryUpdateR (di, dst, base, src, rGroup);
                if (dst)
                    dst = dirs_last (di);
                break;
            }
            i_src++;
        }
        else  /* sd < 0 */
        {
            assert (0);
        }
    }
    dir_free (&e_src);
}

void repositoryUpdate (struct recordGroup *rGroup)
{
    struct dirs_info *di;
    char src[256];
    Dict dict;

    dict = dict_open ("repdict", 40, 1);

    assert (rGroup->path);
    di = dirs_open (dict, rGroup->path);
    strcpy (src, "");
    repositoryUpdateR (di, dirs_read (di), rGroup->path, src, rGroup);
    dirs_free (&di);

    dict_close (dict);
}

void repositoryDelete (struct recordGroup *rGroup)
{
    char src[256];

    assert (rGroup->path);
    strcpy (src, rGroup->path);
    repositoryExtractR (1, src, rGroup);
}

void repositoryAdd (struct recordGroup *rGroup)
{
    char src[256];

    assert (rGroup->path);
    strcpy (src, rGroup->path);
    repositoryExtractR (0, src, rGroup);
}

