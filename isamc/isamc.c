/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isamc.c,v $
 * Revision 1.4  1996-11-01 13:36:46  adam
 * New element, max_blocks_mem, that control how many blocks of max size
 * to store in memory during isc_merge.
 * Function isc_merge now ignoreds delete/update of identical keys and
 * the proper blocks are then non-dirty and not written in flush_blocks.
 *
 * Revision 1.3  1996/11/01  08:59:14  adam
 * First version of isc_merge that supports update/delete.
 *
 * Revision 1.2  1996/10/29 16:44:56  adam
 * Work on isc_merge.
 *
 * Revision 1.1  1996/10/29  13:40:48  adam
 * First work.
 *
 */

/* 
 * TODO:
 *   Reduction to lower categories in isc_merge
 *   Implementation of isc_numkeys
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <log.h>
#include "isamc-p.h"

ISAMC_M isc_getmethod (void)
{
    static struct ISAMC_filecat_s def_cat[] = {
        {   32,    28,     0,    20 },
        {  512,   490,   100,    20 },
        { 4096,  3950,  1000,    20 },
        {32768, 32000, 10000,     0 },
        {    0,     0,     0,     0 }
    };
    ISAMC_M m = xmalloc (sizeof(*m));
    m->filecat = def_cat;

    m->code_start = NULL;
    m->code_item = NULL;
    m->code_stop = NULL;

    m->compare_item = NULL;

    m->debug = 0;

    m->max_blocks_mem = 10;

    return m;
}


ISAMC isc_open (const char *name, int writeflag, ISAMC_M method)
{
    ISAMC is;
    ISAMC_filecat filecat;
    int i;
    int max_buf_size = 0;

    is = xmalloc (sizeof(*is));

    is->method = xmalloc (sizeof(*is->method));
    memcpy (is->method, method, sizeof(*method));
    filecat = is->method->filecat;
    assert (filecat);

    /* determine number of block categories */
    if (is->method->debug)
        logf (LOG_LOG, "isc: bsize  ifill  mfill mblocks");
    for (i = 0; filecat[i].bsize; i++)
    {
        if (is->method->debug)
            logf (LOG_LOG, "isc:%6d %6d %6d %6d",
                  filecat[i].bsize, filecat[i].ifill, 
                  filecat[i].mfill, filecat[i].mblocks);
        if (max_buf_size < filecat[i].mblocks * filecat[i].bsize)
            max_buf_size = filecat[i].mblocks * filecat[i].bsize;
    }
    is->no_files = i;
    is->max_cat = --i;
    /* max_buf_size is the larget buffer to be used during merge */
    max_buf_size = (1 + max_buf_size / filecat[i].bsize) * filecat[i].bsize;
    if (max_buf_size < (1+is->method->max_blocks_mem) * filecat[i].bsize)
        max_buf_size = (1+is->method->max_blocks_mem) * filecat[i].bsize;
    if (is->method->debug)
        logf (LOG_LOG, "isc: max_buf_size %d", max_buf_size);
    
    assert (is->no_files > 0);
    is->files = xmalloc (sizeof(*is->files)*is->no_files);
    if (writeflag)
    {
        is->merge_buf = xmalloc (max_buf_size+128);
	memset (is->merge_buf, 0, max_buf_size+128);
    }
    else
        is->merge_buf = NULL;
    for (i = 0; i<is->no_files; i++)
    {
        char fname[512];

        sprintf (fname, "%s%c", name, i+'A');
        is->files[i].bf = bf_open (fname, is->method->filecat[i].bsize,
                                   writeflag);
        is->files[i].head_is_dirty = 0;
        if (!bf_read (is->files[i].bf, 0, 0, sizeof(ISAMC_head),
                     &is->files[i].head))
        {
            is->files[i].head.lastblock = 1;
            is->files[i].head.freelist = 0;
        }
        is->files[i].no_writes = 0;
        is->files[i].no_reads = 0;
        is->files[i].no_skip_writes = 0;
        is->files[i].no_allocated = 0;
        is->files[i].no_released = 0;
        is->files[i].no_remap = 0;
    }
    return is;
}

