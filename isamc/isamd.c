/*
 * Copyright (c) 1995-1998, Index Data.
 * See the file LICENSE for details.
 * $Id: isamd.c,v 1.21 2002-07-11 16:16:00 heikki Exp $ 
 *
 * Isamd - isam with diffs 
 * Programmed by: Heikki Levanto
 *
 * Todo
 *  - Statistics are missing and/or completely wrong
 *  - Lots of code stolen from isamc, not all needed any more
 */


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <yaz/log.h>
#include "../index/index.h"  /* isamd uses the internal structure of it_key */
#include "isamd-p.h"

static void flush_block (ISAMD is, int cat);
static void release_fc (ISAMD is, int cat);
static void init_fc (ISAMD is, int cat);

#define ISAMD_FREELIST_CHUNK 1

#define SMALL_TEST 0

ISAMD_M isamd_getmethod (ISAMD_M me)
{
    static struct ISAMD_filecat_s def_cat[] = {
#if SMALL_TEST
/*        blocksz,   max. Unused time being */
        {    32,   40 },  /* 24 is the smallest unreasonable size! */
	{    64,    0 },
#else
        {    32,    1 },
	{   128,    1 },
	{   256,    1 },
	{   512,    1 },
        {  1024,    1 },
        {  2048,    1 },
        {  4096,    1 },
        {  8192,    0 },

#endif
#ifdef SKIPTHIS



        {    32,    1 },
        {   128,    1 },
        {   512,    1 },
        {  2048,    1 },
        {  8192,    1 },
        { 32768,    1 },
        {131072,    0 },

        {    24,    1 }, /* Experimental sizes */
        {    32,    1 },
        {    64,    1 },
        {   128,    1 },
        {   256,    1 },
        {   512,    1 },
        {  1024,    1 },
        {  2048,    0 },
#endif 

    };
    ISAMD_M m = (ISAMD_M) xmalloc (sizeof(*m));  /* never released! */
    m->filecat = def_cat;                        /* ok, only alloc'd once */

    m->code_start = NULL;
    m->code_item = NULL;
    m->code_stop = NULL;
    m->code_reset = NULL;

    m->compare_item = NULL;

    m->debug = 0; /* default to no debug */

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
    if (is->method->debug>0)
        logf (LOG_LOG, "isamd: bsize  maxkeys");
    do
    {
        if (is->method->debug>0)
            logf (LOG_LOG, "isamd:%6d %6d",
                  filecat[i].bsize, filecat[i].mblocks);
    } while (filecat[i++].mblocks);
    is->no_files = i;
    is->max_cat = --i;
 
    assert (is->no_files > 0);
    assert (is->max_cat <=8 ); /* we have only 3 bits for it */
    
    is->files = (ISAMD_file) xmalloc (sizeof(*is->files)*is->no_files);

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
        is->files[i].no_op_diffonly=0;
        is->files[i].no_op_main=0;
        init_fc (is, i);
    }
    is->last_pos=0;
    is->last_cat=0;   
    is->no_read=0;    
    is->no_read_main=0;
    is->no_write=0;   
    is->no_op_single=0;
    is->no_op_new=0;
    is->no_read_keys=0;
    is->no_read_eof=0;
    is->no_seek_nxt=0;
    is->no_seek_sam=0;
    is->no_seek_fwd=0;
    is->no_seek_prv=0;
    is->no_seek_bak=0;
    is->no_seek_cat=0;
    is->no_fbuilds=0;
    is->no_appds=0;
    is->no_merges=0;
    is->no_non=0;
    is->no_singles=0;

    return is;
}

int isamd_block_used (ISAMD is, int type)
{
    if ( type==-1) /* singleton */
      return 0; 
    if (type < 0 || type >= is->no_files)
	return -1;
    return is->files[type].head.lastblock-1;
}

int isamd_block_size (ISAMD is, int type)
{
    ISAMD_filecat filecat = is->method->filecat;
    if ( type==-1) /* singleton */
      return 0; /* no bytes used */ 
    if (type < 0 || type >= is->no_files)
	return -1;
    return filecat[type].bsize;
}

