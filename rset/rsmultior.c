/* $Id: rsmultior.c,v 1.2 2004-08-19 12:49:15 heikki Exp $
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




#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zebrautl.h>
#include <isamc.h>
#include <rsmultior.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_read (RSFD rfd, void *buf, int *term_index);
static int r_write (RSFD rfd, const void *buf);
static int r_forward(RSET ct, RSFD rfd, void *buf,  int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);

static const struct rset_control control = 
{
    "multi-or",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_forward,
    rset_default_pos, /* FIXME */
    r_read,
    r_write,
};

const struct rset_control *rset_kind_multior = &control;

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
};

struct heap {
    int heapnum;
    int heapmax;
    int keysize;
    int     (*cmp)(const void *p1, const void *p2);
    struct heap_item **heap; /* ptrs to the rfd */
};
typedef struct heap *HEAP;


struct rset_multior_info {
    int     key_size;
    int     no_rec;
    int     (*cmp)(const void *p1, const void *p2);
    int     no_rsets;
    RSET    *rsets;
    int     term_index;
    struct rset_multior_rfd *rfd_list;
};


struct rset_multior_rfd {
    int flag;
    struct heap_item *items; /* we alloc and free them here */
    HEAP h;
    struct rset_multior_rfd *next;
    struct rset_multior_info *info;
    zint *countp;
    char *prevvalue; /* to see if we are in another record */
};

#if 0
static void heap_dump_item( HEAP h, int i, int level) {
    double cur,tot;
    if (i>h->heapnum)
        return;
    (void)rset_pos(h->heap[i]->rset,h->heap[i]->fd, &cur, &tot);
    logf(LOG_LOG," %d %*s i=%p buf=%p %0.1f/%0.1f",i, level, "",  
                    &(h->heap[i]), h->heap[i]->buf, cur,tot );
    heap_dump_item(h, 2*i, level+1);
    heap_dump_item(h, 2*i+1, level+1);
}
static void heap_dump( HEAP h,char *msg) {
    logf(LOG_LOG, "heap dump: %s num=%d max=%d",msg, h->heapnum, h->heapmax);
    heap_dump_item(h,1,1);
}
#endif

static void heap_swap (HEAP h, int x, int y)
{
    struct heap_item *swap;
    swap = h->heap[x];
    h->heap[x]=h->heap[y];
    h->heap[y]=swap;
}

static int heap_cmp(HEAP h, int x, int y)
{
    return (*h->cmp)(h->heap[x]->buf,h->heap[y]->buf);
}

static int heap_empty(HEAP h)
{
    return ( 0==h->heapnum );
}

static void heap_delete (HEAP h)
{ /* deletes the first item in the heap, and balances the rest */
    int cur = 1, child = 2;
    h->heap[1]=0; /* been deleted */
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

static void heap_balance (HEAP h)
{ /* The heap root element has changed value (to bigger) */
  /* swap downwards until the heap is ordered again */
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
    h->heap[cur]=hi;
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
HEAP heap_create (int size, int key_size,
      int (*cmp)(const void *p1, const void *p2))
{
    HEAP h = (HEAP) xmalloc (sizeof(*h));

    ++size; /* heap array starts at 1 */
    h->heapnum = 0;
    h->heapmax = size;
    h->keysize = key_size;
    h->cmp = cmp;
    h->heap = (struct heap_item**) xmalloc((size)*sizeof(*h->heap));
    h->heap[0]=0; /* not used */
    return h;
}

static void heap_destroy (HEAP h)
{
    xfree (h->heap); /* safe, they all point to the rfd */
    xfree (h);
}


static void *r_create (RSET ct, const struct rset_control *sel, void *parms)
{
    rset_multior_parms *r_parms = (rset_multior_parms *) parms;
    struct rset_multior_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
        /* FIXME - Remove the whole flags thing, from all rsets */
    info = (struct rset_multior_info *) xmalloc (sizeof(*info));
    info->key_size = r_parms->key_size;
    assert (info->key_size > 1);
    info->cmp = r_parms->cmp;
    info->no_rsets= r_parms->no_rsets;
    info->rsets=r_parms->rsets; /* now we own it! */
    info->rfd_list=0;
    info->term_index=0 ; /* r_parms->rset_term; */ /*??*/ /*FIXME */
    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *) xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = r_parms->rset_term;
    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_multior_rfd *rfd;
    struct rset_multior_info *info = (struct rset_multior_info *) ct->buf;
    int i;
    int dummy_termindex;

    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "multior set type is read-only");
        return NULL;
    }
    rfd = (struct rset_multior_rfd *) xmalloc (sizeof(*rfd));
    rfd->flag = flag;
    rfd->next = info->rfd_list;
    rfd->info = info;
    info->rfd_list = rfd;
    if (ct->no_rset_terms==1)
        rfd->countp=&ct->rset_terms[0]->count;
    else
        rfd->countp=0;
    rfd->h = heap_create( info->no_rsets, info->key_size, info->cmp);
    rfd->prevvalue=0;
    rfd->items=(struct heap_item *) xmalloc(info->no_rsets*sizeof(*rfd->items));
    for (i=0; i<info->no_rsets; i++){
       rfd->items[i].rset=info->rsets[i];
       rfd->items[i].buf=xmalloc(info->key_size);
       rfd->items[i].fd=rset_open(info->rsets[i],RSETF_READ);
/*       if (item_readbuf(&(rfd->items[i]))) */
       if ( rset_read(rfd->items[i].rset, rfd->items[i].fd, 
                      rfd->items[i].buf, &dummy_termindex) )
           heap_insert(rfd->h, &(rfd->items[i]));
    }
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_multior_rfd *mrfd = (struct rset_multior_rfd *) rfd;
    struct rset_multior_info *info = mrfd->info;
    struct rset_multior_rfd **rfdp;
    int i;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            *rfdp = (*rfdp)->next;
        
            heap_destroy (mrfd->h);
            for (i = 0; i<info->no_rsets; i++) {
                if (mrfd->items[i].fd)
                    rset_close(info->rsets[i],mrfd->items[i].fd);
                xfree(mrfd->items[i].buf);
            }
            xfree(mrfd->items);
            if (mrfd->prevvalue)
                xfree(mrfd->prevvalue);
            xfree(mrfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_multior_info *info = (struct rset_multior_info *) ct->buf;
    int i;

    assert (info->rfd_list == NULL);
    for(i=0;i<info->no_rsets;i++)
        rset_delete(info->rsets[i]);
    xfree(info->rsets);
    xfree(info);

    for (i = 0; i<ct->no_rset_terms; i++) /* usually only 1 */
        rset_term_destroy (ct->rset_terms[i]);  
    xfree (ct->rset_terms);
}

