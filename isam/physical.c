/* $Id: physical.c,v 1.18 2002-08-02 19:26:56 adam Exp $
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



/*
 * This module handles the representation of tables in the bfiles.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <yaz/log.h>
#include <isam.h>

static int is_freestore_alloc(ISAM is, int type)
{
    int tmp;

    if (is->types[type].freelist >= 0)
    {
    	tmp = is->types[type].freelist;
    	if (bf_read(is->types[type].bf, tmp, 0, sizeof(tmp),
	    &is->types[type].freelist) <=0)
	{
	    logf (LOG_FATAL, "Failed to allocate block");
	    exit(1);
	}
    }
    else
    	tmp = is->types[type].top++;

    logf (LOG_DEBUG, "Allocating block #%d", tmp);
    return tmp;
}

static void is_freestore_free(ISAM is, int type, int block)
{
    int tmp;

    logf (LOG_DEBUG, "Releasing block #%d", block);
    tmp = is->types[type].freelist;
    is->types[type].freelist = block;
    if (bf_write(is->types[type].bf, block, 0, sizeof(tmp), &tmp) < 0)
    {
    	logf (LOG_FATAL, "Failed to deallocate block.");
    	exit(1);
    }
}

/* this code must be modified to handle an index */
int is_p_read_partial(is_mtable *tab, is_mblock *block)
{
    int toread;
    is_mbuf *buf;

    assert(block->state == IS_MBSTATE_UNREAD);
    block->data = buf = xmalloc_mbuf(IS_MBUF_TYPE_LARGE);
    toread = tab->is->types[tab->pos_type].blocksize;
    if (toread > is_mbuf_size[buf->type])
    {
    	toread = is_mbuf_size[buf->type];
    	block->state = IS_MBSTATE_PARTIAL;
    }
    else
    	block->state = IS_MBSTATE_CLEAN;
    if (bf_read(tab->is->types[tab->pos_type].bf, block->diskpos, 0, toread,
	buf->data) < 0)
    {
    	logf (LOG_FATAL, "bfread failed.");
    	return -1;
    }
    /* extract header info */
    buf->offset = 0;
    memcpy(&block->num_records, buf->data, sizeof(block->num_records));
    assert(block->num_records > 0);
    buf->offset += sizeof(block->num_records);
    memcpy(&block->nextpos, buf->data + buf->offset,
	sizeof(block->nextpos));
    buf->offset += sizeof(block->nextpos);
    if (block == tab->data) /* first block */
    {
        memcpy(&tab->num_records, buf->data + buf->offset,
	    sizeof(tab->num_records));
	buf->offset +=sizeof(tab->num_records);
    }
    logf(LOG_DEBUG, "R: Block #%d: num %d nextpos %d total %d",
        block->diskpos, block->num_records, block->nextpos,
	block == tab->data ? tab->num_records : -1);
    buf->num = (toread - buf->offset) / is_keysize(tab->is);
    if (buf->num >= block->num_records)
    {
    	buf->num = block->num_records;
    	block->state = IS_MBSTATE_CLEAN;
    }
    else
    	block->bread = buf->offset + buf->num * is_keysize(tab->is);
    return 0;
}

int is_p_read_full(is_mtable *tab, is_mblock *block)
{
    is_mbuf *buf;
    int dread, toread;

    if (block->state == IS_MBSTATE_UNREAD && is_p_read_partial(tab, block) < 0)
    {
    	logf (LOG_FATAL, "partial read failed.");
    	return -1;
    }
    if (block->state == IS_MBSTATE_PARTIAL)
    {
    	buf = block->data;
    	dread = block->data->num;
    	while (dread < block->num_records)
    	{
	    buf->next = xmalloc_mbuf(IS_MBUF_TYPE_LARGE);
	    buf = buf->next;

	    toread = is_mbuf_size[buf->type] / is_keysize(tab->is);
	    if (toread > block->num_records - dread)
	    	toread = block->num_records - dread;

	    if (bf_read(tab->is->types[tab->pos_type].bf, block->diskpos, block->bread, toread *
		is_keysize(tab->is), buf->data) < 0)
	    {
	    	logf (LOG_FATAL, "bfread failed.");
	    	return -1;
	    }
	    buf->offset = 0;
	    buf->num = toread;
	    dread += toread;
	    block->bread += toread * is_keysize(tab->is);
    	}
	block->state = IS_MBSTATE_CLEAN;
    }
    logf (LOG_DEBUG, "R: Block #%d contains %d records.", block->diskpos, block->num_records);
    return 0;
}

