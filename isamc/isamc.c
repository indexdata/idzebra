/* $Id: isamc.c,v 1.26 2004-08-06 12:28:23 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

/* 
 * TODO:
 *   Reduction to lower categories in isc_merge
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <yaz/log.h>
#include "isamc-p.h"

static void flush_block (ISAMC is, int cat);
static void release_fc (ISAMC is, int cat);
static void init_fc (ISAMC is, int cat);

#define ISAMC_FREELIST_CHUNK 0

#define SMALL_TEST 0

void isc_getmethod (ISAMC_M *m)
{

    static struct ISAMC_filecat_s def_cat[] = {
#if SMALL_TEST
        {    32,     28,      0,  3 },
	{    64,     54,     30,  0 },
#else
        {    64,     56,     40,  5 },
	{   128,    120,    100,  10 },
        {   512,    490,    350,  10 },
        {  2048,   1900,   1700,  10 },
        {  8192,   8000,   7900,  10 },
        { 32768,  32000,  31000,  0 },
#endif
    };
    m->filecat = def_cat;

    m->codec.start = NULL;
    m->codec.decode  = NULL;
    m->codec.encode = NULL;
    m->codec.stop = NULL;
    m->codec.reset = NULL;

    m->compare_item = NULL;
    m->log_item = NULL;

    m->debug = 1;

    m->max_blocks_mem = 10;
}

ISAMC isc_open (BFiles bfs, const char *name, int writeflag, ISAMC_M *method)
{
    ISAMC is;
    ISAMC_filecat filecat;
    int i = 0;
    int max_buf_size = 0;

    is = (ISAMC) xmalloc (sizeof(*is));

    is->method = (ISAMC_M *) xmalloc (sizeof(*is->method));
    memcpy (is->method, method, sizeof(*method));
    filecat = is->method->filecat;
    assert (filecat);

    /* determine number of block categories */
    if (is->method->debug)
        logf (LOG_LOG, "isc: bsize  ifill  mfill mblocks");
    do
    {
        if (is->method->debug)
            logf (LOG_LOG, "isc:%6d %6d %6d %6d",
                  filecat[i].bsize, filecat[i].ifill, 
                  filecat[i].mfill, filecat[i].mblocks);
        if (max_buf_size < filecat[i].mblocks * filecat[i].bsize)
            max_buf_size = filecat[i].mblocks * filecat[i].bsize;
    } while (filecat[i++].mblocks);
    is->no_files = i;
    is->max_cat = --i;
    /* max_buf_size is the larget buffer to be used during merge */
    max_buf_size = (1 + max_buf_size / filecat[i].bsize) * filecat[i].bsize;
    if (max_buf_size < (1+is->method->max_blocks_mem) * filecat[i].bsize)
        max_buf_size = (1+is->method->max_blocks_mem) * filecat[i].bsize;
    if (is->method->debug)
        logf (LOG_LOG, "isc: max_buf_size %d", max_buf_size);
    
    assert (is->no_files > 0);
    is->files = (ISAMC_file) xmalloc (sizeof(*is->files)*is->no_files);
    if (writeflag)
    {
        is->merge_buf = (char *) xmalloc (max_buf_size+256);
	memset (is->merge_buf, 0, max_buf_size+256);
    }
    else
        is->merge_buf = NULL;
    for (i = 0; i<is->no_files; i++)
    {
        char fname[512];

        sprintf (fname, "%s%c", name, i+'A');
        is->files[i].bf = bf_open (bfs, fname, is->method->filecat[i].bsize,
                                   writeflag);
        is->files[i].head_is_dirty = 0;
        if (!bf_read (is->files[i].bf, 0, 0, sizeof(ISAMC_head),
                     &is->files[i].head))
        {
            is->files[i].head.lastblock = 1;
            is->files[i].head.freelist = 0;
        }
	is->files[i].alloc_entries_num = 0;
	is->files[i].alloc_entries_max =
	    is->method->filecat[i].bsize / sizeof(zint) - 1;
	is->files[i].alloc_buf = (char *)
	    xmalloc (is->method->filecat[i].bsize);
        is->files[i].no_writes = 0;
        is->files[i].no_reads = 0;
        is->files[i].no_skip_writes = 0;
        is->files[i].no_allocated = 0;
        is->files[i].no_released = 0;
        is->files[i].no_remap = 0;
	is->files[i].no_forward = 0;
	is->files[i].no_backward = 0;
	is->files[i].sum_forward = 0;
	is->files[i].sum_backward = 0;
	is->files[i].no_next = 0;
	is->files[i].no_prev = 0;

        init_fc (is, i);
    }
    return is;
}

