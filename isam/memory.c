/* $Id: memory.c,v 1.18 2002-08-02 19:26:56 adam Exp $
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
 * This module accesses and rearranges the records of the tables.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zebrautl.h>
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
    	mblock_freelist = (is_mblock *)
	    xmalloc(sizeof(is_mblock) * MALLOC_CHUNK);
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
    	tmp = (is_mbuf*) xmalloc(sizeof(is_mbuf) + is_mbuf_size[type]);
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
	tab->last_mbuf = 0;
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
	tab->last_mbuf = 0;
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
    is_mbuf *mbuf, *inew;

    mbuf = tab->cur_mblock->cur_mbuf;
    if (mbuf->cur_record >= mbuf->num)  /* top of mbuf */
    {
    	mbuf->num--;
    	mbuf->cur_record--;
    }
    else if (mbuf->cur_record == 1) /* beginning of mbuf */
    {
	mbuf->num--;
	mbuf->offset +=is_keysize(tab->is);
	mbuf->cur_record = 0;
    }
    else /* middle of mbuf */
    {
	/* insert block after current one */
	inew = xmalloc_mbuf(IS_MBUF_TYPE_SMALL);
	inew->next = mbuf->next;
	mbuf->next = inew;

	/* virtually transfer everything after current record to new one. */
	inew->data = mbuf->data;
	mbuf->refcount++;
	inew->offset = mbuf->offset + mbuf->cur_record * is_keysize(tab->is);
	inew->num = mbuf->num - mbuf->cur_record;
	
	/* old buf now only contains stuff before current record */
	mbuf->num = mbuf->cur_record -1;
	tab->cur_mblock->cur_mbuf = inew;
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
    /*
    logf (LOG_DEBUG, "is_m_write_rec(rec == %d)", mbuf->cur_record);
    */
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
    if (tab->last_mbuf)
	tab->cur_mblock->cur_mbuf = tab->last_mbuf;
    else
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

int is_m_read_record(is_mtable *tab, void *buf, int keep)
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
	    if (!keep && tab->cur_mblock->state == IS_MBSTATE_CLEAN &&
		tab->cur_mblock->diskpos > 0)
	    {
		xfree_mbufs(tab->cur_mblock->data);
		tab->cur_mblock->data = 0;
		tab->cur_mblock->state = IS_MBSTATE_UNREAD;
	    }
	    if (tab->cur_mblock->next)
	    {
		tab->cur_mblock = tab->cur_mblock->next;
		if (tab->cur_mblock->state <= IS_MBSTATE_PARTIAL)
		    if (read_current_full(tab, tab->cur_mblock) < 0)
			return -1;
		tab->cur_mblock->cur_mbuf = mbuf = tab->cur_mblock->data;
		tab->last_mbuf = 0;
	    }
	    else
	    	return 0;   /* EOTable */
	}
	else
	{
	    tab->last_mbuf = mbuf;
	    tab->cur_mblock->cur_mbuf = mbuf = mbuf->next;
	}
	mbuf->cur_record = 0;
    }
    else
	tab->last_mbuf = 0;
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
	if (is_m_read_record(tab, &peek, 1) <= 0)
	    return 1;
	if ((rs = (*tab->is->cmp)(peek, rec)) > 0)
	{
	    is_m_unread_record(tab);
	    return rs;
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
	    logf (LOG_FATAL, "read full failed");
	    exit(1);
	}
    return tab->num_records;
}