/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsm_or.c,v $
 * Revision 1.6  1998-03-05 08:36:28  adam
 * New result set model.
 *
 * Revision 1.5  1997/12/18 10:54:25  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.4  1997/10/31 12:37:55  adam
 * Code calls xfree() instead of free().
 *
 * Revision 1.3  1997/09/09 13:38:16  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.2  1996/12/23 15:30:49  adam
 * Work on truncation.
 *
 * Revision 1.1  1996/12/20 11:07:21  adam
 * Implemented Multi-or result set.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isam.h>
#include <isamc.h>
#include <rsm_or.h>
#include <zebrautl.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_count (RSET ct);
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
    r_count,
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
    struct trunc_info *ti = xmalloc (sizeof(*ti));
    int i;

    ++size;
    ti->heapnum = 0;
    ti->keysize = key_size;
    ti->cmp = cmp;
    ti->indx = xmalloc (size * sizeof(*ti->indx));
    ti->heap = xmalloc (size * sizeof(*ti->heap));
    ti->ptr = xmalloc (size * sizeof(*ti->ptr));
    ti->swapbuf = xmalloc (ti->keysize);
    ti->tmpbuf = xmalloc (ti->keysize);
    ti->buf = xmalloc (size * ti->keysize);
    for (i = size; --i >= 0; )
    {
        ti->ptr[i] = i;
        ti->heap[i] = ti->buf + ti->keysize * i;
    }
    return ti;
}

static void heap_close (struct trunc_info *ti)
{
    xfree (ti->ptr);
    xfree (ti->indx);
    xfree (ti->heap);
    xfree (ti->swapbuf);
    xfree (ti->tmpbuf);
    xfree (ti);
}

static void *r_create (RSET ct, const struct rset_control *sel, void *parms)
{
    rset_m_or_parms *r_parms = parms;
    struct rset_mor_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
    info = xmalloc (sizeof(*info));
    info->key_size = r_parms->key_size;
    assert (info->key_size > 1);
    info->cmp = r_parms->cmp;
    
    info->no_save_positions = r_parms->no_save_positions;

    info->isc = r_parms->isc;
    info->no_isam_positions = r_parms->no_isam_positions;
    info->isam_positions = xmalloc (sizeof(*info->isam_positions) *
                                    info->no_isam_positions);
    memcpy (info->isam_positions, r_parms->isam_positions,
            sizeof(*info->isam_positions) * info->no_isam_positions);
    info->rfd_list = NULL;

    ct->no_rset_terms = 1;
    ct->rset_terms = xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = rset_term_dup (r_parms->rset_term);
    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_mor_rfd *rfd;
    struct rset_mor_info *info = ct->buf;
    int i;

    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "m_or set type is read-only");
	return NULL;
    }
    rfd = xmalloc (sizeof(*rfd));
    rfd->flag = flag;
    rfd->next = info->rfd_list;
    rfd->info = info;
    info->rfd_list = rfd;

    rfd->ispt = xmalloc (sizeof(*rfd->ispt) * info->no_isam_positions);
        
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
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_mor_info *info = ct->buf;
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

static int r_count (RSET ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf, int *term_index)
{
    struct trunc_info *ti = ((struct rset_mor_rfd *) rfd)->ti;
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
    return 1;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "mor set type is read-only");
    return -1;
}