zint isc_block_used (ISAMC is, int type)
{
    if (type < 0 || type >= is->no_files)
	return -1;
    return is->files[type].head.lastblock-1;
}

int isc_block_size (ISAMC is, int type)
{
    ISAMC_filecat filecat = is->method->filecat;
    if (type < 0 || type >= is->no_files)
	return -1;
    return filecat[type].bsize;
}

int isc_close (ISAMC is)
{
    int i;

    if (is->method->debug)
    {
	logf (LOG_LOG, "isc:    next    forw   mid-f    prev   backw   mid-b");
	for (i = 0; i<is->no_files; i++)
	    logf (LOG_LOG, "isc:%8d%8d%8.1f%8d%8d%8.1f",
		  is->files[i].no_next,
		  is->files[i].no_forward,
		  is->files[i].no_forward ?
		  (double) is->files[i].sum_forward/is->files[i].no_forward
		  : 0.0,
		  is->files[i].no_prev,
		  is->files[i].no_backward,
		  is->files[i].no_backward ?
		  (double) is->files[i].sum_backward/is->files[i].no_backward
		  : 0.0);
    }
    if (is->method->debug)
        logf (LOG_LOG, "isc:  writes   reads skipped   alloc released  remap");
    for (i = 0; i<is->no_files; i++)
    {
        release_fc (is, i);
        assert (is->files[i].bf);
        if (is->files[i].head_is_dirty)
            bf_write (is->files[i].bf, 0, 0, sizeof(ISAMC_head),
                 &is->files[i].head);
        if (is->method->debug)
            logf (LOG_LOG, "isc:%8d%8d%8d%8d%8d%8d",
                  is->files[i].no_writes,
                  is->files[i].no_reads,
                  is->files[i].no_skip_writes,
                  is->files[i].no_allocated,
                  is->files[i].no_released,
                  is->files[i].no_remap);
        xfree (is->files[i].fc_list);
	flush_block (is, i);
        bf_close (is->files[i].bf);
    }
    xfree (is->files);
    xfree (is->merge_buf);
    xfree (is->method);
    xfree (is);
    return 0;
}

int isc_read_block (ISAMC is, int cat, zint pos, char *dst)
{
    ++(is->files[cat].no_reads);
    return bf_read (is->files[cat].bf, pos, 0, 0, dst);
}

int isc_write_block (ISAMC is, int cat, zint pos, char *src)
{
    ++(is->files[cat].no_writes);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_block %d " ZINT_FORMAT, cat, pos);
    return bf_write (is->files[cat].bf, pos, 0, 0, src);
}

int isc_write_dblock (ISAMC is, int cat, zint pos, char *src,
                      zint nextpos, int offset)
{
    ISAMC_BLOCK_SIZE size = offset + ISAMC_BLOCK_OFFSET_N;
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_dblock. size=%d nextpos=" ZINT_FORMAT,
              (int) size, nextpos);
    src -= ISAMC_BLOCK_OFFSET_N;
    memcpy (src, &nextpos, sizeof(nextpos));
    memcpy (src + sizeof(nextpos), &size, sizeof(size));
    return isc_write_block (is, cat, pos, src);
}

#if ISAMC_FREELIST_CHUNK
static void flush_block (ISAMC is, int cat)
{
    char *abuf = is->files[cat].alloc_buf;
    zint block = is->files[cat].head.freelist;
    if (block && is->files[cat].alloc_entries_num)
    {
	memcpy (abuf, &is->files[cat].alloc_entries_num, sizeof(block));
	bf_write (is->files[cat].bf, block, 0, 0, abuf);
	is->files[cat].alloc_entries_num = 0;
    }
    xfree (abuf);
}

static zint alloc_block (ISAMC is, int cat)
{
    zint block = is->files[cat].head.freelist;
    char *abuf = is->files[cat].alloc_buf;

    (is->files[cat].no_allocated)++;

    if (!block)
    {
        block = (is->files[cat].head.lastblock)++;   /* no free list */
	is->files[cat].head_is_dirty = 1;
    }
    else
    {
	if (!is->files[cat].alloc_entries_num) /* read first time */
	{
	    bf_read (is->files[cat].bf, block, 0, 0, abuf);
	    memcpy (&is->files[cat].alloc_entries_num, abuf,
		    sizeof(is->files[cat].alloc_entries_num));
	    assert (is->files[cat].alloc_entries_num > 0);
	}
	/* have some free blocks now */
	assert (is->files[cat].alloc_entries_num > 0);
	is->files[cat].alloc_entries_num--;
	if (!is->files[cat].alloc_entries_num)  /* last one in block? */
	{
	    memcpy (&is->files[cat].head.freelist, abuf + sizeof(int),
		    sizeof(zint));
	    is->files[cat].head_is_dirty = 1;

	    if (is->files[cat].head.freelist)
	    {
		bf_read (is->files[cat].bf, is->files[cat].head.freelist,
			 0, 0, abuf);
		memcpy (&is->files[cat].alloc_entries_num, abuf,
			sizeof(is->files[cat].alloc_entries_num));
		assert (is->files[cat].alloc_entries_num);
	    }
	}
	else
	    memcpy (&block, abuf + sizeof(zint) + sizeof(int) *
		    is->files[cat].alloc_entries_num, sizeof(zint));
    }
    return block;
}

