/*
 * Copyright (c) 1995-1998, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 * 
 * Isamh - append-only isam 
 *
 */




#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <log.h>
#include "isamh-p.h"

static void flush_block (ISAMH is, int cat);
static void release_fc (ISAMH is, int cat);
static void init_fc (ISAMH is, int cat);

#define ISAMH_FREELIST_CHUNK 1

#define SMALL_TEST 0

ISAMH_M isamh_getmethod (void)
{
    static struct ISAMH_filecat_s def_cat[] = {
#if SMALL_TEST
/*        blocksz,   max keys before switching size */
        {    32,  40 },
	{    64,   0 },
#else
        {    24,   40 },
	{   128,  256 },
        {   512, 1024 },
        {  2048, 4096 },
        {  8192,16384 },
        { 32768,   0  },
#endif 
/* assume about 2 bytes per pointer, when compressed. The head uses */
/* 16 bytes, and other blocks use 8 for header info... If you want 3 */
/* blocks of 32 bytes, say max 16+24+24 = 64 keys */

    };
    ISAMH_M m = (ISAMH_M) xmalloc (sizeof(*m));
    m->filecat = def_cat;

    m->code_start = NULL;
    m->code_item = NULL;
    m->code_stop = NULL;
    m->code_reset = NULL;

    m->compare_item = NULL;

    m->debug = 1;

    m->max_blocks_mem = 10;

    return m;
}


ISAMH isamh_open (BFiles bfs, const char *name, int writeflag, ISAMH_M method)
{
    ISAMH is;
    ISAMH_filecat filecat;
    int i = 0;
    int max_buf_size = 0;

    is = (ISAMH) xmalloc (sizeof(*is));

    is->method = (ISAMH_M) xmalloc (sizeof(*is->method));
    memcpy (is->method, method, sizeof(*method));
    filecat = is->method->filecat;
    assert (filecat);

    /* determine number of block categories */
    if (is->method->debug)
        logf (LOG_LOG, "isc: bsize  ifill  mfill mblocks");
    do
    {
        if (is->method->debug)
            logf (LOG_LOG, "isc:%6d %6d",
                  filecat[i].bsize, filecat[i].mblocks);
        if (max_buf_size < filecat[i].bsize)
            max_buf_size = filecat[i].bsize;
    } while (filecat[i++].mblocks);
    is->no_files = i;
    is->max_cat = --i;
#ifdef SKIPTHIS
    /* max_buf_size is the larget buffer to be used during merge */
    max_buf_size = (1 + max_buf_size / filecat[i].bsize) * filecat[i].bsize;
    if (max_buf_size < (1+is->method->max_blocks_mem) * filecat[i].bsize)
        max_buf_size = (1+is->method->max_blocks_mem) * filecat[i].bsize;
#endif

    if (is->method->debug)
        logf (LOG_LOG, "isc: max_buf_size %d", max_buf_size);
    
    assert (is->no_files > 0);
    is->files = (ISAMH_file) xmalloc (sizeof(*is->files)*is->no_files);
    if (writeflag)
    {
#ifdef SKIPTHIS
        is->merge_buf = (char *) xmalloc (max_buf_size+256);
	memset (is->merge_buf, 0, max_buf_size+256);
#else
        is->startblock = (char *) xmalloc (max_buf_size+256);
	memset (is->startblock, 0, max_buf_size+256);
        is->lastblock = (char *) xmalloc (max_buf_size+256);
	memset (is->lastblock, 0, max_buf_size+256);
	/* The spare 256 bytes should not be needed! */
#endif
    }
    else
        is->startblock = is->lastblock = NULL;

    for (i = 0; i<is->no_files; i++)
    {
        char fname[512];

        sprintf (fname, "%s%c", name, i+'A');
        is->files[i].bf = bf_open (bfs, fname, is->method->filecat[i].bsize,
                                   writeflag);
        is->files[i].head_is_dirty = 0;
        if (!bf_read (is->files[i].bf, 0, 0, sizeof(ISAMH_head),
                     &is->files[i].head))
        {
            is->files[i].head.lastblock = 1;
            is->files[i].head.freelist = 0;
        }
	is->files[i].alloc_entries_num = 0;
	is->files[i].alloc_entries_max =
	    is->method->filecat[i].bsize / sizeof(int) - 1;
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

int isamh_block_used (ISAMH is, int type)
{
    if (type < 0 || type >= is->no_files)
	return -1;
    return is->files[type].head.lastblock-1;
}

int isamh_block_size (ISAMH is, int type)
{
    ISAMH_filecat filecat = is->method->filecat;
    if (type < 0 || type >= is->no_files)
	return -1;
    return filecat[type].bsize;
}

int isamh_close (ISAMH is)
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
            bf_write (is->files[i].bf, 0, 0, sizeof(ISAMH_head),
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
    xfree (is->startblock);
    xfree (is->lastblock);
    xfree (is->method);
    xfree (is);
    return 0;
}

int isamh_read_block (ISAMH is, int cat, int pos, char *dst)
{
    ++(is->files[cat].no_reads);
    return bf_read (is->files[cat].bf, pos, 0, 0, dst);
}

int isamh_write_block (ISAMH is, int cat, int pos, char *src)
{
    ++(is->files[cat].no_writes);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_block %d %d", cat, pos);
    return bf_write (is->files[cat].bf, pos, 0, 0, src);
}

int isamh_write_dblock (ISAMH is, int cat, int pos, char *src,
                      int nextpos, int offset)
{
    ISAMH_BLOCK_SIZE size = offset + ISAMH_BLOCK_OFFSET_N;
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_dblock. size=%d nextpos=%d",
              (int) size, nextpos);
    src -= ISAMH_BLOCK_OFFSET_N;
    memcpy (src, &nextpos, sizeof(int));
    memcpy (src + sizeof(int), &size, sizeof(size));
    return isamh_write_block (is, cat, pos, src);
}

#if ISAMH_FREELIST_CHUNK
static void flush_block (ISAMH is, int cat)
{
    char *abuf = is->files[cat].alloc_buf;
    int block = is->files[cat].head.freelist;
    if (block && is->files[cat].alloc_entries_num)
    {
	memcpy (abuf, &is->files[cat].alloc_entries_num, sizeof(int));
	bf_write (is->files[cat].bf, block, 0, 0, abuf);
	is->files[cat].alloc_entries_num = 0;
    }
    xfree (abuf);
}

static int alloc_block (ISAMH is, int cat)
{
    int block = is->files[cat].head.freelist;
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
		    sizeof(int));
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
	    memcpy (&block, abuf + sizeof(int) + sizeof(int) *
		    is->files[cat].alloc_entries_num, sizeof(int));
    }
    return block;
}

