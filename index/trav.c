/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: trav.c,v $
 * Revision 1.24  1996-04-26 10:00:23  adam
 * Added option -V to zebraidx to display version information.
 * Removed stupid warnings from file update.
 *
 * Revision 1.23  1996/04/12  07:02:25  adam
 * File update of single files.
 *
 * Revision 1.22  1996/04/09 06:50:50  adam
 * Bug fix: bad reference in function fileUpdateR.
 *
 * Revision 1.21  1996/03/22 15:34:18  quinn
 * Fixed bad reference
 *
 * Revision 1.20  1996/03/21  14:50:10  adam
 * File update uses modify-time instead of change-time.
 *
 * Revision 1.19  1996/03/20  16:16:55  quinn
 * Added diagnostic output
 *
 * Revision 1.18  1996/03/19  12:43:27  adam
 * Bug fix: File update traversal didn't handle trailing slashes correctly.
 * Bug fix: Update of sub directory groups wasn't handled correctly.
 *
 * Revision 1.17  1996/02/12  18:45:17  adam
 * Changed naming of some functions.
 *
 * Revision 1.16  1996/02/05  12:30:02  adam
 * Logging reduced a bit.
 * The remaining running time is estimated during register merge.
 *
 * Revision 1.15  1995/12/07  17:38:48  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.14  1995/12/06  12:41:26  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.13  1995/11/28  09:09:46  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.12  1995/11/24  11:31:37  adam
 * Commands add & del read filenames from stdin if source directory is
 * empty.
 * Match criteria supports 'constant' strings.
 *
 * Revision 1.11  1995/11/22  17:19:19  adam
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
#include <time.h>

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
    logf (LOG_LOG, "Dir: %s", rep);
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

static void stdinExtractR (int deleteFlag, struct recordGroup *rGroup)
{
    char tmppath[1024];

    logf (LOG_LOG, "stdinExtractR");
    while (scanf ("%s", tmppath) == 1)
        fileExtract (NULL, tmppath, rGroup, deleteFlag);
}

static void fileDeleteR (struct dirs_info *di, struct dirs_entry *dst,
			       const char *base, char *src,
			       struct recordGroup *rGroup)
{
    char tmppath[1024];
    size_t src_len = strlen (src);

    while (dst && !repComp (dst->path, src, src_len+1))
    {
        switch (dst->kind)
        {
        case dirs_file:
            sprintf (tmppath, "%s%s", base, dst->path);
            fileExtract (&dst->sysno, tmppath, rGroup, 1);
             
            strcpy (tmppath, dst->path);
            dst = dirs_read (di); 
            dirs_del (di, tmppath);
            break;
        case dirs_dir:
            strcpy (tmppath, dst->path);
            dst = dirs_read (di);
            dirs_rmdir (di, tmppath);
            break;
        default:
            dst = dirs_read (di);
        }
    }
}

static void fileUpdateR (struct dirs_info *di, struct dirs_entry *dst,
                               const char *base, char *src, 
                               struct recordGroup *rGroup)
{
    struct dir_entry *e_src;
    int i_src = 0;
    static char tmppath[1024];
    size_t src_len = strlen (src);

    sprintf (tmppath, "%s%s", base, src);
    e_src = dir_open (tmppath);
    logf (LOG_LOG, "Dir: %s", tmppath);

#if 0
    if (!dst || repComp (dst->path, src, src_len))
#else
    if (!dst || strcmp (dst->path, src))
#endif
    {
        if (!e_src)
            return;

        if (src_len && src[src_len-1] != '/')
        {
            src[src_len] = '/';
            src[++src_len] = '\0';
        }
        dirs_mkdir (di, src, 0);
        if (dst && repComp (dst->path, src, src_len))
            dst = NULL;
    }
    else if (!e_src)
    {
        strcpy (src, dst->path);
        fileDeleteR (di, dst, base, src, rGroup);
        return;
    }
    else
    {
        if (src_len && src[src_len-1] != '/')
        {
            src[src_len] = '/';
            src[++src_len] = '\0';
        }
        dst = dirs_read (di); 
    }
    dir_sort (e_src);