static void release_block (ISAMC is, int cat, zint pos)
{
    char *abuf = is->files[cat].alloc_buf;
    zint block = is->files[cat].head.freelist;

    (is->files[cat].no_released)++;

    if (block && !is->files[cat].alloc_entries_num) /* must read block */
    {
	bf_read (is->files[cat].bf, block, 0, 0, abuf);
	memcpy (&is->files[cat].alloc_entries_num, abuf,
		sizeof(is->files[cat].alloc_entries_num));
	assert (is->files[cat].alloc_entries_num > 0);
    }
    assert (is->files[cat].alloc_entries_num <= is->files[cat].alloc_entries_max);
    if (is->files[cat].alloc_entries_num == is->files[cat].alloc_entries_max)
    {
	assert (block);
	memcpy (abuf, &is->files[cat].alloc_entries_num, sizeof(int));
	bf_write (is->files[cat].bf, block, 0, 0, abuf);
	is->files[cat].alloc_entries_num = 0;
    }
    if (!is->files[cat].alloc_entries_num) /* make new buffer? */
    {
	memcpy (abuf + sizeof(int), &block, sizeof(zint));
	is->files[cat].head.freelist = pos;
	is->files[cat].head_is_dirty = 1; 
    }
    else
    {
	memcpy (abuf + sizeof(int) +
		is->files[cat].alloc_entries_num*sizeof(zint),
		&pos, sizeof(zint));
    }
    is->files[cat].alloc_entries_num++;
}
#else
static void flush_block (ISAMC is, int cat)
{
    char *abuf = is->files[cat].alloc_buf;
    xfree (abuf);
}

static zint alloc_block (ISAMC is, int cat)
{
    zint block;
    char buf[sizeof(zint)];

    is->files[cat].head_is_dirty = 1;
    (is->files[cat].no_allocated)++;
    if ((block = is->files[cat].head.freelist))
    {
        bf_read (is->files[cat].bf, block, 0, sizeof(zint), buf);
        memcpy (&is->files[cat].head.freelist, buf, sizeof(zint));
    }
    else
        block = (is->files[cat].head.lastblock)++;
    return block;
}

static void release_block (ISAMC is, int cat, zint pos)
{
    char buf[sizeof(zint)];
   
    (is->files[cat].no_released)++;
    is->files[cat].head_is_dirty = 1; 
    memcpy (buf, &is->files[cat].head.freelist, sizeof(zint));
    is->files[cat].head.freelist = pos;
    bf_write (is->files[cat].bf, pos, 0, sizeof(zint), buf);
}
#endif

zint isc_alloc_block (ISAMC is, int cat)
{
    zint block = 0;

    if (is->files[cat].fc_list)
    {
        int j;
	zint nb;
        for (j = 0; j < is->files[cat].fc_max; j++)
            if ((nb = is->files[cat].fc_list[j]) && (!block || nb < block))
            {
                is->files[cat].fc_list[j] = 0;
		block = nb;
                break;
            }
    }
    if (!block)
        block = alloc_block (is, cat);
    if (is->method->debug > 3)
        logf (LOG_LOG, "isc: alloc_block in cat %d: " ZINT_FORMAT, cat, block);
    return block;
}

void isc_release_block (ISAMC is, int cat, zint pos)
{
    if (is->method->debug > 3)
        logf (LOG_LOG, "isc: release_block in cat %d:" ZINT_FORMAT, cat, pos);
    if (is->files[cat].fc_list)
    {
        int j;
        for (j = 0; j<is->files[cat].fc_max; j++)
            if (!is->files[cat].fc_list[j])
            {
                is->files[cat].fc_list[j] = pos;
                return;
            }
    }
    release_block (is, cat, pos);
}

static void init_fc (ISAMC is, int cat)
{
    int j = 100;
        
    is->files[cat].fc_max = j;
    is->files[cat].fc_list = (zint *)
	xmalloc (sizeof(*is->files[0].fc_list) * j);
    while (--j >= 0)
        is->files[cat].fc_list[j] = 0;
}

