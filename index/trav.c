/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: trav.c,v $
 * Revision 1.37  2002-02-20 17:30:01  adam
 * Work on new API. Locking system re-implemented
 *
 * Revision 1.36  1999/05/15 14:36:38  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.35  1999/02/02 14:51:09  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.34  1998/06/08 14:43:14  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.33  1998/01/12 15:04:08  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.32  1997/09/25 14:56:51  adam
 * Windows NT interface code to the stat call.
 *
 * Revision 1.31  1997/09/17 12:19:17  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.30  1997/09/09 13:38:09  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.29  1997/02/12 20:39:47  adam
 * Implemented options -f <n> that limits the log to the first <n>
 * records.
 * Changed some log messages also.
 *
 * Revision 1.28  1996/11/01 08:58:44  adam
 * Interface to isamc system now includes update and delete.
 *
 * Revision 1.27  1996/10/29 14:06:56  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.26  1996/06/04 10:19:01  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.25  1996/05/01  13:46:37  adam
 * First work on multiple records in one file.
 * New option, -offset, to the "unread" command in the filter module.
 *
 * Revision 1.24  1996/04/26  10:00:23  adam
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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#define S_ISREG(x) (x & _S_IFREG)
#define S_ISDIR(x) (x & _S_IFDIR)
#else
#include <unistd.h>
#endif
#include <direntz.h>
#include <fcntl.h>
#include <time.h>

#include "index.h"
#include "zserver.h"

static int repComp (const char *a, const char *b, size_t len)
{
    if (!len)
        return 0;
    return memcmp (a, b, len);
}

static void repositoryExtractR (ZebraHandle zh, int deleteFlag, char *rep,
                                struct recordGroup *rGroup,
				int level)
{
    struct dir_entry *e;
    int i;
    size_t rep_len = strlen (rep);

    e = dir_open (rep);
    if (!e)
        return;
    logf (LOG_LOG, "dir %s", rep);
    if (rep[rep_len-1] != '/')
        rep[rep_len] = '/';
    else
        --rep_len;
    
    for (i=0; e[i].name; i++)
    {
	char *ecp;
        strcpy (rep +rep_len+1, e[i].name);
	if ((ecp = strrchr (e[i].name, '/')))
	    *ecp = '\0';
	if (level == 0 && rGroup->databaseNamePath)
	    rGroup->databaseName = e[i].name;

        switch (e[i].kind)
        {
        case dirs_file:
            fileExtract (zh, NULL, rep, rGroup, deleteFlag);
            break;
        case dirs_dir:
            repositoryExtractR (zh, deleteFlag, rep, rGroup, level+1);
            break;
        }
    }
    dir_free (&e);

}

static void fileDeleteR (ZebraHandle zh,
                         struct dirs_info *di, struct dirs_entry *dst,
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
            fileExtract (zh, &dst->sysno, tmppath, rGroup, 1);
             
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

static void fileUpdateR (ZebraHandle zh,
                         struct dirs_info *di, struct dirs_entry *dst,
			 const char *base, char *src, 
			 struct recordGroup *rGroup,
			 int level)
{
    struct dir_entry *e_src;
    int i_src = 0;
    static char tmppath[1024];
    size_t src_len = strlen (src);

    sprintf (tmppath, "%s%s", base, src);
    e_src = dir_open (tmppath);
    logf (LOG_LOG, "dir %s", tmppath);

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
        fileDeleteR (zh, di, dst, base, src, rGroup);
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

	if (level == 0 && rGroup->databaseNamePath)
	    rGroup->databaseName = e_src[i_src].name;
        if (sd == 0)
        {
            strcpy (src + src_len, e_src[i_src].name);
            sprintf (tmppath, "%s%s", base, src);
            
            switch (e_src[i_src].kind)
            {
            case dirs_file:
                if (e_src[i_src].mtime > dst->mtime)
                {
                    if (fileExtract (zh, &dst->sysno, tmppath, rGroup, 0))
                    {
                        dirs_add (di, src, dst->sysno, e_src[i_src].mtime);
                    }
		    logf (LOG_DEBUG, "old: %s", ctime (&dst->mtime));
                    logf (LOG_DEBUG, "new: %s", ctime (&e_src[i_src].mtime));
                }
                dst = dirs_read (di);
                break;
            case dirs_dir:
                fileUpdateR (zh, di, dst, base, src, rGroup, level+1);
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
                if (fileExtract (zh, &sysno, tmppath, rGroup, 0))
                    dirs_add (di, src, sysno, e_src[i_src].mtime);            
                break;
            case dirs_dir:
                fileUpdateR (zh, di, dst, base, src, rGroup, level+1);
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
                fileExtract (zh, &dst->sysno, tmppath, rGroup, 1);
                dirs_del (di, dst->path);
                dst = dirs_read (di);
                break;
            case dirs_dir:
                fileDeleteR (zh, di, dst, base, src, rGroup);
                dst = dirs_last (di);
            }
        }
    }
    dir_free (&e_src);
}

