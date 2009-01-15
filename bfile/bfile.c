/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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
#endif

#if HAVE_UNISTD_H
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
    MFile_area register_area;
    char *base;
    char *cache_fname;
};

BFiles bfs_create (const char *spec, const char *base)
{
    BFiles bfs = (BFiles) xmalloc(sizeof(*bfs));
    bfs->commit_area = 0;
    bfs->base = 0;
    bfs->cache_fname = 0;
    if (base)
        bfs->base = xstrdup(base);
    bfs->register_area = mf_init("register", spec, base, 0);
    if (!bfs->register_area)
    {
        bfs_destroy(bfs);
        return 0;
    }
    return bfs;
}

void bfs_destroy(BFiles bfs)
{
    if (!bfs)
        return;
    xfree(bfs->cache_fname);
    xfree(bfs->base);
    mf_destroy(bfs->commit_area);
    mf_destroy(bfs->register_area);
    xfree(bfs);
}

static FILE *open_cache(BFiles bfs, const char *flags)
{
    FILE *file;

    file = fopen(bfs->cache_fname, flags);
    return file;
}

static void unlink_cache(BFiles bfs)
{
    if (bfs->cache_fname)
        unlink(bfs->cache_fname);
}

ZEBRA_RES bf_cache(BFiles bfs, const char *spec)
{
    if (spec)
    {
        yaz_log(YLOG_LOG, "enabling shadow spec=%s", spec);
        if (!bfs->commit_area)
	    bfs->commit_area = mf_init("shadow", spec, bfs->base, 1);
        if (bfs->commit_area)
        {
            bfs->cache_fname = xmalloc(strlen(bfs->commit_area->dirs->name)+
                                       8);
            strcpy(bfs->cache_fname, bfs->commit_area->dirs->name);
            strcat(bfs->cache_fname, "/cache");
            yaz_log(YLOG_LOG, "cache_fname = %s", bfs->cache_fname);
        }
	else
	{
	    yaz_log(YLOG_WARN, "shadow could not be enabled");
	    return ZEBRA_FAIL;
	}
    }
    else
        bfs->commit_area = 0;
    return ZEBRA_OK;
}

int bf_close2(BFile bf)
{
    int ret = 0;
    zebra_lock_rdwr_destroy(&bf->rdwr_lock);
    if (bf->cf)
    {
        if (cf_close(bf->cf))
            ret = -1;
    }
    if (bf->mf)
    {
        if (mf_close(bf->mf))
            ret = -1;
    }
    xfree(bf->alloc_buf);
    xfree(bf->magic);
    xfree(bf);
    return ret;
}

void bf_close(BFile bf)
{
    if (bf_close2(bf))
    {
        zebra_exit("bf_close");
    }
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
    if (bf->block_size < HEADER_SIZE)
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
    if (hbuf[l] == ' ')
	l++;
    if (more_info)
	*more_info = hbuf + l;
    return bf;
}

int bf_xclose(BFile bf, int version, const char *more_info)
{
    if (bf->header_dirty)
    {
	zint pos = 0;
	assert(bf->alloc_buf);
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
    return bf_close2(bf);
}

BFile bf_open(BFiles bfs, const char *name, int block_size, int wflag)
{
    BFile bf = (BFile) xmalloc(sizeof(*bf));

    bf->alloc_buf = 0;
    bf->magic = 0;
    bf->block_size = block_size;
    bf->header_dirty = 0;
    bf->cf = 0;
    bf->mf = 0;
    zebra_lock_rdwr_init(&bf->rdwr_lock);

    if (bfs->commit_area)
    {
        int first_time;

        bf->mf = mf_open(bfs->register_area, name, block_size, 0);
        bf->cf = cf_open(bf->mf, bfs->commit_area, name, block_size,
                         wflag, &first_time);
        if (!bf->cf)
        {
            yaz_log(YLOG_FATAL, "cf_open failed for %s", name);
            bf_close(bf);
            return 0;
        }
        if (first_time)
        {
            FILE *outf;

            outf = open_cache(bfs, "ab");
            if (!outf)
            {
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "open %s", bfs->cache_fname);
                bf_close(bf);
                return 0;
            }
            fprintf(outf, "%s %d\n", name, block_size);
            if (fclose(outf))
            {
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "fclose %s", bfs->cache_fname);
                bf_close(bf);
                return 0;
            }
        }
    }
    else
    {
        bf->mf = mf_open(bfs->register_area, name, block_size, wflag);
    }
    if (!bf->mf)
    {
        yaz_log(YLOG_FATAL, "mf_open failed for %s", name);
        bf_close(bf);
        return 0;
    }
    return bf;
}