    while (1)
    {
        int sd;

        if (dst && !repComp (dst->path, src, src_len))
        {
            if (e_src[i_src].name)
            {
                logf (LOG_DEBUG, "dst=%s src=%s", dst->path + src_len,
		      e_src[i_src].name);
                sd = strcmp (dst->path + src_len, e_src[i_src].name);
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
            strcpy (src + src_len, e_src[i_src].name);
            sprintf (tmppath, "%s%s", base, src);
            
            switch (e_src[i_src].kind)
            {
            case dirs_file:
                if (e_src[i_src].mtime > dst->mtime)
                {
                    if (fileExtract (&dst->sysno, tmppath, rGroup, 0))
                    {
                        dirs_add (di, src, dst->sysno, e_src[i_src].mtime);
                    }
		    logf (LOG_LOG, "old: %s", ctime (&dst->mtime));
                    logf (LOG_LOG, "new: %s", ctime (&e_src[i_src].mtime));
                }
                dst = dirs_read (di);
                break;
            case dirs_dir:
                fileUpdateR (di, dst, base, src, rGroup);
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
            strcpy (src + src_len, e_src[i_src].name);
            sprintf (tmppath, "%s%s", base, src);

            switch (e_src[i_src].kind)
            {
            case dirs_file:
                if (fileExtract (&sysno, tmppath, rGroup, 0))
                    dirs_add (di, src, sysno, e_src[i_src].mtime);            
                break;
            case dirs_dir:
                fileUpdateR (di, dst, base, src, rGroup);
                if (dst)
                    dst = dirs_last (di);
                break;
            }
            i_src++;
        }
        else  /* sd < 0 */
        {
            strcpy (src, dst->path);
            sprintf (tmppath, "%s%s", base, dst->path);

            switch (dst->kind)
            {
            case dirs_file:
                fileExtract (&dst->sysno, tmppath, rGroup, 1);
                dirs_del (di, dst->path);
                dst = dirs_read (di);
                break;
            case dirs_dir:
                fileDeleteR (di, dst, base, src, rGroup);
                dst = dirs_last (di);
            }
        }
    }
    dir_free (&e_src);
}

static void groupRes (struct recordGroup *rGroup)
{
    char resStr[256];
    char gPrefix[256];

    if (!rGroup->groupName || !*rGroup->groupName)
        *gPrefix = '\0';
    else
        sprintf (gPrefix, "%s.", rGroup->groupName);

    sprintf (resStr, "%srecordId", gPrefix);
    rGroup->recordId = res_get (common_resource, resStr);
}


void repositoryShow (struct recordGroup *rGroup)
{
    char src[1024];
    int src_len;
    struct dirs_entry *dst;
    Dict dict;
    struct dirs_info *di;
    
    if (!(dict = dict_open (FMATCH_DICT, 50, 1)))
    {
        logf (LOG_FATAL, "dict_open fail of %s", FMATCH_DICT);
        exit (1);
    }
    
    assert (rGroup->path);    
    strcpy (src, rGroup->path);
    src_len = strlen (src);
    
    if (src_len && src[src_len-1] != '/')
    {
        src[src_len] = '/';
        src[++src_len] = '\0';
    }
    
    di = dirs_open (dict, src);
    
    while ( (dst = dirs_read (di)) )
        logf (LOG_LOG, "%s", dst->path);
    dirs_free (&di);
    dict_close (dict);
}

static void fileUpdate (Dict dict, struct recordGroup *rGroup,
                        const char *path)
{
    struct dirs_info *di;
    struct stat sbuf;
    char src[1024];
    char dst[1024];
    int src_len;

    assert (path);
    strcpy (src, path);
    src_len = strlen (src);

    stat (src, &sbuf);
    if (S_ISREG(sbuf.st_mode))
    {
        struct dirs_entry *e_dst;
        di = dirs_fopen (dict, src);

        e_dst = dirs_read (di);
        if (e_dst)
        {
            if (sbuf.st_mtime > e_dst->mtime)
                if (fileExtract (&e_dst->sysno, src, rGroup, 0))
                    dirs_add (di, src, e_dst->sysno, sbuf.st_mtime);
        }
        else
        {
            SYSNO sysno = 0;
            if (fileExtract (&sysno, src, rGroup, 0))
                 dirs_add (di, src, sysno, sbuf.st_mtime);
        }
        dirs_free (&di);
    }
    else if (S_ISDIR(sbuf.st_mode))
    {
        if (src_len && src[src_len-1] != '/')
        {
            src[src_len] = '/';
            src[++src_len] = '\0';
        }
        di = dirs_open (dict, src);
        *dst = '\0';
        fileUpdateR (di, dirs_read (di), src, dst, rGroup);
        dirs_free (&di);
    }
    else
    {
        logf (LOG_WARN, "Cannot handle file %s", src);
    }
}

void repositoryUpdate (struct recordGroup *rGroup)
{
    groupRes (rGroup);
    assert (rGroup->path);
    if (rGroup->recordId && !strcmp (rGroup->recordId, "file"))
    {
        Dict dict;
        if (!(dict = dict_open (FMATCH_DICT, 50, 1)))
        {
            logf (LOG_FATAL, "dict_open fail of %s", FMATCH_DICT);
            exit (1);
        }
        if (*rGroup->path == '\0' || !strcmp(rGroup->path, "-"))
        {
            char src[1024];
            while (scanf ("%s", src) == 1)
                fileUpdate (dict, rGroup, src);
        }
        else
            fileUpdate (dict, rGroup, rGroup->path);
        dict_close (dict);
    }
    else 
    {
        char src[1024];

        strcpy (src, rGroup->path);
        if (*src == '\0' || !strcmp (src, "-"))
            stdinExtractR (0, rGroup);
        else
            repositoryExtractR (0, src, rGroup);
    }
}

void repositoryDelete (struct recordGroup *rGroup)
{
    char src[256];

    assert (rGroup->path);
    groupRes (rGroup);
    strcpy (src, rGroup->path);
    if (*src == '\0' || !strcmp(src, "-"))
	stdinExtractR (1, rGroup);
    else
	repositoryExtractR (1, src, rGroup);
}