static void r_rewind (RSFD rfd)
{
    assert(!"rewind not implemented yet");
}


static int r_forward(RSET ct, RSFD rfd, void *buf,  int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf)
{
    struct rset_multior_rfd *mrfd = (struct rset_multior_rfd *) rfd;
    struct rset_multior_info *info = mrfd->info;
    struct heap_item it;
    int rdres;
    int dummycount=0;
    if (heap_empty(mrfd->h))
        return 0;
    if (cmpfunc)
        assert(cmpfunc==mrfd->info->cmp);
    *term_index=0;
    it = *(mrfd->h->heap[1]);
    memcpy(buf,it.buf, info->key_size); 
    if (mrfd->countp) {
        if (mrfd->prevvalue) {  /* in another record */
            if ( (*mrfd->info->cmp)(mrfd->prevvalue,it.buf) < -1)
                (*mrfd->countp)++; 
        } else {
            mrfd->prevvalue=xmalloc(info->key_size);
            (*mrfd->countp)++; 
        } 
        memcpy(mrfd->prevvalue,it.buf, info->key_size); 
    }
    if (untilbuf)
        rdres=rset_forward(it.rset, it.fd, it.buf, &dummycount,
                           cmpfunc,untilbuf);
    else
        rdres=rset_read(it.rset, it.fd, it.buf, &dummycount);
    if ( rdres )
        heap_balance(mrfd->h);
    else
        heap_delete(mrfd->h);
    return 1;

}

static int r_read (RSFD rfd, void *buf, int *term_index)
{
    return r_forward(0,rfd, buf, term_index,0,0);
}

#if 0
static int old_read
{
    struct rset_multior_rfd *mrfd = (struct rset_multior_rfd *) rfd;
    struct trunc_info *ti = mrfd->ti;
    int n = ti->indx[ti->ptr[1]];

    if (!ti->heapnum)
        return 0;
    *term_index = 0;
    memcpy (buf, ti->heap[ti->ptr[1]], ti->keysize);
    if (((struct rset_multior_rfd *) rfd)->position)
    {
        if (isc_pp_read (((struct rset_multior_rfd *) rfd)->ispt[n], ti->tmpbuf))
        {
            heap_delete (ti);
            if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
                 ((struct rset_multior_rfd *) rfd)->position--;
            heap_insert (ti, ti->tmpbuf, n);
        }
        else
            heap_delete (ti);
        if (mrfd->countp && (
                *mrfd->countp == 0 || (*ti->cmp)(buf, mrfd->pbuf) > 1))
        {
            memcpy (mrfd->pbuf, buf, ti->keysize);
            (*mrfd->countp)++;
        }
        return 1;
    }
    while (1)
    {
        if (!isc_pp_read (((struct rset_multior_rfd *) rfd)->ispt[n], ti->tmpbuf))
        {
            heap_delete (ti);
            break;
        }
        if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
        {
            heap_delete (ti);
            heap_insert (ti, ti->tmpbuf, n);
            break;
        }
    }
    if (mrfd->countp && (
            *mrfd->countp == 0 || (*ti->cmp)(buf, mrfd->pbuf) > 1))
    {
        memcpy (mrfd->pbuf, buf, ti->keysize);
        (*mrfd->countp)++;
    }
    return 1;
}

#endif
/*!*/ /* FIXME Forward */
/*!*/ /* FIXME pos */

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "multior set type is read-only");
    return -1;
}
