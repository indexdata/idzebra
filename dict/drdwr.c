/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#include <sys/types.h>
#include <fcntl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dict-p.h"

void dict_pr_lru (Dict_BFile bf)
{
    struct Dict_file_block *p;
    for (p=bf->lru_back; p; p = p->lru_next)
    {
        printf (" %d", p->no);
    }
    printf ("\n");
    fflush (stdout);
}

static struct Dict_file_block *find_block (Dict_BFile bf, int no)
{
    struct Dict_file_block *p;

    for (p=bf->hash_array[no% bf->hash_size]; p; p=p->h_next)
        if (p->no == no)
            break;
    return p;
}

static void release_block (Dict_BFile bf, struct Dict_file_block *p)
{
    assert (p);

    /* remove from lru queue */    
    if (p->lru_prev)
        p->lru_prev->lru_next = p->lru_next;
    else
        bf->lru_back = p->lru_next;
    if (p->lru_next)
        p->lru_next->lru_prev = p->lru_prev;
    else
        bf->lru_front = p->lru_prev;

    /* remove from hash chain */
    *p->h_prev = p->h_next;
    if (p->h_next)
        p->h_next->h_prev = p->h_prev;

    /* move to list of free blocks */
    p->h_next = bf->free_list;
    bf->free_list = p;
}

void dict_bf_flush_blocks (Dict_BFile bf, int no_to_flush)
{
    struct Dict_file_block *p;
    int i;
    for (i=0; i != no_to_flush && bf->lru_back; i++)
    {
        p = bf->lru_back;
        if (p->dirty)
        {
	    if (!bf->compact_flag)
		bf_write (bf->bf, p->no, 0, 0, p->data);
	    else
	    {
		int effective_block = p->no / bf->block_size;
		int effective_offset = p->no -
		    effective_block * bf->block_size;
		int remain = bf->block_size - effective_offset;

		if (remain >= p->nbytes)
		{
		    bf_write (bf->bf, effective_block, effective_offset,
			      p->nbytes, p->data);
#if 0
		    yaz_log (YLOG_LOG, "bf_write no=%d offset=%d size=%d",
			  effective_block, effective_offset,
			  p->nbytes);
#endif
			  
		}
		else
		{
#if 0
		    yaz_log (YLOG_LOG, "bf_write1 no=%d offset=%d size=%d",
			  effective_block, effective_offset,
			  remain);
#endif
		    bf_write (bf->bf, effective_block, effective_offset,
			      remain, p->data);
#if 0
		    yaz_log (YLOG_LOG, "bf_write2 no=%d offset=%d size=%d",
			  effective_block+1, 0, p->nbytes - remain);
#endif
		    bf_write (bf->bf, effective_block+1, 0,
			      p->nbytes - remain, (char*)p->data + remain);
		}
	    }
        }
        release_block (bf, p);
    }
}

static struct Dict_file_block *alloc_block (Dict_BFile bf, int no)
{
    struct Dict_file_block *p, **pp;

    if (!bf->free_list)
        dict_bf_flush_blocks (bf, 1);
    assert (bf->free_list);
    p = bf->free_list;
    bf->free_list = p->h_next;
    p->dirty = 0;
    p->no = no;

    /* insert at front in lru chain */
    p->lru_next = NULL;
    p->lru_prev = bf->lru_front;
    if (bf->lru_front)
        bf->lru_front->lru_next = p;
    else
        bf->lru_back = p;
    bf->lru_front = p;

    /* insert in hash chain */
    pp = bf->hash_array + (no % bf->hash_size);
    p->h_next = *pp;
    p->h_prev = pp;
    if (*pp)
        (*pp)->h_prev = &p->h_next;
    *pp = p;
   
    return p;
}

static void move_to_front (Dict_BFile bf, struct Dict_file_block *p)
{
    /* Already at front? */
    if (!p->lru_next)
        return ;   

    /* Remove */
    if (p->lru_prev)
        p->lru_prev->lru_next = p->lru_next;
    else
        bf->lru_back = p->lru_next;
    p->lru_next->lru_prev = p->lru_prev;

    /* Insert at front */
    p->lru_next = NULL;
    p->lru_prev = bf->lru_front;
    if (bf->lru_front)
        bf->lru_front->lru_next = p;
    else
        bf->lru_back = p;
    bf->lru_front = p;
}

int dict_bf_readp (Dict_BFile bf, int no, void **bufp)
{
    struct Dict_file_block *p;
    int i;
    if ((p = find_block (bf, no)))
    {
        *bufp = p->data;
        move_to_front (bf, p);
        bf->hits++;
        return 1;
    }
    bf->misses++;
    p = alloc_block (bf, no);

    if (!bf->compact_flag)
	i = bf_read (bf->bf, no, 0, 0, p->data);
    else
    {
	int effective_block = no / bf->block_size;
	int effective_offset = no - effective_block * bf->block_size;

	i = bf_read (bf->bf, effective_block, effective_offset,
		     bf->block_size - effective_offset, p->data);
	if (i > 0 && effective_offset > 0)
	    i = bf_read (bf->bf, effective_block+1, 0, effective_offset,
			 (char*) p->data + bf->block_size - effective_offset);
	i = 1;
    }
    if (i > 0)
    {
        *bufp = p->data;
        return i;
    }
    release_block (bf, p);
    *bufp = NULL;
    return i;
}

int dict_bf_newp (Dict_BFile dbf, int no, void **bufp, int nbytes)
{
    struct Dict_file_block *p;
    if (!(p = find_block (dbf, no)))
        p = alloc_block (dbf, no);
    else
        move_to_front (dbf, p);
    *bufp = p->data;
    memset (p->data, 0, dbf->block_size);
    p->dirty = 1;
    p->nbytes = nbytes;
#if 0
    printf ("bf_newp of %d:", no);
    dict_pr_lru (dbf);
#endif
    return 1;
}

int dict_bf_touch (Dict_BFile dbf, int no)
{
    struct Dict_file_block *p;
    if ((p = find_block (dbf, no)))
    {
        p->dirty = 1;
        return 0;
    }
    return -1;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

