/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.c,v $
 * Revision 1.28  1999-05-12 13:08:05  adam
 * First version of ISAMS.
 *
 * Revision 1.27  1999/02/02 14:50:01  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.26  1998/02/17 10:32:52  adam
 * Fixed bug: binary files weren't opened with flag b on NT.
 *
 * Revision 1.25  1997/10/27 14:25:38  adam
 * Fixed memory leaks.
 *
 * Revision 1.24  1997/09/18 08:59:16  adam
 * Extra generic handle for the character mapping routines.
 *
 * Revision 1.23  1997/09/17 12:19:06  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.22  1997/09/09 13:37:52  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.21  1996/10/29 13:56:13  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.20  1996/03/26 15:59:04  adam
 * The directory of the shadow table file can be specified by the new
 * bf_lockDir call.
 *
 * Revision 1.19  1996/02/05  12:28:58  adam
 * Removed a LOG_LOG message.
 *
 * Revision 1.18  1996/01/02  08:59:06  quinn
 * Changed "commit" setting to "shadow".
 *
 * Revision 1.17  1995/12/11  09:03:51  adam
 * New function: cf_unlink.
 * New member of commit file head: state (0) deleted, (1) hash file.
 *
 * Revision 1.16  1995/12/08  16:21:13  adam
 * Work on commit/update.
 *
 * Revision 1.15  1995/12/01  16:24:28  adam
 * Commit files use separate meta file area.
 *
 * Revision 1.14  1995/12/01  11:37:21  adam
 * Cached/commit files implemented as meta-files.
 *
 * Revision 1.13  1995/11/30  17:00:49  adam
 * Several bug fixes. Commit system runs now.
 *
 * Revision 1.12  1995/11/30  08:33:10  adam
 * Started work on commit facility.
 *
 * Revision 1.11  1995/09/04  12:33:21  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.10  1994/08/25  10:15:54  quinn
 * Trivial
 *
 * Revision 1.9  1994/08/24  08:45:48  quinn
 * Using mfile.
 *
 * Revision 1.8  1994/08/23  15:03:34  quinn
 * *** empty log message ***
 *
 * Revision 1.7  1994/08/23  14:25:45  quinn
 * Added O_CREAT because some geek wanted it. Sheesh.
 *
 * Revision 1.6  1994/08/23  14:21:38  quinn
 * Fixed call to log
 *
 * Revision 1.5  1994/08/18  08:10:08  quinn
 * Minimal changes
 *
 * Revision 1.4  1994/08/17  14:27:32  quinn
 * last mods
 *
 * Revision 1.2  1994/08/17  14:09:32  quinn
 * Compiles cleanly (still only dummy).
 *
 * Revision 1.1  1994/08/17  13:55:08  quinn
 * New blocksystem. dummy only
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <zebrautl.h>
#include <bfile.h>
#include "cfile.h"

struct BFiles_struct {
    MFile_area commit_area;
    MFile_area_struct *register_area;
    char *lockDir;
};

BFiles bfs_create (const char *spec)
{
    BFiles bfs = xmalloc (sizeof(*bfs));
    bfs->commit_area = NULL;
    bfs->register_area = mf_init("register", spec);
    bfs->lockDir = NULL;
    return bfs;
}

void bfs_destroy (BFiles bfs)
{
    xfree (bfs->lockDir);
    mf_destroy (bfs->commit_area);
    mf_destroy (bfs->register_area);
    xfree (bfs);
}

static FILE *open_cache (BFiles bfs, const char *flags)
{
    char cacheFilename[1024];
    FILE *file;

    sprintf (cacheFilename, "%scache",
	     bfs->lockDir ? bfs->lockDir : "");
    file = fopen (cacheFilename, flags);
    return file;
}

static void unlink_cache (BFiles bfs)
{
    char cacheFilename[1024];

    sprintf (cacheFilename, "%scache",
	     bfs->lockDir ? bfs->lockDir : "");
    unlink (cacheFilename);
}

