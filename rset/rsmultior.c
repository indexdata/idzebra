/* $Id: rsmultior.c,v 1.7 2004-08-26 11:11:59 heikki Exp $
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

static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);
static int r_forward(RSET ct, RSFD rfd, void *buf,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);
static void r_pos (RSFD rfd, double *current, double *total);

static const struct rset_control control = 
{
    "multi-or",
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_forward,
    r_pos,
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
    struct rset_multior_rfd *rfd_list; /* rfds in use */
    struct rset_multior_rfd *free_list; /* rfds free */
        /* rfds are allocated when first opened, if nothing in freelist */
        /* when allocated, all their pointers are allocated as well, and */
        /* those are kept intact when in the freelist, ready to be reused */
};


struct rset_multior_rfd {
    int flag;
    struct heap_item *items; /* we alloc and free them here */
    HEAP h;
    struct rset_multior_rfd *next;
    struct rset_multior_info *info;
    zint hits; /* returned so far */
    char *prevvalue; /* to see if we are in another record */
      /* FIXME - is this really needed? */
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
HEAP heap_create (NMEM nmem, int size, int key_size,
      int (*cmp)(const void *p1, const void *p2))
{
    HEAP h = (HEAP) nmem_malloc (nmem, sizeof(*h));

    ++size; /* heap array starts at 1 */
    h->heapnum = 0;
    h->heapmax = size;
    h->keysize = key_size;
    h->cmp = cmp;
    h->heap = (struct heap_item**) nmem_malloc(nmem,(size)*sizeof(*h->heap));
    h->heap[0]=0; /* not used */
    return h;
}

static void heap_clear( HEAP h)
{
    assert(h);
    h->heapnum=0;
}

static void heap_destroy (HEAP h)
{
    /* nothing to delete, all is nmem'd, and will go away in due time */
}


RSET rsmultior_create( NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            int no_rsets, RSET* rsets)
{
    RSET rnew=rset_create_base(&control, nmem);
    struct rset_multior_info *info;
    info = (struct rset_multior_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->key_size = key_size;
    info->cmp = cmp;
    info->no_rsets=no_rsets;
    info->rsets=(RSET*)nmem_malloc(rnew->nmem, no_rsets*sizeof(*rsets));
    memcpy(info->rsets,rsets,no_rsets*sizeof(*rsets));
    info->rfd_list = NULL;
    info->free_list = NULL;
    rnew->priv=info;
    return rnew;
}

static void r_delete (RSET ct)
{
    struct rset_multior_info *info = (struct rset_multior_info *) ct->priv;
    int i;

    assert (info->rfd_list == NULL);
    for(i=0;i<info->no_rsets;i++)
        rset_delete(info->rsets[i]);
}
#if 0
static void *r_create (RSET ct, const struct rset_control *sel, void *parms)
{
    rset_multior_parms *r_parms = (rset_multior_parms *) parms;
    struct rset_multior_info *info;
    info = (struct rset_multior_info *) xmalloc (sizeof(*info));
    info->key_size = r_parms->key_size;
    assert (info->key_size > 1);
    info->cmp = r_parms->cmp;
    info->no_rsets= r_parms->no_rsets;
    info->rsets=r_parms->rsets; /* now we own it! */
    info->rfd_list=0;
    return info;
}
#endif

static RSFD r_open (RSET ct, int flag)
{
    struct rset_multior_rfd *rfd;
    struct rset_multior_info *info = (struct rset_multior_info *) ct->priv;
    int i;

    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "multior set type is read-only");
        return NULL;
    }
    rfd=info->free_list;
    if (rfd) {
        info->free_list=rfd->next;
        heap_clear(rfd->h);
        assert(rfd->items);
        /* all other pointers shouls already be allocated, in right sizes! */
    }
    else {
        rfd = (struct rset_multior_rfd *) nmem_malloc (ct->nmem,sizeof(*rfd));
        rfd->h = heap_create( ct->nmem, info->no_rsets, 
                              info->key_size, info->cmp);
        rfd->items=(struct heap_item *) nmem_malloc(ct->nmem,
                              info->no_rsets*sizeof(*rfd->items));
        for (i=0; i<info->no_rsets; i++){
            rfd->items[i].rset=info->rsets[i];
            rfd->items[i].buf=nmem_malloc(ct->nmem,info->key_size);
        }
    }
    rfd->flag = flag;
    rfd->next = info->rfd_list;
    rfd->info = info;
    info->rfd_list = rfd;
    rfd->prevvalue=0;
    rfd->hits=0;
    for (i=0; i<info->no_rsets; i++){
        rfd->items[i].fd=rset_open(info->rsets[i],RSETF_READ);
        if ( rset_read(rfd->items[i].rset, rfd->items[i].fd, 
                      rfd->items[i].buf) )
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
            }
            mrfd->next=info->free_list;
            info->free_list=mrfd;
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}


static void r_rewind (RSFD rfd)
{
    assert(!"rewind not implemented yet");
}


static int r_forward(RSET ct, RSFD rfd, void *buf,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf)
{
    struct rset_multior_rfd *mrfd = (struct rset_multior_rfd *) rfd;
    struct rset_multior_info *info = mrfd->info;
    struct heap_item it;
    int rdres;
    if (heap_empty(mrfd->h))
        return 0;
    if (cmpfunc)
        assert(cmpfunc==mrfd->info->cmp);
    it = *(mrfd->h->heap[1]);
    memcpy(buf,it.buf, info->key_size); 
    (mrfd->hits)++;
    if (untilbuf)
        rdres=rset_forward(it.rset, it.fd, it.buf, cmpfunc,untilbuf);
    else
        rdres=rset_read(it.rset, it.fd, it.buf);
    if ( rdres )
        heap_balance(mrfd->h);
    else
        heap_delete(mrfd->h);
    return 1;

}

static int r_read (RSFD rfd, void *buf)
{
    return r_forward(0,rfd, buf,0,0);
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    struct rset_multior_rfd *mrfd = (struct rset_multior_rfd *) rfd;
    struct rset_multior_info *info = mrfd->info;
    double cur, tot;
    double scur=0.0, stot=0.0;
    int i;
    for (i=0; i<info->no_rsets; i++){
        rset_pos(mrfd->items[i].rset, mrfd->items[i].fd, &cur, &tot);
        logf(LOG_LOG, "r_pos: %d %0.1f %0.1f", i, cur,tot);
        scur += cur;
        stot += tot;
    }
    if (stot <1.0) { /* nothing there */
        *current=0;
        *total=0;
        return;
    }
    *current=mrfd->hits;
    *total=*current*stot/scur;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "multior set type is read-only");
    return -1;
}