int isamd_close (ISAMD is)
{
    int i;
    int s;

    if (is->method->debug>0)
    {
        logf (LOG_LOG, "isamd statistics");
	logf (LOG_LOG, "f    nxt   forw  mid-f   prev  backw  mid-b");
	for (i = 0; i<is->no_files; i++)
	    logf (LOG_LOG, "%d%7d%7d%7.1f%7d%7d%7.1f",i,
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
    if (is->method->debug>0)
        logf (LOG_LOG, "f  writes   reads skipped   alloc released ");
    for (i = 0; i<is->no_files; i++)
    {
        release_fc (is, i);
        assert (is->files[i].bf);
        if (is->files[i].head_is_dirty)
            bf_write (is->files[i].bf, 0, 0, sizeof(ISAMD_head),
                 &is->files[i].head);
        if (is->method->debug>0)
            logf (LOG_LOG, "%d%8d%8d%8d%8d%8d",i,
                  is->files[i].no_writes,
                  is->files[i].no_reads,
                  is->files[i].no_skip_writes,
                  is->files[i].no_allocated,
                  is->files[i].no_released);
        xfree (is->files[i].fc_list);
	flush_block (is, i);
        bf_close (is->files[i].bf);
    }
    
    if (is->method->debug>0) 
    {
        logf (LOG_LOG, "f   opens    main  diffonly");
        for (i = 0; i<is->no_files; i++)
        {
            logf (LOG_LOG, "%d%8d%8d%8d",i,
                  is->files[i].no_op_main+
                  is->files[i].no_op_diffonly,
                  is->files[i].no_op_main,
                  is->files[i].no_op_diffonly);
        }
        logf(LOG_LOG,"open single  %8d", is->no_op_single);
        logf(LOG_LOG,"open new     %8d", is->no_op_new);

        logf(LOG_LOG, "new build   %8d", is->no_fbuilds);
        logf(LOG_LOG, "append      %8d", is->no_appds);
        logf(LOG_LOG, "  merges    %8d", is->no_merges);
        logf(LOG_LOG, "  singles   %8d", is->no_singles);
        logf(LOG_LOG, "  no-ops    %8d", is->no_non);

        logf(LOG_LOG, "read blocks %8d", is->no_read);
        logf(LOG_LOG, "read keys:  %8d %8.1f k/bl", 
                  is->no_read_keys, 
                  1.0*(is->no_read_keys+1)/(is->no_read+1) );
        logf(LOG_LOG, "read main-k %8d %8.1f %% of keys",
                  is->no_read_main,
                  100.0*(is->no_read_main+1)/(is->no_read_keys+1) );
        logf(LOG_LOG, "read ends:  %8d %8.1f k/e",
                  is->no_read_eof,
                  1.0*(is->no_read_keys+1)/(is->no_read_eof+1) );
        s= is->no_seek_nxt+ is->no_seek_sam+ is->no_seek_fwd +
           is->no_seek_prv+ is->no_seek_bak+ is->no_seek_cat;
        if (s==0) 
          s++;
        logf(LOG_LOG, "seek same   %8d %8.1f%%",
            is->no_seek_sam, 100.0*is->no_seek_sam/s );
        logf(LOG_LOG, "seek next   %8d %8.1f%%",
            is->no_seek_nxt, 100.0*is->no_seek_nxt/s );
        logf(LOG_LOG, "seek prev   %8d %8.1f%%",
            is->no_seek_prv, 100.0*is->no_seek_prv/s );
        logf(LOG_LOG, "seek forw   %8d %8.1f%%",
            is->no_seek_fwd, 100.0*is->no_seek_fwd/s );
        logf(LOG_LOG, "seek back   %8d %8.1f%%",
            is->no_seek_bak, 100.0*is->no_seek_bak/s );
        logf(LOG_LOG, "seek cat    %8d %8.1f%%",
            is->no_seek_cat, 100.0*is->no_seek_cat/s );
    }
    xfree (is->files);
    xfree (is->method);
    xfree (is);
    return 0;
}

static void isamd_seek_stat(ISAMD is, int cat, int pos)
{
  if (cat != is->last_cat)
     is->no_seek_cat++;
  else if ( pos == is->last_pos)
     is->no_seek_sam++;
  else if ( pos == is->last_pos+1)
     is->no_seek_nxt++;
  else if ( pos == is->last_pos-1)
     is->no_seek_prv++;
  else if ( pos > is->last_pos)
     is->no_seek_fwd++;
  else if ( pos < is->last_pos)
     is->no_seek_bak++;
  is->last_cat = cat;
  is->last_pos = pos;
} /* seek_stat */

int isamd_read_block (ISAMD is, int cat, int pos, char *dst)
{
    isamd_seek_stat(is,cat,pos);
    ++(is->files[cat].no_reads);
    ++(is->no_read);
    if (is->method->debug > 6)
        logf (LOG_LOG, "isamd: read_block %d:%d",cat, pos);
    return bf_read (is->files[cat].bf, pos, 0, 0, dst);
}

int isamd_write_block (ISAMD is, int cat, int pos, char *src)
{
    isamd_seek_stat(is,cat,pos);
    ++(is->files[cat].no_writes);
    ++(is->no_write);
    if (is->method->debug > 6)
        logf (LOG_LOG, "isamd: write_block %d:%d", cat, pos);
    return bf_write (is->files[cat].bf, pos, 0, 0, src);
}

int isamd_write_dblock (ISAMD is, int cat, int pos, char *src,
                      int nextpos, int offset)
{
    ISAMD_BLOCK_SIZE size = offset + ISAMD_BLOCK_OFFSET_N;
    if (is->method->debug > 4)
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
    if (is->method->debug > 4)
        logf (LOG_LOG, "isamd: alloc_block in cat %d: %d", cat, block);
    return block;
}

void isamd_release_block (ISAMD is, int cat, int pos)
{
    if (is->method->debug > 4)
        logf (LOG_LOG, "isamd: release_block in cat %d: %d", cat, pos);
    assert(pos!=0);
    
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
    isamd_free_diffs(pp);  /* see merge-d.h */
    if (is->method->debug > 5)
       logf (LOG_LOG, "isamd_pp_close %p %d=%d:%d  sz=%d n=%d=%d:%d nk=%d",
             pp, isamd_addr(pp->pos, pp->cat), pp->cat, pp->pos, pp->size, 
             pp->next, isamd_type(pp->next), isamd_block(pp->next), 
             pp->numKeys );
    xfree (pp->buf);
    xfree (pp);
}



ISAMD_PP isamd_pp_open (ISAMD is, ISAMD_P ipos)
{
    ISAMD_PP pp = (ISAMD_PP) xmalloc (sizeof(*pp));
    char *src;
    int sz = is->method->filecat[is->max_cat].bsize;
                 /* always allocate for the largest blocks, saves trouble */
    struct it_key singlekey;
    char *c_ptr; /* for fake encoding the singlekey */
    char *i_ptr;
    int ofs;
    
    pp->numKeys = 0;
    src = pp->buf = (char *) xmalloc (sz);
    memset(src,'\0',sz); /* clear the buffer, for new blocks */
    
    pp->next = 0;
    pp->size = 0;
    pp->offset = 0;
    pp->is = is;
    pp->diffs=0;
    pp->diffbuf=0;
    pp->diffinfo=0;
    pp->decodeClientData = (*is->method->code_start)(ISAMD_DECODE);
    
    if ( is_singleton(ipos) ) 
    {
       pp->cat=0; 
       pp->pos=0;
       if (is->method->debug > 5)
          logf (LOG_LOG, "isamd_pp_open  %p %d=%d:%d  sz=%d n=%d=%d:%d",
                pp, isamd_addr(pp->pos, pp->cat), pp->cat, pp->pos, pp->size, 
                pp->next, isamd_type(pp->next), isamd_block(pp->next) );
       singleton_decode(ipos, &singlekey );
       pp->offset=ISAMD_BLOCK_OFFSET_1;
       pp->numKeys = 1;
       ofs=pp->offset+sizeof(int); /* reserve length of diffsegment */
       singlekey.seqno = singlekey.seqno * 2 + 1; /* make an insert diff */  
       c_ptr=&(pp->buf[ofs]);
       i_ptr=(char*)(&singlekey); 
       (*is->method->code_item)(ISAMD_ENCODE, pp->decodeClientData, 
                                &c_ptr, &i_ptr);
       (*is->method->code_reset)(pp->decodeClientData);
       ofs += c_ptr-&(pp->buf[ofs]);
       memcpy( &(pp->buf[pp->offset]), &ofs, sizeof(int) );
       /* since we memset buf earlier, we already have a zero endmark! */
       pp->size = ofs;
       if (is->method->debug > 5)
          logf (LOG_LOG, "isamd_pp_open single %d=%x: %d.%d sz=%d", 
            ipos,ipos, 
            singlekey.sysno, singlekey.seqno/2,
            pp->size );
       is->no_op_single++;
       return pp;
    } /* singleton */
   
    pp->cat = isamd_type(ipos);
    pp->pos = isamd_block(ipos); 
    
    if (0==pp->pos)
      is->no_op_new++; 
      
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
        assert (pp->next != isamd_addr(pp->pos,pp->cat));
        pp->offset = src - pp->buf; 
        assert (pp->offset == ISAMD_BLOCK_OFFSET_1);
        assert(pp->size>=ISAMD_BLOCK_OFFSET_1); /*??*/
        if (pp->next)
          is->files[pp->cat].no_op_main++;
        else
          is->files[pp->cat].no_op_diffonly++;
    }
    if (is->method->debug > 5)
       logf (LOG_LOG, "isamd_pp_open  %p %d=%d:%d  sz=%d n=%d=%d:%d",
             pp, isamd_addr(pp->pos, pp->cat), pp->cat, pp->pos, pp->size, 
             pp->next, isamd_type(pp->next), isamd_block(pp->next) );

    return pp;
}



void isamd_buildfirstblock(ISAMD_PP pp){
  char *dst=pp->buf;
  assert(pp->buf);
  assert(pp->next != isamd_addr(pp->pos,pp->cat)); 
  memcpy(dst, &pp->next, sizeof(pp->next) );
  dst += sizeof(pp->next);
  memcpy(dst, &pp->size,sizeof(pp->size));
  dst += sizeof(pp->size);
  memcpy(dst, &pp->numKeys, sizeof(pp->numKeys));
  dst += sizeof(pp->numKeys);
  assert (dst - pp->buf  == ISAMD_BLOCK_OFFSET_1);
  if (pp->is->method->debug > 5)
     logf (LOG_LOG, "isamd: bldfirst:  p=%d=%d:%d n=%d:%d:%d sz=%d nk=%d ",
           isamd_addr(pp->pos,pp->cat),pp->cat, pp->pos, 
           pp->next, isamd_type(pp->next), isamd_block(pp->next),
           pp->size, pp->numKeys);
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
  if (pp->is->method->debug > 5)
     logf (LOG_LOG, "isamd: l8r: sz=%d  p=%d/%d>%d/%d",
           pp->size, 
           pp->pos, pp->cat, 
           isamd_block(pp->next), isamd_type(pp->next) );
}



/* returns non-zero if item could be read; 0 otherwise */
int isamd_pp_read (ISAMD_PP pp, void *buf)
{

    return isamd_read_item (pp, (char **) &buf);
       /* note: isamd_read_item is in merge-d.c, because it is so */
       /* convoluted with the merge process */
}

/* read one main item from file - decode and store it in *dst.
   Does not worry about diffs
   Returns
     0 if end-of-file
     1 if item could be read ok
*/
int isamd_read_main_item (ISAMD_PP pp, char **dst)
{
    ISAMD is = pp->is;
    char *src = pp->buf + pp->offset;
    int newcat;
    int oldoffs;

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
        pp->pos = isamd_block(pp->next);
        pp->cat = isamd_type(pp->next);
        pp->is->no_read_main++;
        src = pp->buf;
	/* read block and save 'next' and 'size' entry */
        isamd_read_block (is, pp->cat, pp->pos, src);
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        /* assume block is non-empty */
        pp->offset = oldoffs = src - pp->buf; 
        assert (pp->offset == ISAMD_BLOCK_OFFSET_N);
        assert (pp->next != isamd_addr(pp->pos,pp->cat));
        (*is->method->code_reset)(pp->decodeClientData);
        /* finally, read the item */
        (*is->method->code_item)(ISAMD_DECODE, pp->decodeClientData, dst, &src);
        pp->offset = src - pp->buf; 
        if (is->method->debug > 8)
            logf (LOG_LOG, "isamd: read_m: block %d:%d sz=%d ofs=%d-%d next=%d",
                 pp->cat, pp->pos, pp->size, oldoffs, pp->offset, pp->next);
        return 2;
    }
    oldoffs=pp->offset;
    (*is->method->code_item)(ISAMD_DECODE, pp->decodeClientData, dst, &src);
    pp->offset = src - pp->buf; 
    if (is->method->debug > 8)
        logf (LOG_LOG, "isamd: read_m: got %d:%d sz=%d ofs=%d-%d next=%d",
             pp->cat, pp->pos, pp->size, oldoffs, pp->offset, pp->next);
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
  int diffmax=1;
  int diffidx;
  char hexbuff[64];
  int olddebug= is->method->debug;
  is->method->debug=0; /* no debug logs while reading for dump */
  
  logf(LOG_LOG,"dumping isamd block %d (%d:%d)",
                  (int)ipos, isamd_type(ipos), isamd_block(ipos) );
  pp=isamd_pp_open(is,ipos);
  logf(LOG_LOG,"numKeys=%d,  ofs=%d sz=%d",
       pp->numKeys, pp->offset, pp->size );
  diffidx=oldoffs= pp->offset;
  while ((diffidx < is->method->filecat[pp->cat].bsize) && (diffmax>0))
  {
    memcpy(&diffmax,&(pp->buf[diffidx]),sizeof(int));
    logf (LOG_LOG,"diff set at %d-%d: %s", diffidx, diffmax, 
      hexdump(pp->buf+diffidx,8,0)); 
      /*! todo: dump the actual diffs as well !!! */
    diffidx=diffmax;
    
  } /* dump diffs */
  while(isamd_pp_read(pp, &key))
  {
     if (oldaddr != isamd_addr(pp->pos,pp->cat) )
     {
        oldaddr = isamd_addr(pp->pos,pp->cat); 
        logf(LOG_LOG,"block %d=%d:%d sz=%d nx=%d=%d:%d ofs=%d",
                  isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                  pp->size,
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
  is->method->debug=olddebug;
} /* dump */

/*
 * $Log: isamd.c,v $
 * Revision 1.21  2002-07-11 16:16:00  heikki
 * Fixed a bug in isamd, failed to store a single key when its bits
 * did not fit into a singleton.
 *
 * Revision 1.20  2002/06/19 10:29:18  adam
 * align block sizes for isam sys. Better plot for test
 *
 * Revision 1.19  1999/11/30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.18  1999/10/06 15:18:13  heikki
 *
 * Improving block sizes again
 *
 * Revision 1.17  1999/10/06 11:46:36  heikki
 * mproved statistics on isam-d
 *
 * Revision 1.16  1999/10/05 09:57:40  heikki
 * Tuning the isam-d (and fixed a small "detail")
 *
 * Revision 1.15  1999/09/27 14:36:36  heikki
 * singletons
 *
 * Revision 1.14  1999/09/23 18:01:18  heikki
 * singleton optimising
 *
 * Revision 1.13  1999/09/20 15:48:06  heikki
 * Small changes
 *
 * Revision 1.12  1999/09/13 13:28:28  heikki
 * isam-d optimizing: merging input data in the same go
 *
 * Revision 1.11  1999/08/25 18:09:24  heikki
 * Starting to optimize
 *
 * Revision 1.10  1999/08/24 13:17:42  heikki
 * Block sizes, comments
 *
 * Revision 1.9  1999/08/20 12:25:58  heikki
 * Statistics in isamd
 *
 * Revision 1.8  1999/08/18 13:28:16  heikki
 * Set log levels to decent values
 *
 * Revision 1.6  1999/08/17 19:44:25  heikki
 * Fixed memory leaks
 *
 * Revision 1.4  1999/08/04 14:21:18  heikki
 * isam-d seems to be working.
 *
 * Revision 1.3  1999/07/21 14:24:50  heikki
 * isamd write and read functions ok, except when diff block full.
 * (merge not yet done)
 *
 * Revision 1.1  1999/07/14 12:34:43  heikki
 * Copied from isamh, starting to change things...
 *
 *
 */
