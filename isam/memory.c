/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: memory.c,v $
 * Revision 1.5  1994-09-28 16:58:33  quinn
 * Small mod.
 *
 * Revision 1.4  1994/09/27  20:03:52  quinn
 * Seems relatively bug-free.
 *
 * Revision 1.3  1994/09/26  17:11:30  quinn
 * Trivial
 *
 * Revision 1.2  1994/09/26  17:06:35  quinn
 * Back again...
 *
 * Revision 1.1  1994/09/26  16:07:56  quinn
 * Most of the functionality in place.
 *
 */

/*
 * This module accesses and rearranges the records of the tables.
 */

#include <assert.h>

#include <util.h>
#include <isam.h>

int is_mbuf_size[3] = { 0, 1024, 4096 };

static is_mblock *mblock_tmplist = 0, *mblock_freelist = 0;
static is_mbuf *mbuf_freelist[3] = {0, 0, 0};

#define MALLOC_CHUNK 20

is_mblock *xmalloc_mblock()
{
    is_mblock *tmp;
    int i;

    if (!mblock_freelist)
    {
    	mblock_freelist = xmalloc(sizeof(is_mblock) * MALLOC_CHUNK);
	for (i = 0; i < MALLOC_CHUNK - 1; i++)
	    mblock_freelist[i].next = &mblock_freelist[i+1];
	mblock_freelist[i].next = 0;
    }
    tmp = mblock_freelist;
    mblock_freelist = mblock_freelist->next;
    tmp->next = 0;
    tmp->state = IS_MBSTATE_UNREAD;
    tmp->data = 0;
    return tmp;
}

is_mbuf *xmalloc_mbuf(int type)
{
    is_mbuf *tmp;

    if (mbuf_freelist[type])
    {
    	tmp = mbuf_freelist[type];
    	mbuf_freelist[type] = tmp->next;
    }
    else
    {
    	tmp = xmalloc(sizeof(is_mbuf) + is_mbuf_size[type]);
	tmp->type = type;
    }
    tmp->refcount = type ? 1 : 0;
    tmp->offset = tmp->num = tmp->cur_record = 0;
    tmp->data = (char*) tmp + sizeof(is_mbuf);
    tmp->next = 0;
    return tmp;
}

void xfree_mbuf(is_mbuf *p)
{
    p->next = mbuf_freelist[p->type];
    mbuf_freelist[p->type] = p;
}

void xfree_mbufs(is_mbuf *l)
{
    is_mbuf *p;

    while (l)
    {
    	p = l->next;
    	xfree_mbuf(l);
    	l = p;
    }
}

void xfree_mblock(is_mblock *p)
{
    xfree_mbufs(p->data);
    p->next = mblock_freelist;
    mblock_freelist = p;
}

void xrelease_mblock(is_mblock *p)
{
    p->next = mblock_tmplist;
    mblock_tmplist = p;
}

void xfree_mblocks(is_mblock *l)
{
    is_mblock *p;

    while (l)
    {
    	p = l->next;
    	xfree_mblock(l);
    	l = p;
    }
}

void is_m_establish_tab(ISAM is, is_mtable *tab, ISAM_P pos)
{
    tab->data = xmalloc_mblock();
    if (pos > 0)
    {
	tab->pos_type = is_type(pos);
	tab->num_records = -1;
	tab->data->num_records = -1;
	tab->data->diskpos = is_block(pos);
	tab->data->state = IS_MBSTATE_UNREAD;
	tab->data->data = 0;
	tab->cur_mblock = tab->data;
	tab->cur_mblock->cur_mbuf = 0;
    }
    else /* new block */
    {
    	tab->pos_type = 0;
    	tab->num_records = 0;
	tab->data->num_records = 0;
	tab->data->diskpos = -1;
	tab->data->state = IS_MBSTATE_CLEAN;
	tab->data->data = xmalloc_mbuf(IS_MBUF_TYPE_LARGE);
	tab->cur_mblock = tab->data;
	tab->cur_mblock->cur_mbuf = tab->data->data;
	tab->cur_mblock->cur_mbuf->cur_record = 0;
    }
    tab->is = is;
}