/*
 * write dirty blocks to bfile.
 * Allocate blocks as necessary.
 */
void is_p_sync(is_mtable *tab)
{
    is_mblock *p;
    is_mbuf *b;
    int sum, v;
    isam_blocktype *type;

    type = &tab->is->types[tab->pos_type];
    for (p = tab->data; p; p = p->next)
    {
    	if (p->state < IS_MBSTATE_DIRTY)
	    continue;
	/* make sure that blocks are allocated. */
    	if (p->diskpos < 0)
	    p->diskpos = is_freestore_alloc(tab->is, tab->pos_type);
	if (p->next)
	{
	    if (p->next->diskpos < 0)
		p->nextpos = p->next->diskpos = is_freestore_alloc(tab->is,
		    tab->pos_type);
	    else
	    	p->nextpos = p->next->diskpos;
	}
	else
	    p->nextpos = 0;
	sum = 0;
	memcpy(type->dbuf, &p->num_records, sizeof(p->num_records));
	sum += sizeof(p->num_records);
	memcpy(type->dbuf + sum, &p->nextpos, sizeof(p->nextpos));
	sum += sizeof(p->nextpos);
	if (p == tab->data)  /* first block */
	{
	    memcpy(type->dbuf + sum, &tab->num_records,
		sizeof(tab->num_records));
	    sum += sizeof(tab->num_records);
	}
	logf (LOG_DEBUG, "W: Block #%d contains %d records.", p->diskpos,
	    p->num_records);
	assert(p->num_records > 0);
	for (b = p->data; b; b = b->next)
	{
            logf(LOG_DEBUG, "   buf: offset %d, keys %d, type %d, ref %d",
	    	b->offset, b->num, b->type, b->refcount);
	    if ((v = b->num * is_keysize(tab->is)) > 0)
		memcpy(type->dbuf + sum, b->data + b->offset, v);

	    sum += v;
	    assert(sum <= type->blocksize);
	}
	if (bf_write(type->bf, p->diskpos, 0, sum, type->dbuf) < 0)
	{
	    logf (LOG_FATAL, "Failed to write block.");
	    exit(1);
	}
    }
}

/*
 * Free all disk blocks associated with table.
 */
void is_p_unmap(is_mtable *tab)
{
    is_mblock *p;

    for (p = tab->data; p; p = p->next)
    {
    	if (p->diskpos >= 0)
    	{
	    is_freestore_free(tab->is, tab->pos_type, p->diskpos);
	    p->diskpos = -1;
	}
    }
}

static is_mbuf *mbuf_takehead(is_mbuf **mb, int *num, int keysize)
{
    is_mbuf *p = 0, **pp = &p, *inew;
    int toget = *num;

    if (!toget)
    	return 0;
    while (*mb && toget >= (*mb)->num)
    {
	toget -= (*mb)->num;
	*pp = *mb;
	*mb = (*mb)->next;
	(*pp)->next = 0;
	pp = &(*pp)->next;
    }
    if (toget > 0 && *mb)
    {
	inew = xmalloc_mbuf(IS_MBUF_TYPE_SMALL);
	inew->next = (*mb)->next;
	(*mb)->next = inew;
	inew->data = (*mb)->data;
	(*mb)->refcount++;
	inew->offset = (*mb)->offset + toget * keysize;
	inew->num = (*mb)->num - toget;
	(*mb)->num = toget;
	*pp = *mb;
	*mb = (*mb)->next;
	(*pp)->next = 0;
	toget = 0;
    }
    *num -= toget;
    return p;
}

/*
 * Split up individual blocks which have grown too large.
 * is_p_align and is_p_remap are alternative functions which trade off
 * speed in updating versus optimum usage of disk blocks.
 */
