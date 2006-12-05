/* $Id: rsm_or.c,v 1.16.2.2 2006-12-05 21:14:45 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/




#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zebrautl.h>
#include <isam.h>
#include <isamc.h>
#include <rsm_or.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_read (RSFD rfd, void *buf, int *term_index);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control = 
{
    "multi-or",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    rset_default_forward, /* FIXME */
    rset_default_pos, /* FIXME */
    r_read,
    r_write,
};

const struct rset_control *rset_kind_m_or = &control;

struct rset_mor_info {
    int     key_size;
    int     no_rec;
    int     (*cmp)(const void *p1, const void *p2);
    ISAMC   isc;
    ISAM_P  *isam_positions;

    int     no_isam_positions;
    int     no_save_positions;
    struct rset_mor_rfd *rfd_list;
};

struct trunc_info {
    int  *ptr;
    int  *indx;
    char **heap;
    int  heapnum;
    int  (*cmp)(const void *p1, const void *p2);
    int  keysize;
    char *swapbuf;
    char *tmpbuf;
    char *buf;
};

struct rset_mor_rfd {
    int flag;
    int position;
    int position_max;
    ISAMC_PP *ispt;
    struct rset_mor_rfd *next;
    struct rset_mor_info *info;
    struct trunc_info *ti;
    int  *countp;
    char *pbuf;
};

static void heap_swap (struct trunc_info *ti, int i1, int i2)
{
    int swap;

    swap = ti->ptr[i1];
    ti->ptr[i1] = ti->ptr[i2];
    ti->ptr[i2] = swap;
}

static void heap_delete (struct trunc_info *ti)
{
    int cur = 1, child = 2;

    heap_swap (ti, 1, ti->heapnum--);
    while (child <= ti->heapnum) {
        if (child < ti->heapnum &&
            (*ti->cmp)(ti->heap[ti->ptr[child]],
                       ti->heap[ti->ptr[1+child]]) > 0)
            child++;
        if ((*ti->cmp)(ti->heap[ti->ptr[cur]],
                       ti->heap[ti->ptr[child]]) > 0)
        {
            heap_swap (ti, cur, child);
            cur = child;
            child = 2*cur;
        }
        else
            break;
    }
}

static void heap_insert (struct trunc_info *ti, const char *buf, int indx)
{
    int cur, parent;

    cur = ++(ti->heapnum);
    memcpy (ti->heap[ti->ptr[cur]], buf, ti->keysize);
    ti->indx[ti->ptr[cur]] = indx;
    parent = cur/2;
    while (parent && (*ti->cmp)(ti->heap[ti->ptr[parent]],
                                ti->heap[ti->ptr[cur]]) > 0)
    {
        heap_swap (ti, cur, parent);
        cur = parent;
        parent = cur/2;
    }
}

static
struct trunc_info *heap_init (int size, int key_size,
                              int (*cmp)(const void *p1, const void *p2))
{
    struct trunc_info *ti = (struct trunc_info *) xmalloc (sizeof(*ti));
    int i;

    ++size;
    ti->heapnum = 0;
    ti->keysize = key_size;
    ti->cmp = cmp;
    ti->indx = (int *) xmalloc (size * sizeof(*ti->indx));
    ti->heap = (char **) xmalloc (size * sizeof(*ti->heap));
    ti->ptr = (int *) xmalloc (size * sizeof(*ti->ptr));
    ti->swapbuf = (char *) xmalloc (ti->keysize);
    ti->tmpbuf = (char *) xmalloc (ti->keysize);
    ti->buf = (char *) xmalloc (size * ti->keysize);
    for (i = size; --i >= 0; )
    {
        ti->ptr[i] = i;
        ti->heap[i] = ti->buf + ti->keysize * i;
    }
    return ti;
}

static void heap_close (struct trunc_info *ti)
{
    xfree (ti->buf);
    xfree (ti->ptr);
    xfree (ti->indx);
    xfree (ti->heap);
    xfree (ti->swapbuf);
    xfree (ti->tmpbuf);
    xfree (ti);
}

