/*
 * Copyright (c) 1995-1998, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 * 
 * Isamd - isam with diffs 
 *
 * todo
 *   most of it, this is just a copy of isamh
 *
 */




#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <log.h>
#include "isamd-p.h"

#include "../index/index.h" /* for dump */

static void flush_block (ISAMD is, int cat);
static void release_fc (ISAMD is, int cat);
static void init_fc (ISAMD is, int cat);

#define ISAMD_FREELIST_CHUNK 1

#define SMALL_TEST 1

ISAMD_M isamd_getmethod (ISAMD_M me)
{
    static struct ISAMD_filecat_s def_cat[] = {
#if SMALL_TEST
/*        blocksz,   max keys before switching size */
        {    32,   40 },
	{   128,    0 },
#else
        {    24,   40 },
        {  2048, 2048 },
        { 16384,    0 },

#endif 

/* old values from isamc, long time ago...
        {    24,   40 },
	{   128,  256 },
        {   512, 1024 },
        {  2048, 4096 },
        {  8192,16384 },
        { 32768,   0  },
*/

/* assume about 2 bytes per pointer, when compressed. The head uses */
/* 16 bytes, and other blocks use 8 for header info... If you want 3 */
/* blocks of 32 bytes, say max 16+24+24 = 64 keys */

    };
    ISAMD_M m = (ISAMD_M) xmalloc (sizeof(*m));
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



ISAMD isamd_open (BFiles bfs, const char *name, int writeflag, ISAMD_M method)
{
    ISAMD is;
    ISAMD_filecat filecat;
    int i = 0;

    is = (ISAMD) xmalloc (sizeof(*is));

    is->method = (ISAMD_M) xmalloc (sizeof(*is->method));
    memcpy (is->method, method, sizeof(*method));
    filecat = is->method->filecat;
    assert (filecat);

    /* determine number of block categories */
    if (is->method->debug)
        logf (LOG_LOG, "isamd: bsize  maxkeys");
    do
    {
        if (is->method->debug)
            logf (LOG_LOG, "isamd:%6d %6d",
                  filecat[i].bsize, filecat[i].mblocks);
    } while (filecat[i++].mblocks);
    is->no_files = i;
    is->max_cat = --i;

    assert (is->no_files > 0);
    is->files = (ISAMD_file) xmalloc (sizeof(*is->files)*is->no_files);
    if (writeflag)
    {
      /* TODO - what ever needs to be done here... */
    }
    else
    {
    }

    for (i = 0; i<is->no_files; i++)
    {
        char fname[512];

        sprintf (fname, "%s%c", name, i+'A');
        is->files[i].bf = bf_open (bfs, fname, is->method->filecat[i].bsize,
                                   writeflag);
        is->files[i].head_is_dirty = 0;
        if (!bf_read (is->files[i].bf, 0, 0, sizeof(ISAMD_head),
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
        is->files[i].no_writes = 0; /* clear statistics */
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

int isamd_block_used (ISAMD is, int type)
{
    if (type < 0 || type >= is->no_files)
	return -1;
    return is->files[type].head.lastblock-1;
}

int isamd_block_size (ISAMD is, int type)
{
    ISAMD_filecat filecat = is->method->filecat;
    if (type < 0 || type >= is->no_files)
	return -1;
    return filecat[type].bsize;
}

int isamd_close (ISAMD is)
{
    int i;

    if (is->method->debug)
    {
	logf (LOG_LOG, "isamd:    next    forw   mid-f    prev   backw   mid-b");
	for (i = 0; i<is->no_files; i++)
	    logf (LOG_LOG, "isamd:%8d%8d%8.1f%8d%8d%8.1f",
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
        logf (LOG_LOG, "isamd:  writes   reads skipped   alloc released  remap");
    for (i = 0; i<is->no_files; i++)
    {
        release_fc (is, i);
        assert (is->files[i].bf);
        if (is->files[i].head_is_dirty)
            bf_write (is->files[i].bf, 0, 0, sizeof(ISAMD_head),
                 &is->files[i].head);
        if (is->method->debug)
            logf (LOG_LOG, "isamd:%8d%8d%8d%8d%8d%8d",
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
    xfree (is->method);
    xfree (is);
    return 0;
}

int isamd_read_block (ISAMD is, int cat, int pos, char *dst)
{
    ++(is->files[cat].no_reads);
    return bf_read (is->files[cat].bf, pos, 0, 0, dst);
}

int isamd_write_block (ISAMD is, int cat, int pos, char *src)
{
    ++(is->files[cat].no_writes);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isamd: write_block %d %d", cat, pos);
    return bf_write (is->files[cat].bf, pos, 0, 0, src);
}

int isamd_write_dblock (ISAMD is, int cat, int pos, char *src,
                      int nextpos, int offset)
{
    ISAMD_BLOCK_SIZE size = offset + ISAMD_BLOCK_OFFSET_N;
    if (is->method->debug > 2)
        logf (LOG_LOG, "isamd: write_dblock. size=%d nextpos=%d",
              (int) size, nextpos);
    src -= ISAMD_BLOCK_OFFSET_N;
    assert( ISAMD_BLOCK_OFFSET_N == sizeof(int)+sizeof(int) );
    memcpy (src, &nextpos, sizeof(int));
    memcpy (src + sizeof(int), &size, sizeof(size));
    return isamd_write_block (is, cat, pos, src);
}

#if ISAMD_FREELIST_CHUNK
static void flush_block (ISAMD is, int cat)
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

static int alloc_block (ISAMD is, int cat)
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

static void release_block (ISAMD is, int cat, int pos)
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
static void flush_block (ISAMD is, int cat)
{
    char *abuf = is->files[cat].alloc_buf;
    xfree (abuf);
}

static int alloc_block (ISAMD is, int cat)
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

static void release_block (ISAMD is, int cat, int pos)
{
    char buf[sizeof(int)];
   
    (is->files[cat].no_released)++;
    is->files[cat].head_is_dirty = 1; 
    memcpy (buf, &is->files[cat].head.freelist, sizeof(int));
    is->files[cat].head.freelist = pos;
    bf_write (is->files[cat].bf, pos, 0, sizeof(int), buf);
}
#endif

int isamd_alloc_block (ISAMD is, int cat)
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
        logf (LOG_LOG, "isamd: alloc_block in cat %d: %d", cat, block);
    return block;
}

void isamd_release_block (ISAMD is, int cat, int pos)
{
    if (is->method->debug > 3)
        logf (LOG_LOG, "isamd: release_block in cat %d: %d", cat, pos);
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

static void init_fc (ISAMD is, int cat)
{
    int j = 100;
        
    is->files[cat].fc_max = j;
    is->files[cat].fc_list = (int *)
	xmalloc (sizeof(*is->files[0].fc_list) * j);
    while (--j >= 0)
        is->files[cat].fc_list[j] = 0;
}

static void release_fc (ISAMD is, int cat)
{
    int b, j = is->files[cat].fc_max;

    while (--j >= 0)
        if ((b = is->files[cat].fc_list[j]))
        {
            release_block (is, cat, b);
            is->files[cat].fc_list[j] = 0;
        }
}

void isamd_pp_close (ISAMD_PP pp)
{
    ISAMD is = pp->is;

    (*is->method->code_stop)(ISAMD_DECODE, pp->decodeClientData);
    xfree (pp->buf);
    xfree (pp);
}

ISAMD_PP isamd_pp_open (ISAMD is, ISAMD_P ipos)
{
    ISAMD_PP pp = (ISAMD_PP) xmalloc (sizeof(*pp));
    char *src;
   
    pp->cat = isamd_type(ipos);
    pp->pos = isamd_block(ipos); 

    src = pp->buf = (char *) xmalloc (is->method->filecat[pp->cat].bsize);

    pp->next = 0;
    pp->size = 0;
    pp->offset = 0;
    pp->is = is;
    pp->decodeClientData = (*is->method->code_start)(ISAMD_DECODE);
    //pp->deleteFlag = 0;
    pp->numKeys = 0;
    pp->diffs=0;
    
    if (pp->pos)
    {
        src = pp->buf;
        isamd_read_block (is, pp->cat, pp->pos, src);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        memcpy (&pp->numKeys, src, sizeof(pp->numKeys));
        src += sizeof(pp->numKeys);
        memcpy (&pp->diffs, src, sizeof(pp->diffs));
        src += sizeof(pp->diffs);
        assert (pp->next != pp->pos);
        pp->offset = src - pp->buf; 
        assert (pp->offset == ISAMD_BLOCK_OFFSET_1);
        if (is->method->debug > 2)
            logf (LOG_LOG, "isamd_pp_open sz=%d c=%d p=%d n=%d",
                 pp->size, pp->cat, pp->pos, isamd_block(pp->next));
    }
    return pp;
}



void isamd_buildfirstblock(ISAMD_PP pp){
  char *dst=pp->buf;
  assert(pp->buf);
  assert(pp->next != pp->pos); 
  memcpy(dst, &pp->next, sizeof(pp->next) );
  dst += sizeof(pp->next);
  memcpy(dst, &pp->size,sizeof(pp->size));
  dst += sizeof(pp->size);
  memcpy(dst, &pp->numKeys, sizeof(pp->numKeys));
  dst += sizeof(pp->numKeys);
  memcpy(dst, &pp->diffs, sizeof(pp->diffs));
  dst += sizeof(pp->diffs);  
  assert (dst - pp->buf  == ISAMD_BLOCK_OFFSET_1);
  if (pp->is->method->debug > 2)
     logf (LOG_LOG, "isamd: first: sz=%d  p=%d/%d>%d/%d nk=%d d=%d",
           pp->size, 
           pp->cat, pp->pos, 
           isamd_type(pp->next), isamd_block(pp->next),
           pp->numKeys, pp->diffs);
}

void isamd_buildlaterblock(ISAMD_PP pp){
  char *dst=pp->buf;
  assert(pp->buf);
  assert(pp->next != isamd_addr(pp->pos,pp->cat)); 
  memcpy(dst, &pp->next, sizeof(pp->next) );
  dst += sizeof(pp->next);
  memcpy(dst, &pp->size,sizeof(pp->size));
  dst += sizeof(pp->size);
  assert (dst - pp->buf  == ISAMD_BLOCK_OFFSET_N);
  if (pp->is->method->debug > 2)
     logf (LOG_LOG, "isamd: l8r: sz=%d  p=%d/%d>%d/%d",
           pp->size, 
           pp->pos, pp->cat, 
           isamd_block(pp->next), isamd_type(pp->next) );
}



/* returns non-zero if item could be read; 0 otherwise */
int isamd_pp_read (ISAMD_PP pp, void *buf)
{
    return isamd_read_item (pp, (char **) &buf);
}

/* read one item from file - decode and store it in *dst.
   Returns
     0 if end-of-file
     1 if item could be read ok and NO boundary
     2 if item could be read ok and boundary */
int isamd_read_item (ISAMD_PP pp, char **dst)
{
    ISAMD is = pp->is;
    char *src = pp->buf + pp->offset;
    int newcat;

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
	newcat = isamd_type(pp->next);
	if (pp->cat != newcat ) {
	  pp->buf = xrealloc(pp->buf, is->method->filecat[newcat].bsize);
	}
        pp->pos = isamd_block(pp->next);
        pp->cat = isamd_type(pp->next);
        
        src = pp->buf;
	/* read block and save 'next' and 'size' entry */
        isamd_read_block (is, pp->cat, pp->pos, src);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        /* assume block is non-empty */
        assert (src - pp->buf == ISAMD_BLOCK_OFFSET_N);
        assert (pp->next != isamd_addr(pp->pos,pp->cat));
        //if (pp->deleteFlag)
        //    isamd_release_block (is, pp->cat, pp->pos);
        (*is->method->code_reset)(pp->decodeClientData);
        (*is->method->code_item)(ISAMD_DECODE, pp->decodeClientData, dst, &src);
        pp->offset = src - pp->buf; 
        if (is->method->debug > 2)
            logf (LOG_LOG, "isamd: read_block size=%d %d %d next=%d",
                 pp->size, pp->cat, pp->pos, pp->next);
        return 2;
    }
    (*is->method->code_item)(ISAMD_DECODE, pp->decodeClientData, dst, &src);
    pp->offset = src - pp->buf; 
    return 1;
}

int isamd_pp_num (ISAMD_PP pp)
{
    return pp->numKeys;
}

static char *hexdump(unsigned char *p, int len, char *buff) {
  static char localbuff[128];
  char bytebuff[8];
  if (!buff) buff=localbuff;
  *buff='\0';
  while (len--) {
    sprintf(bytebuff,"%02x",*p);
    p++;
    strcat(buff,bytebuff);
    if (len) strcat(buff," ");
  }
  return buff;
}


void isamd_pp_dump (ISAMD is, ISAMD_P ipos)
{
  ISAMD_PP pp;
  ISAMD_P oldaddr=0;
  struct it_key key;
  int i,n;
  int occur =0;
  int oldoffs;
  char hexbuff[64];
  
  logf(LOG_LOG,"dumping isamd block %d (%d:%d)",
                  (int)ipos, isamd_type(ipos), isamd_block(ipos) );
  pp=isamd_pp_open(is,ipos);
  logf(LOG_LOG,"numKeys=%d,  ofs=%d d=%d",
       pp->numKeys, 
       pp->offset, pp->diffs);
  oldoffs= pp->offset;
  while(isamd_pp_read(pp, &key))
  {
     if (oldaddr != isamd_addr(pp->pos,pp->cat) )
     {
        oldaddr = isamd_addr(pp->pos,pp->cat); 
        logf(LOG_LOG,"block %d (%d:%d) sz=%d nx=%d (%d:%d) ofs=%d",
                  isamd_addr(pp->pos,pp->cat), 
                  pp->cat, pp->pos, pp->size,
                  pp->next, isamd_type(pp->next), isamd_block(pp->next),
                  pp->offset);
        i=0;      
        while (i<pp->size) {
          n=pp->size-i;
          if (n>8) n=8;
          logf(LOG_LOG,"  %05x: %s",i,hexdump(pp->buf+i,n,hexbuff));
          i+=n;
        }
        if (oldoffs >  ISAMD_BLOCK_OFFSET_N)
           oldoffs=ISAMD_BLOCK_OFFSET_N;
     } /* new block */
     occur++;
     logf (LOG_LOG,"    got %d:%d=%x:%x from %s at %d=%x",
      	          key.sysno, key.seqno,
	          key.sysno, key.seqno,
	          hexdump(pp->buf+oldoffs, pp->offset-oldoffs, hexbuff),
	          oldoffs, oldoffs);
     oldoffs = pp->offset;
  }
  /*!*/ /*TODO: dump diffs too!!! */
  isamd_pp_close(pp);
} /* dump */

/*
 * $Log: isamd.c,v $
 * Revision 1.2  1999-07-14 15:05:30  heikki
 * slow start on isam-d
 *
 * Revision 1.1  1999/07/14 12:34:43  heikki
 * Copied from isamh, starting to change things...
 *
 *
 */