int bf_read(BFile bf, zint no, int offset, int nbytes, void *buf)
{
    int ret = bf_read2(bf, no, offset, nbytes, buf);

    if (ret == -1)
    {
        zebra_exit("bf_read");
    }
    return ret;
}

int bf_read2(BFile bf, zint no, int offset, int nbytes, void *buf)
{
    int ret;

    zebra_lock_rdwr_rlock(&bf->rdwr_lock);
    if (bf->cf)
    {
	if ((ret = cf_read(bf->cf, no, offset, nbytes, buf)) == 0)
	    ret = mf_read(bf->mf, no, offset, nbytes, buf);
    }
    else 
	ret = mf_read(bf->mf, no, offset, nbytes, buf);
    zebra_lock_rdwr_runlock(&bf->rdwr_lock);
    return ret;
}

int bf_write(BFile bf, zint no, int offset, int nbytes, const void *buf)
{
    int ret = bf_write2(bf, no, offset, nbytes, buf);

    if (ret == -1)
    {
        zebra_exit("bf_write");
    }
    return ret;
}

int bf_write2(BFile bf, zint no, int offset, int nbytes, const void *buf)
{
    int r;
    zebra_lock_rdwr_wlock(&bf->rdwr_lock);
    if (bf->cf)
        r = cf_write(bf->cf, no, offset, nbytes, buf);
    else
	r = mf_write(bf->mf, no, offset, nbytes, buf);
    zebra_lock_rdwr_wunlock(&bf->rdwr_lock);
    return r;
}

int bf_commitExists(BFiles bfs)
{
    FILE *inf;

    inf = open_cache(bfs, "rb");
    if (inf)
    {
        fclose(inf);
        return 1;
    }
    return 0;
}

void bf_reset(BFiles bfs)
{
    if (!bfs)
	return;
    mf_reset(bfs->commit_area, 1);
    mf_reset(bfs->register_area, 1);
    unlink_cache(bfs);
}

int bf_commitExec(BFiles bfs)
{
    FILE *inf;
    int block_size;
    char path[256];
    MFile mf;
    CFile cf;
    int first_time;
    int r = 0;

    assert(bfs->commit_area);
    if (!(inf = open_cache(bfs, "rb")))
    {
        yaz_log(YLOG_LOG, "No commit file");
        return -1;
    }
    while (fscanf(inf, "%s %d", path, &block_size) == 2)
    {
        mf = mf_open(bfs->register_area, path, block_size, 1);
        if (!mf)
        {
            r = -1;
            break;
        }
        cf = cf_open(mf, bfs->commit_area, path, block_size, 0, &first_time);
        if (!cf)
        {
            mf_close(mf);
            r = -1;
            break;
        }

        r = cf_commit(cf);

        cf_close(cf);
        mf_close(mf);

        if (r)
            break;
    }
    fclose(inf);
    return r;
}

void bf_commitClean(BFiles bfs, const char *spec)
{
    int mustDisable = 0;

    if (!bfs->commit_area)
    {
        bf_cache(bfs, spec);
        mustDisable = 1;
    }

    mf_reset(bfs->commit_area, 1);

    unlink_cache(bfs);
    if (mustDisable)
        bf_cache(bfs, 0);
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
	    if (bf_read(bf, bf->free_list, 0, sizeof(buf), buf) != 1)
	    {
		yaz_log(YLOG_WARN, "Bad freelist entry " ZINT_FORMAT,
			bf->free_list);
                return -1;
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

int bfs_register_directory_stat(BFiles bfs, int no, const char **directory,
				double *used_bytes, double *max_bytes)
{
    return mf_area_directory_stat(bfs->register_area, no, directory,
				  used_bytes, max_bytes);
}


int bfs_shadow_directory_stat(BFiles bfs, int no, const char **directory,
			      double *used_bytes, double *max_bytes)
{
    if (!bfs->commit_area)
	return 0;
    return mf_area_directory_stat(bfs->commit_area, no, directory,
				  used_bytes, max_bytes);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