void is_m_release_tab(is_mtable *tab)
{
    xfree_mblocks(tab->data);
    xfree_mblocks(mblock_tmplist);
    mblock_tmplist = 0;
}

void is_m_rewind(is_mtable *tab)
{
    tab->cur_mblock = tab->data;
    if (tab->data)
    {
    	tab->data->cur_mbuf = tab->data->data;
    	if (tab->data->data)
	    tab->data->data->cur_record = 0;
    }
}

static int read_current_full(is_mtable *tab, is_mblock *mblock)
{
    if (is_p_read_full(tab, mblock) < 0)
    	return -1;
    if (mblock->nextpos && !mblock->next)
    {
    	mblock->next = xmalloc_mblock();
    	mblock->next->diskpos = mblock->nextpos;
    	mblock->next->state = IS_MBSTATE_UNREAD;
    	mblock->next->data = 0;
    }
    mblock->cur_mbuf = mblock->data;
    mblock->data->cur_record = 0;
    return 0;
}

int is_m_read_full(is_mtable *tab, is_mblock *mblock)
{
    return read_current_full(tab, mblock);
}

/*
 * replace the record right behind the pointer.
 */
void is_m_replace_record(is_mtable *tab, const void *rec)
{
    is_mbuf *mbuf = tab->cur_mblock->cur_mbuf;
    
    /* we assume that block is already in memory and that we are in the
     * right mbuf, and that it has space for us. */
    memcpy(mbuf->data + mbuf->offset + (mbuf->cur_record - 1) *
    	is_keysize(tab->is), rec, is_keysize(tab->is));
    tab->cur_mblock->state = IS_MBSTATE_DIRTY;
}

/*
 * Delete the record right behind the pointer.
 */
void is_m_delete_record(is_mtable *tab)
{
    is_mbuf *mbuf, *new;

    mbuf = tab->cur_mblock->cur_mbuf;
    if (mbuf->cur_record >= mbuf->num)  /* top of mbuf */
    {
    	mbuf->num--;
    	mbuf->cur_record--;
    }
    else /* middle of a block */
    {
	new = xmalloc_mbuf(IS_MBUF_TYPE_SMALL);
	new->next = mbuf->next;
	mbuf->next = new;
	new->data = mbuf->data;
	mbuf->refcount++;
	new->offset = mbuf->offset + mbuf->cur_record * is_keysize(tab->is);
	new->num = mbuf->num - mbuf->cur_record;
	mbuf->num = mbuf->cur_record -1;
	mbuf = mbuf->next;
	mbuf->cur_record = 0;
    }
    tab->num_records--;
    tab->cur_mblock->num_records--;
    tab->cur_mblock->state = tab->data->state = IS_MBSTATE_DIRTY;
}

int is_m_write_record(is_mtable *tab, const void *rec)
{
    is_mbuf *mbuf, *oldnext, *dmbuf;

    /* make sure block is all in memory */
    if (tab->cur_mblock->state <= IS_MBSTATE_PARTIAL)
    	if (read_current_full(tab, tab->cur_mblock) < 0)
	    return -1;
    mbuf = tab->cur_mblock->cur_mbuf;
    if (mbuf->cur_record >= mbuf->num)  /* top of mbuf */
    {
    	/* mbuf is reference or full */
    	if (mbuf->refcount != 1 || mbuf->offset + (mbuf->num + 1) *
	    is_keysize(tab->is) > is_mbuf_size[mbuf->type])
	{
	    oldnext = mbuf->next;
	    mbuf->next = xmalloc_mbuf(IS_MBUF_TYPE_LARGE);
	    mbuf->next->next = oldnext;
	    mbuf = mbuf->next;
	    tab->cur_mblock->cur_mbuf = mbuf;
	    mbuf->cur_record = 0;
	}
    }
    else
    {
	oldnext = mbuf->next;
	mbuf->next = xmalloc_mbuf(IS_MBUF_TYPE_MEDIUM);
	mbuf->next->next = dmbuf = xmalloc_mbuf(IS_MBUF_TYPE_SMALL);
	dmbuf->data = mbuf->data;
	dmbuf->next = oldnext;
	dmbuf->offset = mbuf->offset + mbuf->cur_record * is_keysize(tab->is);
	dmbuf->num = mbuf->num - mbuf->cur_record;
	mbuf->num -= dmbuf->num;
	mbuf->refcount++;
	mbuf = tab->cur_mblock->cur_mbuf = mbuf->next;
	mbuf->cur_record = 0;
    }
    log(LOG_DEBUG, "is_m_write_rec(rec == %d)", mbuf->cur_record);
    memcpy(mbuf->data + mbuf->offset + mbuf->cur_record * is_keysize(tab->is),
	rec, is_keysize(tab->is));
    mbuf->num++;
    mbuf->cur_record++;
    tab->num_records++;
    tab->cur_mblock->num_records++;
    tab->cur_mblock->state = tab->data->state = IS_MBSTATE_DIRTY;
    return 0;
}