static void groupRes (ZebraService zs, struct recordGroup *rGroup)
{
    char resStr[256];
    char gPrefix[256];

    if (!rGroup->groupName || !*rGroup->groupName)
        *gPrefix = '\0';
    else
        sprintf (gPrefix, "%s.", rGroup->groupName);

    sprintf (resStr, "%srecordId", gPrefix);
    rGroup->recordId = res_get (zs->res, resStr);
    sprintf (resStr, "%sdatabasePath", gPrefix);
    rGroup->databaseNamePath =
	atoi (res_get_def (zs->res, resStr, "0"));
}

void repositoryShow (ZebraHandle zh)
                     
{
    struct recordGroup *rGroup = &zh->rGroup;
    char src[1024];
    int src_len;
    struct dirs_entry *dst;
    Dict dict;
    struct dirs_info *di;
    
    if (!(dict = dict_open (zh->service->bfs, FMATCH_DICT, 50, 0, 0)))
    {
        logf (LOG_FATAL, "dict_open fail of %s", FMATCH_DICT);
	return;
    }
    
    assert (rGroup->path);    
    strcpy (src, rGroup->path);
    src_len = strlen (src);
    
    if (src_len && src[src_len-1] != '/')
    {
        src[src_len] = '/';
        src[++src_len] = '\0';
    }
    
    di = dirs_open (dict, src, rGroup->flagRw);
    
    while ( (dst = dirs_read (di)) )
        logf (LOG_LOG, "%s", dst->path);
    dirs_free (&di);
    dict_close (dict);
}

static void fileUpdate (ZebraHandle zh,
                        Dict dict, struct recordGroup *rGroup,
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
                if (fileExtract (zh, &e_dst->sysno, src, rGroup, 0))
                    dirs_add (di, src, e_dst->sysno, sbuf.st_mtime);
        }
        else
        {
            SYSNO sysno = 0;
            if (fileExtract (zh, &sysno, src, rGroup, 0))
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
        di = dirs_open (dict, src, rGroup->flagRw);
        *dst = '\0';
        fileUpdateR (zh, di, dirs_read (di), src, dst, rGroup, 0);
        dirs_free (&di);
    }
    else
    {
        logf (LOG_WARN, "Ignoring path %s", src);
    }
}


static void repositoryExtract (ZebraHandle zh,
                               int deleteFlag, struct recordGroup *rGroup,
                               const char *path)
{
    struct stat sbuf;
    char src[1024];

    assert (path);
    strcpy (src, path);

    stat (src, &sbuf);
    if (S_ISREG(sbuf.st_mode))
        fileExtract (zh, NULL, src, rGroup, deleteFlag);
    else if (S_ISDIR(sbuf.st_mode))
	repositoryExtractR (zh, deleteFlag, src, rGroup, 0);
    else
        logf (LOG_WARN, "Ignoring path %s", src);
}

static void repositoryExtractG (ZebraHandle zh,
                                int deleteFlag, struct recordGroup *rGroup)
{
    if (*rGroup->path == '\0' || !strcmp(rGroup->path, "-"))
    {
        char src[1024];

        while (scanf ("%s", src) == 1)
            repositoryExtract (zh, deleteFlag, rGroup, src);
    }
    else
        repositoryExtract (zh, deleteFlag, rGroup, rGroup->path);
}

void repositoryUpdate (ZebraHandle zh)
{
    struct recordGroup *rGroup = &zh->rGroup;
    groupRes (zh->service, rGroup);
    assert (rGroup->path);
    if (rGroup->recordId && !strcmp (rGroup->recordId, "file"))
    {
        Dict dict;
        if (!(dict = dict_open (zh->service->bfs, FMATCH_DICT, 50,
				rGroup->flagRw, 0)))
        {
            logf (LOG_FATAL, "dict_open fail of %s", FMATCH_DICT);
	    return ;
        }
        if (*rGroup->path == '\0' || !strcmp(rGroup->path, "-"))
        {
            char src[1024];
            while (scanf ("%s", src) == 1)
                fileUpdate (zh, dict, rGroup, src);
        }
        else
            fileUpdate (zh, dict, rGroup, rGroup->path);
        dict_close (dict);
    }
    else 
        repositoryExtractG (zh, 0, rGroup);
}

void repositoryDelete (ZebraHandle zh)
{
    repositoryExtractG (zh, 1, &zh->rGroup);
}

