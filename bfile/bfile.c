/* $Id: bfile.c,v 1.35.2.1 2006-08-14 10:38:50 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

This file is part of the Zebra server.

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
    char *base;
    char *cache_fname;
};

BFiles bfs_create (const char *spec, const char *base)
{
    BFiles bfs = (BFiles) xmalloc (sizeof(*bfs));
    bfs->commit_area = NULL;
    bfs->base = 0;
    bfs->cache_fname = 0;
    if (base)
        bfs->base = xstrdup (base);
    bfs->register_area = mf_init("register", spec, base);
    if (!bfs->register_area)
    {
        bfs_destroy(bfs);
        return 0;
    }
    return bfs;
}

void bfs_destroy (BFiles bfs)
{
    if (!bfs)
        return;
    xfree (bfs->cache_fname);
    xfree (bfs->base);
    mf_destroy (bfs->commit_area);
    mf_destroy (bfs->register_area);
    xfree (bfs);
}

static FILE *open_cache (BFiles bfs, const char *flags)
{
    FILE *file;

    file = fopen (bfs->cache_fname, flags);
    return file;
}

static void unlink_cache (BFiles bfs)
{
    unlink (bfs->cache_fname);
}

void bf_cache (BFiles bfs, const char *spec)
{
    if (spec)
    {
        yaz_log (LOG_LOG, "enabling cache spec=%s", spec);
        if (!bfs->commit_area)
	    bfs->commit_area = mf_init ("shadow", spec, bfs->base);
        if (bfs->commit_area)
        {
            bfs->cache_fname = xmalloc (strlen(bfs->commit_area->dirs->name)+
                                       8);
            strcpy (bfs->cache_fname, bfs->commit_area->dirs->name);
            strcat (bfs->cache_fname, "/cache");
            yaz_log (LOG_LOG, "cache_fname = %s", bfs->cache_fname);
        }
    }
    else
        bfs->commit_area = NULL;
}

int bf_close (BFile bf)
{
    zebra_lock_rdwr_destroy (&bf->rdwr_lock);
    if (bf->cf)
        cf_close (bf->cf);
    mf_close (bf->mf);
    xfree (bf);
    return 0;
}

BFile bf_open (BFiles bfs, const char *name, int block_size, int wflag)
{
    BFile tmp = (BFile) xmalloc(sizeof(BFile_struct));

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
                logf (LOG_FATAL|LOG_ERRNO, "open %s", bfs->cache_fname);
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
    zebra_lock_rdwr_init (&tmp->rdwr_lock);
    return(tmp);
}

int bf_read (BFile bf, int no, int offset, int nbytes, void *buf)
{
    int r;

    zebra_lock_rdwr_rlock (&bf->rdwr_lock);
    if (bf->cf)
    {
	if ((r = cf_read (bf->cf, no, offset, nbytes, buf)) == -1)
	    r = mf_read (bf->mf, no, offset, nbytes, buf);
    }
    else 
	r = mf_read (bf->mf, no, offset, nbytes, buf);
    zebra_lock_rdwr_runlock (&bf->rdwr_lock);
    return r;
}

int bf_write (BFile bf, int no, int offset, int nbytes, const void *buf)
{
    int r;
    zebra_lock_rdwr_wlock (&bf->rdwr_lock);
    if (bf->cf)
        r = cf_write (bf->cf, no, offset, nbytes, buf);
    else
	r = mf_write (bf->mf, no, offset, nbytes, buf);
    zebra_lock_rdwr_wunlock (&bf->rdwr_lock);
    return r;
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

void bf_reset (BFiles bfs)
{
    if (!bfs)
	return;
    mf_reset (bfs->commit_area);
    mf_reset (bfs->register_area);
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