static void release_block (ISAMH is, int cat, int pos)
{
    char *abuf = is->files[cat].alloc_buf;
    int block = is->files[cat].head.freelist;

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
	memcpy (abuf + sizeof(int), &block, sizeof(int));
	is->files[cat].head.freelist = pos;
	is->files[cat].head_is_dirty = 1; 
    }
    else
    {
	memcpy (abuf + sizeof(int) +
		is->files[cat].alloc_entries_num*sizeof(int),
		&pos, sizeof(int));
    }
    is->files[cat].alloc_entries_num++;
}
#else
static void flush_block (ISAMH is, int cat)
{
    char *abuf = is->files[cat].alloc_buf;
    xfree (abuf);
}

static int alloc_block (ISAMH is, int cat)
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

static void release_block (ISAMH is, int cat, int pos)
{
    char buf[sizeof(int)];
   
    (is->files[cat].no_released)++;
    is->files[cat].head_is_dirty = 1; 
    memcpy (buf, &is->files[cat].head.freelist, sizeof(int));
    is->files[cat].head.freelist = pos;
    bf_write (is->files[cat].bf, pos, 0, sizeof(int), buf);
}
#endif

int isamh_alloc_block (ISAMH is, int cat)
{
    int block = 0;

    if (is->files[cat].fc_list)
    {
        int j, nb;
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
        logf (LOG_LOG, "isc: alloc_block in cat %d: %d", cat, block);
    return block;
}

void isamh_release_block (ISAMH is, int cat, int pos)
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

static void init_fc (ISAMH is, int cat)
{
    int j = 100;
        
    is->files[cat].fc_max = j;
    is->files[cat].fc_list = (int *)
	xmalloc (sizeof(*is->files[0].fc_list) * j);
    while (--j >= 0)
        is->files[cat].fc_list[j] = 0;
}

static void release_fc (ISAMH is, int cat)
{
    int b, j = is->files[cat].fc_max;

    while (--j >= 0)
        if ((b = is->files[cat].fc_list[j]))
        {
            release_block (is, cat, b);
            is->files[cat].fc_list[j] = 0;
        }
}

void isamh_pp_close (ISAMH_PP pp)
{
    ISAMH is = pp->is;

    (*is->method->code_stop)(ISAMH_DECODE, pp->decodeClientData);
    xfree (pp->buf);
    xfree (pp);
}

ISAMH_PP isamh_pp_open (ISAMH is, ISAMH_P ipos)
{
    ISAMH_PP pp = (ISAMH_PP) xmalloc (sizeof(*pp));
    char *src;
   
    pp->cat = isamh_type(ipos);
    pp->pos = isamh_block(ipos); 

    src = pp->buf = (char *) xmalloc (is->method->filecat[pp->cat].bsize);

    pp->next = 0;
    pp->size = 0;
    pp->offset = 0;
    pp->is = is;
    pp->decodeClientData = (*is->method->code_start)(ISAMH_DECODE);
    pp->deleteFlag = 0;
    pp->numKeys = 0;
    pp->lastblock=0;
    
    if (pp->pos)
    {
        src = pp->buf;
        isamh_read_block (is, pp->cat, pp->pos, src);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        memcpy (&pp->numKeys, src, sizeof(pp->numKeys));
        src += sizeof(pp->numKeys);
        memcpy (&pp->lastblock, src, sizeof(pp->lastblock));
        src += sizeof(pp->lastblock);
        assert (pp->next != pp->pos);
        pp->offset = src - pp->buf; 
        assert (pp->offset == ISAMH_BLOCK_OFFSET_1);
        if (is->method->debug > 2)
            logf (LOG_LOG, "isamh_pp_open sz=%d c=%d p=%d n=%d",
                 pp->size, pp->cat, pp->pos, isamh_block(pp->next));
    }
    return pp;
}



void isamh_buildfirstblock(ISAMH_PP pp){
  char *dst=pp->buf;
  assert(pp->buf);
  assert(pp->next != pp->pos); 
  memcpy(dst, &pp->next, sizeof(pp->next) );
  dst += sizeof(pp->next);
  memcpy(dst, &pp->size,sizeof(pp->size));
  dst += sizeof(pp->size);
  memcpy(dst, &pp->numKeys, sizeof(pp->numKeys));
  dst += sizeof(pp->numKeys);
  memcpy(dst, &pp->lastblock, sizeof(pp->lastblock));
  dst += sizeof(pp->lastblock);  
  assert (dst - pp->buf  == ISAMH_BLOCK_OFFSET_1);
  if (pp->is->method->debug > 2)
     logf (LOG_LOG, "isamh: first: sz=%d  p=%d/%d>%d/%d>%d/%d nk=%d",
           pp->size, 
           pp->pos, pp->cat, 
           isamh_block(pp->next), isamh_type(pp->next),
           isamh_block(pp->lastblock), isamh_type(pp->lastblock),
           pp->numKeys);
}

void isamh_buildlaterblock(ISAMH_PP pp){
  char *dst=pp->buf;
  assert(pp->buf);
  assert(pp->next != isamh_addr(pp->pos,pp->cat)); 
  memcpy(dst, &pp->next, sizeof(pp->next) );
  dst += sizeof(pp->next);
  memcpy(dst, &pp->size,sizeof(pp->size));
  dst += sizeof(pp->size);
  assert (dst - pp->buf  == ISAMH_BLOCK_OFFSET_N);
  if (pp->is->method->debug > 2)
     logf (LOG_LOG, "isamh: l8r: sz=%d  p=%d/%d>%d/%d",
           pp->size, 
           pp->pos, pp->cat, 
           isamh_block(pp->next), isamh_type(pp->next) );
}



/* returns non-zero if item could be read; 0 otherwise */
int isamh_pp_read (ISAMH_PP pp, void *buf)
{
    return isamh_read_item (pp, (char **) &buf);
}

/* read one item from file - decode and store it in *dst.
   Returns
     0 if end-of-file
     1 if item could be read ok and NO boundary
     2 if item could be read ok and boundary */
int isamh_read_item (ISAMH_PP pp, char **dst)
{
    ISAMH is = pp->is;
    char *src = pp->buf + pp->offset;

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
        isamh_read_block (is, pp->cat, pp->pos, src);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        /* assume block is non-empty */
        assert (src - pp->buf == ISAMH_BLOCK_OFFSET_N);
        assert (pp->next != pp->pos);
        if (pp->deleteFlag)
            isamh_release_block (is, pp->cat, pp->pos);
        (*is->method->code_item)(ISAMH_DECODE, pp->decodeClientData, dst, &src);
        pp->offset = src - pp->buf; 
        if (is->method->debug > 2)
            logf (LOG_LOG, "isc: read_block size=%d %d %d next=%d",
                 pp->size, pp->cat, pp->pos, pp->next);
        return 2;
    }
    (*is->method->code_item)(ISAMH_DECODE, pp->decodeClientData, dst, &src);
    pp->offset = src - pp->buf; 
    return 1;
}

int isamh_pp_num (ISAMH_PP pp)
{
    return pp->numKeys;
}

/*
 * $Log: isamh.c,v $
 * Revision 1.4  1999-07-07 09:36:04  heikki
 * Fixed an assertion in isamh
 *
 * Revision 1.2  1999/07/06 09:37:05  heikki
 * Working on isamh - not ready yet.
 *
 * Revision 1.1  1999/06/30 15:04:54  heikki
 * Copied from isamc.c, slowly starting to simplify...
 *
 */