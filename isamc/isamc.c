/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isamc.c,v $
 * Revision 1.7  1997-02-12 20:42:43  adam
 * Bug fix: during isc_merge operations, some pages weren't marked dirty
 * even though they should be. At this point the merge operation marks
 * a page dirty if the previous page changed at all. A better approach is
 * to mark it dirty if the last key written changed in previous page.
 *
 * Revision 1.6  1996/11/08 11:15:29  adam
 * Number of keys in chain are stored in first block and the function
 * to retrieve this information, isc_pp_num is implemented.
 *
 * Revision 1.5  1996/11/04 14:08:57  adam
 * Optimized free block usage.
 *
 * Revision 1.4  1996/11/01 13:36:46  adam
 * New element, max_blocks_mem, that control how many blocks of max size
 * to store in memory during isc_merge.
 * Function isc_merge now ignores delete/update of identical keys and
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
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <log.h>
#include "isamc-p.h"

static void release_fc (ISAMC is, int cat);
static void init_fc (ISAMC is, int cat);

ISAMC_M isc_getmethod (void)
{
    static struct ISAMC_filecat_s def_cat[] = {
        {   32,    28,     0,    20 },
        {  512,   490,   100,    20 },
        { 4096,  3950,  1000,    20 },
        {32768, 32000, 10000,     0 },
    };
    ISAMC_M m = xmalloc (sizeof(*m));
    m->filecat = def_cat;

    m->code_start = NULL;
    m->code_item = NULL;
    m->code_stop = NULL;

    m->compare_item = NULL;

    m->debug = 1;

    m->max_blocks_mem = 10;

    return m;
}


ISAMC isc_open (const char *name, int writeflag, ISAMC_M method)
{
    ISAMC is;
    ISAMC_filecat filecat;
    int i = 0;
    int max_buf_size = 0;

    is = xmalloc (sizeof(*is));

    is->method = xmalloc (sizeof(*is->method));
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
    is->files = xmalloc (sizeof(*is->files)*is->no_files);
    if (writeflag)
    {
        is->merge_buf = xmalloc (max_buf_size+256);
	memset (is->merge_buf, 0, max_buf_size+256);
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

        init_fc (is, i);
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
    unsigned short size = offset + ISAMC_BLOCK_OFFSET_N;
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_dblock. size=%d nextpos=%d",
              (int) size, nextpos);
    src -= ISAMC_BLOCK_OFFSET_N;
    memcpy (src, &nextpos, sizeof(int));
    memcpy (src + sizeof(int), &size, sizeof(size));
    return isc_write_block (is, cat, pos, src);
}

static int alloc_block (ISAMC is, int cat)
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
    return block;
}

int isc_alloc_block (ISAMC is, int cat)
{
    int block = 0;

    if (is->files[cat].fc_list)
    {
        int j, nb;
        for (j = 0; j < is->files[cat].fc_max; j++)
            if ((nb = is->files[cat].fc_list[j]) && (!block || nb < block))
            {
                is->files[cat].fc_list[j] = 0;
                break;
            }
    }
    if (!block)
        block = alloc_block (is, cat);
    if (is->method->debug > 3)
        logf (LOG_LOG, "isc: alloc_block in cat %d: %d", cat, block);
    return block;
}

static void release_block (ISAMC is, int cat, int pos)
{
    char buf[sizeof(int)];
   
    (is->files[cat].no_released)++;
    is->files[cat].head_is_dirty = 1; 
    memcpy (buf, &is->files[cat].head.freelist, sizeof(int));
    is->files[cat].head.freelist = pos;
    bf_write (is->files[cat].bf, pos, 0, sizeof(int), buf);
}

void isc_release_block (ISAMC is, int cat, int pos)
{
    if (is->method->debug > 3)
        logf (LOG_LOG, "isc: release_block in cat %d: %d", cat, pos);
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
    is->files[cat].fc_list = xmalloc (sizeof(*is->files[0].fc_list) * j);
    while (--j >= 0)
        is->files[cat].fc_list[j] = 0;
}

static void release_fc (ISAMC is, int cat)
{
    int b, j = is->files[cat].fc_max;

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

    (*is->method->code_stop)(ISAMC_DECODE, pp->decodeClientData);
    xfree (pp->buf);
    xfree (pp);
}

ISAMC_PP isc_pp_open (ISAMC is, ISAMC_P ipos)
{
    ISAMC_PP pp = xmalloc (sizeof(*pp));
    char *src;
   
    pp->cat = isc_type(ipos);
    pp->pos = isc_block(ipos); 

    src = pp->buf = xmalloc (is->method->filecat[pp->cat].bsize);

    pp->next = 0;
    pp->size = 0;
    pp->offset = 0;
    pp->is = is;
    pp->decodeClientData = (*is->method->code_start)(ISAMC_DECODE);
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
        assert (pp->next != pp->pos);
        pp->offset = src - pp->buf; 
        assert (pp->offset == ISAMC_BLOCK_OFFSET_1);
        if (is->method->debug > 2)
            logf (LOG_LOG, "isc: read_block size=%d %d %d",
                 pp->size, pp->cat, pp->pos);
    }
    return pp;
}

/* returns non-zero if item could be read; 0 otherwise */
int isc_pp_read (ISAMC_PP pp, void *buf)
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
        assert (src - pp->buf == ISAMC_BLOCK_OFFSET_N);
        assert (pp->next != pp->pos);
        if (pp->deleteFlag)
            isc_release_block (is, pp->cat, pp->pos);
        (*is->method->code_item)(ISAMC_DECODE, pp->decodeClientData, dst, &src);
        pp->offset = src - pp->buf; 
        if (is->method->debug > 2)
            logf (LOG_LOG, "isc: read_block size=%d %d %d",
                 pp->size, pp->cat, pp->pos);
        return 2;
    }
    (*is->method->code_item)(ISAMC_DECODE, pp->decodeClientData, dst, &src);
    pp->offset = src - pp->buf; 
    return 1;
}

int isc_pp_num (ISAMC_PP pp)
{
    return pp->numKeys;
}

