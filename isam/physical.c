/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: physical.c,v $
 * Revision 1.2  1994-09-26 17:06:36  quinn
 * Back again...
 *
 * Revision 1.1  1994/09/26  16:07:57  quinn
 * Most of the functionality in place.
 *
 */

/*
 * This module handles the representation of tables in the bfiles.
 */

#include <assert.h>

#include <isam.h>
#include <ismemory.h>

static int is_freestore_alloc(ISAM is, int type)
{
    int tmp;

    if (is->types[type].freelist >= 0)
    {
    	tmp = is->types[type].freelist;
    	if (bf_read(is->types[type].bf, tmp, 0, sizeof(tmp),
	    &is->types[type].freelist) <=0)
	{
	    log(LOG_FATAL, "Failed to allocate block");
	    exit(1);
	}
    }
    else
    	tmp = is->types[type].top++;

    return tmp;
}

static void is_freestore_free(ISAM is, int type, int block)
{
    int tmp;

    tmp = is->types[type].freelist;
    is->types[type].freelist = block;
    if (bf_write(is->types[type].bf, block, 0, sizeof(tmp), &tmp) < 0)
    {
    	log(LOG_FATAL, "Failed to deallocate block.");
    	exit(1);
    }
}

/* this code must be modified to handle an index */
int is_p_read_partial(is_mtable *tab, is_mblock *block)
{
    int toread;
    is_mbuf *buf;

    assert(block->state == IS_MBSTATE_UNREAD);
    block->data = buf =  xmalloc_mbuf(IS_MBUF_TYPE_LARGE);
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
    	log(LOG_FATAL, "bfread failed.");
    	return -1;
    }
    /* extract header info */
    buf->offset = 0;
    memcpy(&block->num_records, buf->data, sizeof(block->num_records));
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
    buf->num = (toread - buf->offset) / is_keysize(tab->is);
    if (buf->num >= block->num_records)
    {
    	buf->num = block->num_records;
    	block->state = IS_MBSTATE_CLEAN;
    }
    else
    	block->bread = buf->num * is_keysize(tab->is);
    return 0;
}

int is_p_read_full(is_mtable *tab, is_mblock *block)
{
    is_mbuf *buf;
    int dread, toread;

    if (block->state == IS_MBSTATE_UNREAD && is_p_read_partial(tab, block) < 0)
    {
    	log(LOG_FATAL, "partial read failed.");
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
	    	log(LOG_FATAL, "bfread failed.");
	    	return -1;
	    }
	    buf->offset = 0;
	    buf->num = toread;
	    dread += toread;
	    block->bread += toread * is_keysize(tab->is);
    	}
    }
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
	for (b = p->data; b; b = b->next)
	{
	    memcpy(type->dbuf + sum, b->data + b->offset, v = b->num *
		is_keysize(tab->is));
	    sum += v;
	    assert(sum <= type->blocksize);
	}
	if (bf_write(type->bf, p->diskpos, 0, sum, type->dbuf) < 0)
	{
	    log(LOG_FATAL, "Failed to write block.");
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
    	if (p->diskpos >= 0)
    	{
	    is_freestore_free(tab->is, tab->pos_type, p->diskpos);
	    p->diskpos = -1;
	}
}

static is_mbuf *mbuf_takehead(is_mbuf **mb, int *num, int keysize)
{
    is_mbuf *p = 0, **pp = &p, *new;
    int toget = *num;

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
	new = xmalloc_mbuf(IS_MBUF_TYPE_SMALL);
	new->next = (*mb)->next;
	(*mb)->next = new;
	new->data = (*mb)->data;
	(*mb)->refcount++;
	new->offset = (*mb)->offset + toget * keysize;
	new->num = (*mb)->num - toget;
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
    is_mblock *mblock, *new;
    is_mbuf *mbufs, *mbp;
    int blocks, recsblock;

    log(LOG_DEBUG, "Realigning table.");
    for (mblock = tab->data; mblock; mblock = mblock->next)
    {
    	if (mblock->state == IS_MBSTATE_DIRTY && mblock->num_records >
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
		is_keysize(tab->is))))
	    {
	    	new = xmalloc_mblock();
	    	new->diskpos = -1;
	    	new->state = IS_MBSTATE_DIRTY;
	    	new->next = mblock->next;
	    	mblock->next = new;
	    	mblock->data = mbp;
	    	mblock->num_records = recsblock;
	    	mblock = mblock->next;
	    }
	}
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

    log(LOG_DEBUG, "Remapping table.");
    /* collect all data */
    bufpp = &mbufs;
    for (blockp = tab->data; blockp; blockp = blockp->next)
    {
    	*bufpp = blockp->data;
    	while (*bufpp)
	    bufpp = &(*bufpp)->next;
        blockp->data = 0;
    }
    blocks = tab->num_records / tab->is->types[tab->pos_type].nice_keys_block;
    if (tab->num_records % tab->is->types[tab->pos_type].nice_keys_block)
    	blocks++;
    recsblock = tab->num_records / blocks;
    if (recsblock < 1)
    	recsblock = 1;
    blockpp = &tab->data;
    while ((mbp = mbuf_takehead(&mbufs, &recsblock, is_keysize(tab->is))))
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
}