void bf_lockDir (BFiles bfs, const char *lockDir)
{
    size_t len;
    
    xfree (bfs->lockDir);
    if (lockDir == NULL)
        lockDir = "";
    len = strlen(lockDir);
    bfs->lockDir = xmalloc (len+2);
    strcpy (bfs->lockDir, lockDir);
    
    if (len > 0 && bfs->lockDir[len-1] != '/')
        strcpy (bfs->lockDir + len, "/");
}

void bf_cache (BFiles bfs, const char *spec)
{
    if (spec)
    {
        if (!bfs->commit_area)
	    bfs->commit_area = mf_init ("shadow", spec);
    }
    else
        bfs->commit_area = NULL;
}

int bf_close (BFile bf)
{
    if (bf->cf)
        cf_close (bf->cf);
    mf_close (bf->mf);
    xfree (bf);
    return 0;
}

BFile bf_open (BFiles bfs, const char *name, int block_size, int wflag)
{
    BFile tmp = xmalloc(sizeof(BFile_struct));

    if (bfs->commit_area)
    {
        int first_time;

        tmp->mf = mf_open (bfs->register_area, name, block_size, 0);
        tmp->cf = cf_open (tmp->mf, bfs->commit_area, name, block_size,
                           wflag, &first_time);
        if (first_time)
        {
            FILE *outf;

            outf = open_cache (bfs, "ab");
            if (!outf)
            {
                logf (LOG_FATAL|LOG_ERRNO, "open %scache",
                      bfs->lockDir ? bfs->lockDir : "");
                exit (1);
            }
            fprintf (outf, "%s %d\n", name, block_size);
            fclose (outf);
        }
    }
    else
    {
        tmp->mf = mf_open(bfs->register_area, name, block_size, wflag);
        tmp->cf = NULL;
    }
    if (!tmp->mf)
    {
        logf (LOG_FATAL, "mf_open failed for %s", name);
        xfree (tmp);
        return 0;
    }
    return(tmp);
}

int bf_read (BFile bf, int no, int offset, int nbytes, void *buf)
{
    int r;

    if (bf->cf && (r=cf_read (bf->cf, no, offset, nbytes, buf)) != -1)
        return r;
    return mf_read (bf->mf, no, offset, nbytes, buf);
}

int bf_write (BFile bf, int no, int offset, int nbytes, const void *buf)
{
    if (bf->cf)
        return cf_write (bf->cf, no, offset, nbytes, buf);
    return mf_write (bf->mf, no, offset, nbytes, buf);
}

int bf_commitExists (BFiles bfs)
{
    FILE *inf;

    inf = open_cache (bfs, "rb");
    if (inf)
    {
        fclose (inf);
        return 1;
    }
    return 0;
}

void bf_commitExec (BFiles bfs)
{
    FILE *inf;
    int block_size;
    char path[256];
    MFile mf;
    CFile cf;
    int first_time;

    assert (bfs->commit_area);
    if (!(inf = open_cache (bfs, "rb")))
    {
        logf (LOG_LOG, "No commit file");
        return ;
    }
    while (fscanf (inf, "%s %d", path, &block_size) == 2)
    {
        mf = mf_open (bfs->register_area, path, block_size, 1);
        cf = cf_open (mf, bfs->commit_area, path, block_size, 0, &first_time);

        cf_commit (cf);

        cf_close (cf);
        mf_close (mf);
    }
    fclose (inf);
}

void bf_commitClean (BFiles bfs, const char *spec)
{
    FILE *inf;
    int block_size;
    char path[256];
    MFile mf;
    CFile cf;
    int mustDisable = 0;
    int firstTime;

    if (!bfs->commit_area)
    {
        bf_cache (bfs, spec);
        mustDisable = 1;
    }

    if (!(inf = open_cache (bfs, "rb")))
        return ;
    while (fscanf (inf, "%s %d", path, &block_size) == 2)
    {
        mf = mf_open (bfs->register_area, path, block_size, 0);
        cf = cf_open (mf, bfs->commit_area, path, block_size, 1, &firstTime);
        cf_unlink (cf);
        cf_close (cf);
        mf_close (mf);
    }
    fclose (inf);
    unlink_cache (bfs);
    if (mustDisable)
        bf_cache (bfs, 0);
}