int isc_close (ISAMC is)
{
    int i;

    if (is->method->debug)
        logf (LOG_LOG, "isc:  writes   reads skipped   alloc released  remap");
    for (i = 0; i<is->no_files; i++)
    {
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
        bf_close (is->files[i].bf);
    }
    xfree (is->files);
    xfree (is->merge_buf);
    xfree (is);
    return 0;
}

int isc_read_block (ISAMC is, int cat, int pos, char *dst)
{
    ++(is->files[cat].no_reads);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: read_block %d %d", cat, pos);
    return bf_read (is->files[cat].bf, pos, 0, 0, dst);
}

int isc_write_block (ISAMC is, int cat, int pos, char *src)
{
    ++(is->files[cat].no_writes);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_block %d %d", cat, pos);
    return bf_write (is->files[cat].bf, pos, 0, 0, src);
}

int isc_write_dblock (ISAMC is, int cat, int pos, char *src,
                      int nextpos, int offset)
{
    int xoffset = offset + 2*sizeof(int);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_dblock. size=%d nextpos=%d",
              offset, nextpos);
    memcpy (src - sizeof(int)*2, &nextpos, sizeof(int));
    memcpy (src - sizeof(int), &xoffset, sizeof(int));
    return isc_write_block (is, cat, pos, src - sizeof(int)*2);
}

int isc_alloc_block (ISAMC is, int cat)
{
    int block;
    char buf[sizeof(int)];

    is->files[cat].head_is_dirty = 1;
    (is->files[cat].no_allocated)++;
    if ((block = is->files[cat].head.freelist))
    {
        bf_read (is->files[cat].bf, block, 0, sizeof(int), buf);
        memcpy (&is->files[cat].head.freelist, buf, sizeof(int));
    }
    else
        block = (is->files[cat].head.lastblock)++;
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: alloc_block in cat %d: %d", cat, block);
    return block;
}

void isc_release_block (ISAMC is, int cat, int pos)
{
    char buf[sizeof(int)];
   
    (is->files[cat].no_released)++;
    is->files[cat].head_is_dirty = 1; 
    memcpy (buf, &is->files[cat].head.freelist, sizeof(int));
    is->files[cat].head.freelist = pos;
    bf_write (is->files[cat].bf, pos, 0, sizeof(int), buf);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: release_block in cat %d: %d", cat, pos);
}

void isc_pp_close (ISAMC_PP pp)
{
    ISAMC is = pp->is;

    (*is->method->code_stop)(ISAMC_DECODE, pp->decodeClientData);
    xfree (pp->buf);
    xfree (pp);
}

ISAMC_PP isc_pp_open (ISAMC is, ISAMC_P ipos)
{
    ISAMC_PP pp = xmalloc (sizeof(*pp));
    char *src;
   
    pp->cat = isc_type(ipos);
    pp->next = isc_block(ipos); 

    src = pp->buf = xmalloc (is->method->filecat[pp->cat].bsize);

    pp->pos = 0;    
    pp->size = 0;
    pp->offset = 0;
    pp->is = is;
    pp->decodeClientData = (*is->method->code_start)(ISAMC_DECODE);
    pp->deleteFlag = 0;
    return pp;
}

/* returns non-zero if item could be read; 0 otherwise */
int isc_read_key (ISAMC_PP pp, void *buf)
{
    return isc_read_item (pp, (char **) &buf);
}

/* returns non-zero if item could be read; 0 otherwise */
int isc_read_item (ISAMC_PP pp, char **dst)
{
    ISAMC is = pp->is;
    char *src = pp->buf + pp->offset;

    if (pp->offset >= pp->size)
    {
        pp->pos = pp->next;
        if (!pp->pos)
            return 0;
        src = pp->buf;
        isc_read_block (is, pp->cat, pp->pos, src);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        /* assume block is non-empty */
        assert (pp->next != pp->pos);
        if (pp->deleteFlag)
            isc_release_block (is, pp->cat, pp->pos);
        (*is->method->code_item)(ISAMC_DECODE, pp->decodeClientData, dst, &src);
        pp->offset = src - pp->buf; 
        return 2;
    }
    (*is->method->code_item)(ISAMC_DECODE, pp->decodeClientData, dst, &src);
    pp->offset = src - pp->buf; 
    return 1;
}

int isc_numkeys (ISAMC_PP pp)
{
    return 1;
}

