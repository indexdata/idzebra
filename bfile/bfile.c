/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.c,v $
 * Revision 1.12  1995-11-30 08:33:10  adam
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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <alexutil.h>
#include <bfile.h>
#include "cfile.h"

static const char *cache_name = NULL;

void bf_cache (const char *name)
{
    cache_name = name;
}

int bf_close (BFile bf)
{
    if (bf->cf)
        cf_close (bf->cf);
    mf_close (bf->mf);
    xfree (bf);
    return 0;
}

BFile bf_open (const char *name, int block_size, int wflag)
{
    BFile tmp = xmalloc(sizeof(BFile_struct));

    if (cache_name)
    {
        FILE *outf;
        int first_time;

        logf (LOG_LOG, "cf,mf_open %s, cache_name=%s", name, cache_name);
        tmp->mf = mf_open(0, name, block_size, wflag);
        tmp->cf = cf_open(tmp->mf, cache_name, name, block_size, wflag,
                          &first_time);

        if (first_time)
        {
            outf = fopen (cache_name, "a");
            fprintf (outf, "%s %d\n", name, block_size);
            fclose (outf);
        }
    }
    else
    {
        tmp->mf = mf_open(0, name, block_size, wflag);
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

int bf_read (BFile bf, int no, int offset, int num, void *buf)
{
    int r;

    if (bf->cf && (r=cf_read (bf->cf, no, offset, num, buf)) != -1)
        return r;
    return mf_read (bf->mf, no, offset, num, buf);
}

int bf_write (BFile bf, int no, int offset, int num, const void *buf)
{
    if (bf->cf)
        return cf_write (bf->cf, no, offset, num, buf);
    return mf_write (bf->mf, no, offset, num, buf);
}

void bf_commit (const char *name)
{
    FILE *inf;
    int block_size;
    char path[256];
    MFile mf;
    CFile cf;
    int first_time;

    if (!(inf = fopen (name, "r")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "cannot open commit %s", name);
        exit (1);
    }
    while (fscanf (inf, "%s %d", path, &block_size) == 1)
    {
        mf = mf_open(0, path, block_size, 1);
        cf = cf_open(mf, name, path, block_size, 0, &first_time);

        cf_commit (cf);

        cf_close (cf);
        mf_close (mf);
    }
    fclose (inf);
}
