/* $Id: rsmultiandor.c,v 1.22 2006-07-04 10:25:22 adam Exp $
   Copyright (C) 1995-2006
   Index Data ApS

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


/**
 * \file rsmultiandor.c
 * \brief This module implements the rsmulti_or and rsmulti_and result sets
 *
 * rsmultior is based on a heap, from which we find the next hit.
 *
 * rsmultiand is based on a simple array of rsets, and a linear
 * search to find the record that exists in all of those rsets.
 * To speed things up, the array is sorted so that the smallest
 * rsets come first, they are most likely to have the hits furthest
 * away, and thus forwarding to them makes the most sense.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <idzebra/util.h>
#include <idzebra/isamc.h>
#include <rset.h>

static RSFD r_open_and (RSET ct, int flag);
static RSFD r_open_or (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static int r_read_and (RSFD rfd, void *buf, TERMID *term);
static int r_read_or (RSFD rfd, void *buf, TERMID *term);
static int r_write (RSFD rfd, const void *buf);
static int r_forward_and(RSFD rfd, void *buf, TERMID *term,
                     const void *untilbuf);
static int r_forward_or(RSFD rfd, void *buf, TERMID *term,
                     const void *untilbuf);
static void r_pos (RSFD rfd, double *current, double *total);
static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm);

static const struct rset_control control_or = 
{
    "multi-or",
    r_delete,
    r_get_terms,
    r_open_or,
    r_close,
    r_forward_or,
    r_pos,
    r_read_or,
    r_write,
};

static const struct rset_control control_and = 
{
    "multi-and",
    r_delete,
    r_get_terms,
    r_open_and,
    r_close,
    r_forward_and,
    r_pos,
    r_read_and,
    r_write,
};

/* The heap structure: 
 * The rset contains a list or rsets we are ORing together 
 * The rfd contains a heap of heap-items, which contain
 * a rfd opened to those rsets, and a buffer for one key.
 * They also contain a ptr to the rset list in the rset 
 * itself, for practical reasons. 
 */

struct heap_item {
    RSFD fd;
    void *buf;
    RSET rset;
    TERMID term;
};

struct heap {
    int heapnum;
    int heapmax;
    const struct rset_key_control *kctrl;
    struct heap_item **heap; /* ptrs to the rfd */
};
typedef struct heap *HEAP;


struct rset_private {
    int dummy;
};


struct rfd_private {
    int flag;
    struct heap_item *items; /* we alloc and free them here */
    HEAP h; /* and move around here */
    zint hits; /* returned so far */
    int eof; /* seen the end of it */
    int tailcount; /* how many items are tailing */
    char *tailbits;
};

static int log_level = 0;
static int log_level_initialized = 0;


/* Heap functions ***********************/

#if 0
static void heap_dump_item( HEAP h, int i, int level)
{
    double cur,tot;
    if (i>h->heapnum)
        return;
    (void)rset_pos(h->heap[i]->rset,h->heap[i]->fd, &cur, &tot);
    yaz_log(log_level," %d %*s i=%p buf=%p %0.1f/%0.1f",i, level, "",  
                    &(h->heap[i]), h->heap[i]->buf, cur,tot );
    heap_dump_item(h, 2*i, level+1);
    heap_dump_item(h, 2*i+1, level+1);
}
static void heap_dump( HEAP h,char *msg) {
    yaz_log(log_level, "heap dump: %s num=%d max=%d",msg, h->heapnum, h->heapmax);
    heap_dump_item(h,1,1);
}
#endif

static void heap_swap (HEAP h, int x, int y)
{
    struct heap_item *swap;
    swap = h->heap[x];
    h->heap[x] = h->heap[y];
    h->heap[y] = swap;
}

static int heap_cmp(HEAP h, int x, int y)
{
    return (*h->kctrl->cmp)(h->heap[x]->buf,h->heap[y]->buf);
}

static int heap_empty(HEAP h)
{
    return ( 0==h->heapnum );
}

/** \brief deletes the first item in the heap, and balances the rest 
 */
static void heap_delete (HEAP h)
{
    int cur = 1, child = 2;
    h->heap[1] = 0; /* been deleted */
    heap_swap (h, 1, h->heapnum--);
    while (child <= h->heapnum) {
        if (child < h->heapnum && heap_cmp(h,child,1+child)>0 )
            child++;
        if (heap_cmp(h,cur,child) > 0)
        {
            heap_swap (h, cur, child);
            cur = child;
            child = 2*cur;
        }
        else
            break;
    }
}