void is_p_align(is_mtable *tab)
{
    is_mblock *mblock, *inew, *last = 0, *next;
    is_mbuf *mbufs, *mbp;
    int blocks, recsblock;

    logf (LOG_DEBUG, "Realigning table.");
    for (mblock = tab->data; mblock; mblock = next)
    {
        next = mblock->next;
        if (mblock->state == IS_MBSTATE_DIRTY && mblock->num_records == 0)
        {
	    if (last)
	    {
	    	last->next = mblock->next;
	    	last->state = IS_MBSTATE_DIRTY;
	    	next = mblock->next;
	    }
	    else
	    {
	    	next = tab->data->next;
		if (next)
		{
		    if (next->state < IS_MBSTATE_CLEAN)
		    {
			if (is_p_read_full(tab, next) < 0)
			{
			    logf(LOG_FATAL, "Error during re-alignment");
			    abort();
			}
			if (next->nextpos && !next->next)
			{
			    next->next = xmalloc_mblock();
			    next->next->diskpos = next->nextpos;
			    next->next->state = IS_MBSTATE_UNREAD;
			    next->next->data = 0;
			}
		    }
		    next->state = IS_MBSTATE_DIRTY; /* force re-process */
		    tab->data = next;
		}
	    }
	    if (mblock->diskpos >= 0)
		is_freestore_free(tab->is, tab->pos_type, mblock->diskpos);
	    xrelease_mblock(mblock);
	}
    	else if (mblock->state == IS_MBSTATE_DIRTY && mblock->num_records >
	    (mblock == tab->data ?
	    tab->is->types[tab->pos_type].max_keys_block0 :
	    tab->is->types[tab->pos_type].max_keys_block))
	{
	    blocks = tab->num_records /
	    tab->is->types[tab->pos_type].nice_keys_block;
	    if (tab->num_records %
		tab->is->types[tab->pos_type].nice_keys_block)
		blocks++;
	    recsblock = tab->num_records / blocks;
	    if (recsblock < 1)
		recsblock = 1;
	    mbufs = mblock->data;
	    while ((mbp = mbuf_takehead(&mbufs, &recsblock,
		is_keysize(tab->is))) && recsblock)
	    {
	    	if (mbufs)
	    	{
		    inew = xmalloc_mblock();
		    inew->diskpos = -1;
		    inew->state = IS_MBSTATE_DIRTY;
		    inew->next = mblock->next;
		    mblock->next = inew;
		}
	    	mblock->data = mbp;
	    	mblock->num_records = recsblock;
	    	last = mblock;
	    	mblock = mblock->next;
	    }
	    next = mblock; 
	}
	else
	    last = mblock;
    }
}

/*
 * Reorganize data in blocks for minimum block usage and quick access.
 * Free surplus blocks.
 * is_p_align and is_p_remap are alternative functions which trade off
 * speed in updating versus optimum usage of disk blocks.
 */
void is_p_remap(is_mtable *tab)
{
    is_mbuf *mbufs, **bufpp, *mbp;
    is_mblock *blockp, **blockpp;
    int recsblock, blocks;

    logf (LOG_DEBUG, "Remapping table.");
    /* collect all data */
    bufpp = &mbufs;
    for (blockp = tab->data; blockp; blockp = blockp->next)
    {
    	if (blockp->state < IS_MBSTATE_CLEAN && is_m_read_full(tab, blockp) < 0)
	{
	    logf (LOG_FATAL, "Read-full failed in remap.");
	    exit(1);
	}
    	*bufpp = blockp->data;
    	while (*bufpp)
	    bufpp = &(*bufpp)->next;
        blockp->data = 0;
    }
    blocks = tab->num_records / tab->is->types[tab->pos_type].nice_keys_block;
    if (tab->num_records % tab->is->types[tab->pos_type].nice_keys_block)
    	blocks++;
    if (blocks == 0)
    	blocks = 1;
    recsblock = tab->num_records / blocks + 1;
    if (recsblock > tab->is->types[tab->pos_type].nice_keys_block)
    	recsblock--;
    blockpp = &tab->data;
    while ((mbp = mbuf_takehead(&mbufs, &recsblock, is_keysize(tab->is))) &&
    	recsblock)
    {
    	if (!*blockpp)
    	{
	    *blockpp = xmalloc_mblock();
	    (*blockpp)->diskpos = -1;
	}
    	(*blockpp)->data = mbp;
    	(*blockpp)->num_records = recsblock;
    	(*blockpp)->state = IS_MBSTATE_DIRTY;
    	blockpp = &(*blockpp)->next;
    }
    if (mbp)
    	xfree_mbufs(mbp);
    if (*blockpp)
    {
    	for (blockp = *blockpp; blockp; blockp = blockp->next)
	    if (blockp->diskpos >= 0)
		is_freestore_free(tab->is, tab->pos_type, blockp->diskpos);
    	xfree_mblocks(*blockpp);
    	*blockpp = 0;
    }
}