static void *r_create (RSET ct, const struct rset_control *sel, void *parms)
{
    rset_m_or_parms *r_parms = (rset_m_or_parms *) parms;
    struct rset_mor_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
    info = (struct rset_mor_info *) xmalloc (sizeof(*info));
    info->key_size = r_parms->key_size;
    assert (info->key_size > 1);
    info->cmp = r_parms->cmp;
    
    info->no_save_positions = r_parms->no_save_positions;

    info->isc = r_parms->isc;
    info->no_isam_positions = r_parms->no_isam_positions;
    info->isam_positions = (ISAM_P *)
	xmalloc (sizeof(*info->isam_positions) * info->no_isam_positions);
    memcpy (info->isam_positions, r_parms->isam_positions,
            sizeof(*info->isam_positions) * info->no_isam_positions);
    info->rfd_list = NULL;

    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *) xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = r_parms->rset_term;
    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_mor_rfd *rfd;
    struct rset_mor_info *info = (struct rset_mor_info *) ct->buf;
    int i;

    if (flag & RSETF_WRITE)
    {
	yaz_log(YLOG_FATAL, "m_or set type is read-only");
	return NULL;
    }
    rfd = (struct rset_mor_rfd *) xmalloc (sizeof(*rfd));
    rfd->flag = flag;
    rfd->next = info->rfd_list;
    rfd->info = info;
    info->rfd_list = rfd;

    rfd->ispt = (ISAMC_PP *)
	xmalloc (sizeof(*rfd->ispt) * info->no_isam_positions);
        
    rfd->ti = heap_init (info->no_isam_positions, info->key_size, info->cmp);

    ct->rset_terms[0]->nn = 0;
    for (i = 0; i<info->no_isam_positions; i++)
    {
        rfd->ispt[i] = isc_pp_open (info->isc, info->isam_positions[i]);
	
	ct->rset_terms[0]->nn += isc_pp_num (rfd->ispt[i]);

        if (isc_pp_read (rfd->ispt[i], rfd->ti->tmpbuf))
            heap_insert (rfd->ti, rfd->ti->tmpbuf, i);
        else
        {
            isc_pp_close (rfd->ispt[i]);
            rfd->ispt[i] = NULL;
        }
    }
    rfd->position = info->no_save_positions;

    if (ct->no_rset_terms == 1)
        rfd->countp = &ct->rset_terms[0]->count;
    else
        rfd->countp = 0;
    rfd->pbuf = xmalloc (info->key_size);

    r_rewind (rfd);
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_mor_info *info = ((struct rset_mor_rfd*)rfd)->info;
    struct rset_mor_rfd **rfdp;
    int i;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            *rfdp = (*rfdp)->next;
        
            heap_close (((struct rset_mor_rfd *) rfd)->ti);
            for (i = 0; i<info->no_isam_positions; i++)
                if (((struct rset_mor_rfd *) rfd)->ispt[i])
                    isc_pp_close (((struct rset_mor_rfd *) rfd)->ispt[i]);
            xfree (((struct rset_mor_rfd *)rfd)->ispt);
            xfree (((struct rset_mor_rfd *)rfd)->pbuf);
            xfree (rfd);
            return;
        }
    yaz_log(YLOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_mor_info *info = (struct rset_mor_info *) ct->buf;
    int i;

    assert (info->rfd_list == NULL);
    xfree (info->isam_positions);

    for (i = 0; i<ct->no_rset_terms; i++)
	rset_term_destroy (ct->rset_terms[i]);
    xfree (ct->rset_terms);

    xfree (info);
}

static void r_rewind (RSFD rfd)
{
}


static int r_read (RSFD rfd, void *buf, int *term_index)
{
    struct rset_mor_rfd *mrfd = (struct rset_mor_rfd *) rfd;
    struct trunc_info *ti = mrfd->ti;
    int n = ti->indx[ti->ptr[1]];

    if (!ti->heapnum)
        return 0;
    *term_index = 0;
    memcpy (buf, ti->heap[ti->ptr[1]], ti->keysize);
    if (((struct rset_mor_rfd *) rfd)->position)
    {
        if (isc_pp_read (((struct rset_mor_rfd *) rfd)->ispt[n], ti->tmpbuf))
        {
            heap_delete (ti);
            if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
                 ((struct rset_mor_rfd *) rfd)->position--;
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
        if (!isc_pp_read (((struct rset_mor_rfd *) rfd)->ispt[n], ti->tmpbuf))
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

static int r_write (RSFD rfd, const void *buf)
{
    yaz_log(YLOG_FATAL, "mor set type is read-only");
    return -1;
}
