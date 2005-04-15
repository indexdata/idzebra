/* $Id: bfile.c,v 1.41 2005-04-15 10:47:47 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
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

#include <yaz/xmalloc.h>
#include <idzebra/util.h>
#include <idzebra/bfile.h>
#include "mfile.h"
#include "cfile.h"

struct BFile_struct
{
    MFile mf;
    Zebra_lock_rdwr rdwr_lock;
    struct CFile_struct *cf;
    char *alloc_buf;
    int block_size;
    int alloc_buf_size;
    zint last_block;
    zint free_list;
    zint root_block;
    char *magic;
    int header_dirty;
};

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
        yaz_log (YLOG_LOG, "enabling cache spec=%s", spec);
        if (!bfs->commit_area)
	    bfs->commit_area = mf_init ("shadow", spec, bfs->base);
        if (bfs->commit_area)
        {
            bfs->cache_fname = xmalloc (strlen(bfs->commit_area->dirs->name)+
                                       8);
            strcpy (bfs->cache_fname, bfs->commit_area->dirs->name);
            strcat (bfs->cache_fname, "/cache");
            yaz_log (YLOG_LOG, "cache_fname = %s", bfs->cache_fname);
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
    xfree(bf->alloc_buf);
    xfree(bf->magic);
    xfree(bf);
    return 0;
}

#define HEADER_SIZE 256

BFile bf_xopen(BFiles bfs, const char *name, int block_size, int wrflag,
	       const char *magic, int *read_version,
	       const char **more_info)
{
    char read_magic[40];
    int l = 0;
    int i = 0;
    char *hbuf;
    zint pos = 0;
    BFile bf = bf_open(bfs, name, block_size, wrflag);

    if (!bf)
	return 0;
     /* HEADER_SIZE is considered enough for our header */
    if (bf->alloc_buf_size < HEADER_SIZE)
	bf->alloc_buf_size = HEADER_SIZE;
    else
	bf->alloc_buf_size = bf->block_size;
	
    hbuf = bf->alloc_buf = xmalloc(bf->alloc_buf_size);

    /* fill-in default values */
    bf->free_list = 0;
    bf->root_block = bf->last_block = HEADER_SIZE / bf->block_size + 1;
    bf->magic = xstrdup(magic);

    if (!bf_read(bf, pos, 0, 0, hbuf + pos * bf->block_size))
    {
	if (wrflag)
	    bf->header_dirty = 1;
	return bf;
    }
    while(hbuf[pos * bf->block_size + i] != '\0')
    {
	if (i == bf->block_size)
	{   /* end of block at pos reached .. */
	    if (bf->alloc_buf_size  < (pos+1) * bf->block_size)
	    {
		/* not room for all in alloc_buf_size */
		yaz_log(YLOG_WARN, "bad header for %s (3)", magic);
		bf_close(bf);
		return 0;
	    }
	    /* read next block in header */
	    pos++;
	    if (!bf_read(bf, pos, 0, 0, hbuf + pos * bf->block_size))
	    {
		yaz_log(YLOG_WARN, "missing header block %s (4)", magic);
		bf_close(bf);
		return 0;
	    }
	    i = 0; /* pos within block is reset */
	}
	else
	    i++;
    }
    if (sscanf(hbuf, "%39s %d " ZINT_FORMAT " " ZINT_FORMAT "%n",
	       read_magic, read_version, &bf->last_block,
	       &bf->free_list, &l) < 4 && l)  /* if %n is counted, it's 5 */
    {
	yaz_log(YLOG_WARN, "bad header for %s (1)", magic);
	bf_close(bf);
	return 0;
    }
    if (strcmp(read_magic, magic))
    {
	yaz_log(YLOG_WARN, "bad header for %s (2)", magic);
	bf_close(bf);
	return 0;
    }
    if (more_info)
	*more_info = hbuf + l;
    return bf;
}

int bf_xclose (BFile bf, int version, const char *more_info)
{
    if (bf->header_dirty)
    {
	assert(bf->alloc_buf);
	zint pos = 0;
	assert(bf->magic);
	sprintf(bf->alloc_buf, "%s %d " ZINT_FORMAT " " ZINT_FORMAT " ", 
		bf->magic, version, bf->last_block, bf->free_list);
	if (more_info)
	    strcat(bf->alloc_buf, more_info);
	while (1)
	{
	    bf_write(bf, pos, 0, 0, bf->alloc_buf + pos * bf->block_size);
	    pos++;
	    if (pos * bf->block_size > strlen(bf->alloc_buf))
		break;
	}
    }
    return bf_close(bf);
}

BFile bf_open (BFiles bfs, const char *name, int block_size, int wflag)
{
    BFile bf = (BFile) xmalloc(sizeof(struct BFile_struct));

    bf->alloc_buf = 0;
    bf->magic = 0;
    bf->block_size = block_size;
    bf->header_dirty = 0;
    if (bfs->commit_area)
    {
        int first_time;

        bf->mf = mf_open (bfs->register_area, name, block_size, 0);
        bf->cf = cf_open (bf->mf, bfs->commit_area, name, block_size,
                           wflag, &first_time);
        if (first_time)
        {
            FILE *outf;

            outf = open_cache(bfs, "ab");
            if (!outf)
            {
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "open %s", bfs->cache_fname);
                exit(1);
            }
            fprintf(outf, "%s %d\n", name, block_size);
            fclose(outf);
        }
    }
    else
    {
        bf->mf = mf_open(bfs->register_area, name, block_size, wflag);
        bf->cf = NULL;
    }
    if (!bf->mf)
    {
        yaz_log(YLOG_FATAL, "mf_open failed for %s", name);
        xfree(bf);
        return 0;
    }
    zebra_lock_rdwr_init(&bf->rdwr_lock);
    return bf;
}

int bf_read (BFile bf, zint no, int offset, int nbytes, void *buf)
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

int bf_write (BFile bf, zint no, int offset, int nbytes, const void *buf)
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
        yaz_log (YLOG_LOG, "No commit file");
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

int bf_alloc(BFile bf, int no, zint *blocks)
{
    int i;
    assert(bf->alloc_buf);
    bf->header_dirty = 1;
    for (i = 0; i < no; i++)
    {
	if (!bf->free_list)
	    blocks[i] = bf->last_block++;
	else
	{
	    char buf[16];
	    const char *cp = buf;
	    memset(buf, '\0', sizeof(buf));

	    blocks[i] = bf->free_list;
	    if (!bf_read(bf, bf->free_list, 0, sizeof(buf), buf))
	    {
		yaz_log(YLOG_WARN, "Bad freelist entry " ZINT_FORMAT,
			bf->free_list);
		exit(1);
	    }
	    zebra_zint_decode(&cp, &bf->free_list);
	}
    }
    return 0;
}

int bf_free(BFile bf, int no, const zint *blocks)
{
    int i;
    assert(bf->alloc_buf);
    bf->header_dirty = 1;
    for (i = 0; i < no; i++)
    {
	char buf[16];
	char *cp = buf;
	memset(buf, '\0', sizeof(buf));
	zebra_zint_encode(&cp, bf->free_list);
	bf->free_list = blocks[i];
	bf_write(bf, bf->free_list, 0, sizeof(buf), buf);
    }
    return 0;
}