/** \brief puts item into heap.
    The heap root element has changed value (to bigger) 
    Swap downwards until the heap is ordered again 
*/
static void heap_balance (HEAP h)
{
    int cur = 1, child = 2;
    while (child <= h->heapnum) {
        if (child < h->heapnum && heap_cmp(h,child,1+child)>0 )
            child++;
        if (heap_cmp(h,cur,child) > 0)
        {
            heap_swap (h, cur, child);
            cur = child;
            child = 2*cur;
        }
        else
            break;
    }
}


static void heap_insert (HEAP h, struct heap_item *hi)
{
    int cur, parent;

    cur = ++(h->heapnum);
    assert(cur <= h->heapmax);
    h->heap[cur] = hi;
    parent = cur/2;
    while (parent && (heap_cmp(h,parent,cur) > 0))
    {
        assert(parent>0);
        heap_swap (h, cur, parent);
        cur = parent;
        parent = cur/2;
    }
}


static
HEAP heap_create (NMEM nmem, int size, const struct rset_key_control *kctrl)
{
    HEAP h = (HEAP) nmem_malloc (nmem, sizeof(*h));

    ++size; /* heap array starts at 1 */
    h->heapnum = 0;
    h->heapmax = size;
    h->kctrl = kctrl;
    h->heap = (struct heap_item**) nmem_malloc(nmem,size*sizeof(*h->heap));
    h->heap[0]=0; /* not used */
    return h;
}

static void heap_clear( HEAP h)
{
    assert(h);
    h->heapnum = 0;
}

static void heap_destroy (HEAP h)
{
    /* nothing to delete, all is nmem'd, and will go away in due time */
}

/** \brief compare and items for quicksort
    used in qsort to get the multi-and args in optimal order
    that is, those with fewest occurrences first
*/
int compare_ands(const void *x, const void *y)
{   const struct heap_item *hx = x;
    const struct heap_item *hy = y;
    double cur, totx, toty;
    rset_pos(hx->fd, &cur, &totx);
    rset_pos(hy->fd, &cur, &toty);
    if ( totx > toty +0.5 )
	return 1;
    if ( totx < toty -0.5 )
	return -1;
    return 0;  /* return totx - toty, except for overflows and rounding */
}

static RSET rsmulti_andor_create(NMEM nmem,
				 struct rset_key_control *kcontrol, 
				 int scope, TERMID termid,
				 int no_rsets, RSET* rsets, 
				 const struct rset_control *ctrl)
{
    RSET rnew = rset_create_base(ctrl, nmem, kcontrol, scope, termid,
				 no_rsets, rsets);
    struct rset_private *info;
    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("rsmultiandor");
        log_level_initialized = 1;
    }
    yaz_log(log_level, "rsmultiand_andor_create scope=%d", scope);
    info = (struct rset_private *) nmem_malloc(rnew->nmem, sizeof(*info));
    rnew->priv = info;
    return rnew;
}

RSET rset_create_or(NMEM nmem, struct rset_key_control *kcontrol,
                    int scope, TERMID termid, int no_rsets, RSET* rsets)
{
    return rsmulti_andor_create(nmem, kcontrol, scope, termid,
                                no_rsets, rsets, &control_or);
}

RSET rset_create_and(NMEM nmem, struct rset_key_control *kcontrol,
                     int scope, int no_rsets, RSET* rsets)
{
    return rsmulti_andor_create(nmem, kcontrol, scope, 0,
                                no_rsets, rsets, &control_and);
}

static void r_delete (RSET ct)
{
}