void is_m_unread_record(is_mtable *tab)
{
    assert(tab->cur_mblock->cur_mbuf->cur_record);
    tab->cur_mblock->cur_mbuf->cur_record--;
}

/*
 * non-destructive read.
 */
int is_m_peek_record(is_mtable *tab, void *rec)
{
    is_mbuf *mbuf;
    is_mblock *mblock;

    /* make sure block is all in memory */
    if (tab->cur_mblock->state <= IS_MBSTATE_PARTIAL)
    	if (read_current_full(tab, tab->cur_mblock) < 0)
	    return -1;
    mblock = tab->cur_mblock;
    mbuf = mblock->cur_mbuf;
    if (mbuf->cur_record >= mbuf->num) /* are we at end of mbuf? */
    {
	if (!mbuf->next) /* end of mblock */
	{
	    if (mblock->next)
	    {
		mblock = mblock->next;
		if (mblock->state <= IS_MBSTATE_PARTIAL)
		    if (read_current_full(tab, mblock) < 0)
			return -1;
		mbuf = mblock->data;
	    }
	    else
	    	return 0;   /* EOTable */
	}
	else
	    mbuf = mbuf->next;
	mbuf->cur_record = 0;
    }
    memcpy(rec, mbuf->data + mbuf->offset + mbuf->cur_record *
	is_keysize(tab->is), is_keysize(tab->is));
    return 1;
}

int is_m_read_record(is_mtable *tab, void *buf)
{
    is_mbuf *mbuf;

    /* make sure block is all in memory */
    if (tab->cur_mblock->state <= IS_MBSTATE_PARTIAL)
    	if (read_current_full(tab, tab->cur_mblock) < 0)
	    return -1;
    mbuf = tab->cur_mblock->cur_mbuf;
    if (mbuf->cur_record >= mbuf->num) /* are we at end of mbuf? */
    {
	if (!mbuf->next) /* end of mblock */
	{
	    if (tab->cur_mblock->next)
	    {
		tab->cur_mblock = tab->cur_mblock->next;
		if (tab->cur_mblock->state <= IS_MBSTATE_PARTIAL)
		    if (read_current_full(tab, tab->cur_mblock) < 0)
			return -1;
		tab->cur_mblock->cur_mbuf = mbuf = tab->cur_mblock->data;
	    }
	    else
	    	return 0;   /* EOTable */
	}
	else
	    tab->cur_mblock->cur_mbuf = mbuf = mbuf->next;
	mbuf->cur_record = 0;
    }
    memcpy(buf, mbuf->data + mbuf->offset + mbuf->cur_record *
	is_keysize(tab->is), is_keysize(tab->is));
    mbuf->cur_record++;
    return 1;
}

/*
 * TODO: optimize this function by introducing a higher-level search.
 */
int is_m_seek_record(is_mtable *tab, const void *rec)
{
    char peek[IS_MAX_RECORD];
    int rs;

    for (;;)
    {
	if (is_m_read_record(tab, &peek) <= 0)
	    return 1;
	if ((rs = (*tab->is->cmp)(peek, rec)) > 0)
	{
	    is_m_unread_record(tab);
	    return 1;
	}
	else if (rs == 0)
	    return 0;
    }
}

int is_m_num_records(is_mtable *tab)
{
    if (tab->data->state < IS_MBSTATE_PARTIAL)
	if (read_current_full(tab, tab->data) < 0)
	{
	    log(LOG_FATAL, "read full failed");
	    exit(1);
	}
    return tab->num_records;
}