static void release_fc (ISAMC is, int cat)
{
    int j = is->files[cat].fc_max;
    zint b;

    while (--j >= 0)
        if ((b = is->files[cat].fc_list[j]))
        {
            release_block (is, cat, b);
            is->files[cat].fc_list[j] = 0;
        }
}

void isc_pp_close (ISAMC_PP pp)
{
    ISAMC is = pp->is;

    (*is->method->codec.stop)(pp->decodeClientData);
    xfree (pp->buf);
    xfree (pp);
}

ISAMC_PP isc_pp_open (ISAMC is, ISAMC_P ipos)
{
    ISAMC_PP pp = (ISAMC_PP) xmalloc (sizeof(*pp));
    char *src;
   
    pp->cat = (int) isc_type(ipos);
    pp->pos = isc_block(ipos); 

    src = pp->buf = (char *) xmalloc (is->method->filecat[pp->cat].bsize);

    pp->next = 0;
    pp->size = 0;
    pp->offset = 0;
    pp->is = is;
    pp->decodeClientData = (*is->method->codec.start)();
    pp->deleteFlag = 0;
    pp->numKeys = 0;

    if (pp->pos)
    {
        src = pp->buf;
        isc_read_block (is, pp->cat, pp->pos, src);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        memcpy (&pp->numKeys, src, sizeof(pp->numKeys));
        src += sizeof(pp->numKeys);
	if (pp->next == pp->pos)
	{
	    yaz_log(LOG_FATAL|LOG_LOG, "pp->next = " ZINT_FORMAT, pp->next);
	    yaz_log(LOG_FATAL|LOG_LOG, "pp->pos = " ZINT_FORMAT, pp->pos);
	    assert (pp->next != pp->pos);
	}
        pp->offset = src - pp->buf; 
        assert (pp->offset == ISAMC_BLOCK_OFFSET_1);
        if (is->method->debug > 2)
            logf (LOG_LOG, "isc: read_block size=%d %d " ZINT_FORMAT " next="
		  ZINT_FORMAT, pp->size, pp->cat, pp->pos, pp->next);
    }
    return pp;
}

/* returns non-zero if item could be read; 0 otherwise */
int isc_pp_read (ISAMC_PP pp, void *buf)
{
    char *cp = buf;
    return isc_read_item (pp, &cp);
}

/* read one item from file - decode and store it in *dst.
   Returns
     0 if end-of-file
     1 if item could be read ok and NO boundary
     2 if item could be read ok and boundary */
int isc_read_item (ISAMC_PP pp, char **dst)
{
    ISAMC is = pp->is;
    const char *src = pp->buf + pp->offset;

    if (pp->offset >= pp->size)
    {
	if (!pp->next)
	{
	    pp->pos = 0;
	    return 0; /* end of file */
	}
	if (pp->next > pp->pos)
	{
	    if (pp->next == pp->pos + 1)
		is->files[pp->cat].no_next++;
	    else
	    {
		is->files[pp->cat].no_forward++;
		is->files[pp->cat].sum_forward += pp->next - pp->pos;
	    }
	}
	else
	{
	    if (pp->next + 1 == pp->pos)
		is->files[pp->cat].no_prev++;
	    else
	    {
		is->files[pp->cat].no_backward++;
		is->files[pp->cat].sum_backward += pp->pos - pp->next;
	    }
	}
	/* out new block position */
        pp->pos = pp->next;
        src = pp->buf;
	/* read block and save 'next' and 'size' entry */
        isc_read_block (is, pp->cat, pp->pos, pp->buf);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        /* assume block is non-empty */
        assert (src - pp->buf == ISAMC_BLOCK_OFFSET_N);

	if (pp->next == pp->pos)
	{
	    yaz_log(LOG_FATAL|LOG_LOG, "pp->next = " ZINT_FORMAT, pp->next);
	    yaz_log(LOG_FATAL|LOG_LOG, "pp->pos = " ZINT_FORMAT, pp->pos);
	    assert (pp->next != pp->pos);
	}

        if (pp->deleteFlag)
            isc_release_block (is, pp->cat, pp->pos);
        (*is->method->codec.decode)(pp->decodeClientData, dst, &src);
        pp->offset = src - pp->buf; 
        if (is->method->debug > 2)
            logf (LOG_LOG, "isc: read_block size=%d %d " ZINT_FORMAT " next="
		  ZINT_FORMAT, pp->size, pp->cat, pp->pos, pp->next);
        return 2;
    }
    (*is->method->codec.decode)(pp->decodeClientData, dst, &src);
    pp->offset = src - pp->buf; 
    return 1;
}

zint isc_pp_num (ISAMC_PP pp)
{
    return pp->numKeys;
}