static RSFD r_open_andor (RSET ct, int flag, int is_and)
{
    RSFD rfd;
    struct rfd_private *p;
    const struct rset_key_control *kctrl = ct->keycontrol;
    int i;

    if (flag & RSETF_WRITE)
    {
        yaz_log (YLOG_FATAL, "multiandor set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv) {
        p = (struct rfd_private *)rfd->priv;
        if (!is_and)
            heap_clear(p->h);
        assert(p->items);
        /* all other pointers shouls already be allocated, in right sizes! */
    }
    else 
    {
        p = (struct rfd_private *) nmem_malloc (ct->nmem,sizeof(*p));
        rfd->priv = p;
        p->h = 0;
        p->tailbits = 0;
        if (is_and)
            p->tailbits = nmem_malloc(ct->nmem, ct->no_children*sizeof(char) );
        else 
            p->h = heap_create( ct->nmem, ct->no_children, kctrl);
        p->items = (struct heap_item *) 
	    nmem_malloc(ct->nmem, ct->no_children*sizeof(*p->items));
        for (i = 0; i<ct->no_children; i++)
	{
            p->items[i].rset = ct->children[i];
            p->items[i].buf = nmem_malloc(ct->nmem, kctrl->key_size);
        }
    }
    p->flag = flag;
    p->hits = 0;
    p->eof = 0;
    p->tailcount = 0;
    if (is_and)
    { /* read the array and sort it */
        for (i = 0; i<ct->no_children; i++){
            p->items[i].fd = rset_open(ct->children[i], RSETF_READ);
            if (!rset_read(p->items[i].fd, p->items[i].buf, &p->items[i].term))
                p->eof = 1;
            p->tailbits[i] = 0;
        }
        qsort(p->items, ct->no_children, sizeof(p->items[0]), compare_ands);
    } 
    else
    { /* fill the heap for ORing */
        for (i = 0; i<ct->no_children; i++){
            p->items[i].fd = rset_open(ct->children[i],RSETF_READ);
            if ( rset_read(p->items[i].fd, p->items[i].buf, &p->items[i].term))
                heap_insert(p->h, &(p->items[i]));
        }
    }
    return rfd;
}

static RSFD r_open_or (RSET ct, int flag)
{
    return r_open_andor(ct, flag, 0);
}

static RSFD r_open_and (RSET ct, int flag)
{
    return r_open_andor(ct, flag, 1);
}


static void r_close (RSFD rfd)
{
    struct rfd_private *p=(struct rfd_private *)(rfd->priv);
    int i;

    if (p->h)
        heap_destroy (p->h);
    for (i = 0; i<rfd->rset->no_children; i++) 
        if (p->items[i].fd)
            rset_close(p->items[i].fd);
}

static int r_forward_or(RSFD rfd, void *buf, 
                        TERMID *term, const void *untilbuf)
{ /* while heap head behind untilbuf, forward it and rebalance heap */
    struct rfd_private *p = rfd->priv;
    const struct rset_key_control *kctrl = rfd->rset->keycontrol;
    if (heap_empty(p->h))
        return 0;
    while ( (*kctrl->cmp)(p->h->heap[1]->buf,untilbuf) < -rfd->rset->scope )
    {
        if (rset_forward(p->h->heap[1]->fd,p->h->heap[1]->buf,
                         &p->h->heap[1]->term, untilbuf))
            heap_balance(p->h);
        else 
        {
            heap_delete(p->h);
            if (heap_empty(p->h))
                return 0;
        }

    }
    return r_read_or(rfd, buf, term);
}


/** \brief reads one item key from an 'or' set
    \param rfd set handle
    \param buf resulting item buffer
    \param term resulting term
    \retval 0 EOF
    \retval 1 item could be read
*/
static int r_read_or (RSFD rfd, void *buf, TERMID *term)
{
    RSET rset = rfd->rset;
    struct rfd_private *mrfd = rfd->priv;
    const struct rset_key_control *kctrl = rset->keycontrol;
    struct heap_item *it;
    int rdres;
    if (heap_empty(mrfd->h))
        return 0;
    it = mrfd->h->heap[1];
    memcpy(buf, it->buf, kctrl->key_size);
    if (term)
    {
	if (rset->term)
	    *term = rset->term;
	else
	    *term = it->term;
	assert(*term);
    }
    (mrfd->hits)++;
    rdres = rset_read(it->fd, it->buf, &it->term);
    if ( rdres )
        heap_balance(mrfd->h);
    else
        heap_delete(mrfd->h);
    return 1;

}

/** \brief reads one item key from an 'and' set
    \param rfd set handle
    \param buf resulting item buffer
    \param term resulting term
    \retval 0 EOF
    \retval 1 item could be read
    
    Has to return all hits where each item points to the
    same sysno (scope), in order. Keep an extra key (hitkey)
    as long as all records do not point to hitkey, forward
    them, and update hitkey to be the highest seen so far.
    (if any item eof's, mark eof, and return 0 thereafter)
    Once a hit has been found, scan all items for the smallest
    value. Mark all as being in the tail. Read next from that
    item, and if not in the same record, clear its tail bit
*/
static int r_read_and (RSFD rfd, void *buf, TERMID *term)
{   struct rfd_private *p = rfd->priv;
    RSET ct = rfd->rset;
    const struct rset_key_control *kctrl = ct->keycontrol;
    int i, mintail;
    int cmp;

    while (1) {
        if (p->tailcount) 
        { /* we are tailing, find lowest tail and return it */
            mintail = 0;
            while ((mintail<ct->no_children) && !p->tailbits[mintail])
                mintail++; /* first tail */
            for (i = mintail+1; i<ct->no_children; i++)
            {
                if (p->tailbits[i])
                {
                    cmp=(*kctrl->cmp)(p->items[i].buf,p->items[mintail].buf);
                    if (cmp<0) 
                        mintail = i;
                }
            }
            /* return the lowest tail */
            memcpy(buf, p->items[mintail].buf, kctrl->key_size); 
            if (term)
                *term = p->items[mintail].term;
            if (!rset_read(p->items[mintail].fd, p->items[mintail].buf,
                           &p->items[mintail].term))
            {
                p->eof = 1; /* game over, once tails have been returned */
                p->tailbits[mintail] = 0; 
                (p->tailcount)--;
		(p->hits)++;
                return 1;
            }
            /* still a tail? */
            cmp = (*kctrl->cmp)(p->items[mintail].buf,buf);
            if (cmp >= rfd->rset->scope){
                p->tailbits[mintail] = 0;
                (p->tailcount)--;
            }
	    (p->hits)++;
            return 1;
        } 
        /* not tailing, forward until all reocrds match, and set up */
        /* as tails. the earlier 'if' will then return the hits */
        if (p->eof)
            return 0; /* nothing more to see */
        i = 1; /* assume items[0] is highest up */
        while (i<ct->no_children) {
            cmp = (*kctrl->cmp)(p->items[0].buf, p->items[i].buf);
            if (cmp <= -rfd->rset->scope) { /* [0] was behind, forward it */
                if (!rset_forward(p->items[0].fd, p->items[0].buf, 
                                  &p->items[0].term, p->items[i].buf))
                {
                    p->eof = 1; /* game over */
                    return 0;
                }
                i = 0; /* start frowarding from scratch */
            } 
            else if (cmp>=rfd->rset->scope)
            { /* [0] was ahead, forward i */
                if (!rset_forward(p->items[i].fd, p->items[i].buf, 
                                  &p->items[i].term, p->items[0].buf))
                {
                    p->eof = 1; /* game over */
                    return 0;
                }
            } 
            else
                i++;
        } /* while i */
        /* if we get this far, all rsets are now within +- scope of [0] */
        /* ergo, we have a hit. Mark them all as tailing, and let the */
        /* upper 'if' return the hits in right order */
        for (i = 0; i<ct->no_children; i++)
            p->tailbits[i] = 1;
        p->tailcount = ct->no_children;
    } /* while 1 */
}


static int r_forward_and(RSFD rfd, void *buf, TERMID *term, 
                         const void *untilbuf)
{ 
    struct rfd_private *p = rfd->priv;
    RSET ct = rfd->rset;
    const struct rset_key_control *kctrl = ct->keycontrol;
    int i;
    int cmp;
    int killtail = 0;

    for (i = 0; i<ct->no_children; i++)
    {
        cmp = (*kctrl->cmp)(p->items[i].buf,untilbuf);
        if (cmp <= -rfd->rset->scope)
        {
            killtail = 1; /* we are moving to a different hit */
            if (!rset_forward(p->items[i].fd, p->items[i].buf, 
                              &p->items[i].term, untilbuf))
            {
                p->eof = 1; /* game over */
                p->tailcount = 0;
                return 0;
            }
        }
    }
    if (killtail) 
    {
        for (i = 0; i<ct->no_children; i++)
            p->tailbits[i] = 0;
        p->tailcount = 0;
    }
    return r_read_and(rfd,buf,term);
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    RSET ct = rfd->rset;
    struct rfd_private *mrfd = 
	(struct rfd_private *)(rfd->priv);
    double cur, tot;
    double scur = 0.0, stot = 0.0;
    int i;
    for (i = 0; i<ct->no_children; i++){
        rset_pos(mrfd->items[i].fd, &cur, &tot);
        yaz_log(log_level, "r_pos: %d %0.1f %0.1f", i, cur,tot); 
        scur += cur;
        stot += tot;
    }
    if (stot < 1.0) { /* nothing there */
        *current = 0;
        *total = 0;
        yaz_log(log_level, "r_pos: NULL  %0.1f %0.1f",  *current, *total);
    }
    else
    {
	*current = (double) (mrfd->hits);
	*total = *current*stot/scur;
	yaz_log(log_level, "r_pos: =  %0.1f %0.1f",  *current, *total);
    }
}

static int r_write (RSFD rfd, const void *buf)
{
    yaz_log (YLOG_FATAL, "multior set type is read-only");
    return -1;
}

static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm)
{
    if (ct->term)
	rset_get_one_term(ct, terms, maxterms, curterm);
    else
    {
	/* Special case: Some multi-ors have all terms pointing to the same 
	   term. We do not want to duplicate those. Other multiors (and ands)
	   have different terms under them. Those we want. 
	*/
	int firstterm= *curterm;
	int i;

 	for (i = 0; i<ct->no_children; i++)
	{
	    rset_getterms(ct->children[i], terms, maxterms, curterm);
	    if ( ( *curterm > firstterm+1 ) &&
		 ( *curterm <= maxterms ) &&
		 ( terms[(*curterm)-1] == terms[firstterm] ) 
		)
		(*curterm)--; /* forget the term, seen that before */
	}
    }
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

