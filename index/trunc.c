/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: trunc.c,v $
 * Revision 1.1  1996-11-04 14:07:40  adam
 * Moved truncation code to trunc.c.
 *
 */
#include <stdio.h>
#include <assert.h>

#include "zserver.h"
#include <rstemp.h>
#include <rsisam.h>
#include <rsisamc.h>
#include <rsnull.h>

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

static RSET rset_trunc_r (ZServerInfo *zi, ISAM_P *isam_p, int from, int to,
                         int merge_chunk)
{
    RSET result; 
    RSFD result_rsfd;
    rset_temp_parms parms;

    parms.key_size = sizeof(struct it_key);
    result = rset_create (rset_kind_temp, &parms);
    result_rsfd = rset_open (result, RSETF_WRITE|RSETF_SORT_SYSNO);

    if (to - from > merge_chunk)
    {
        RSFD *rsfd;
        RSET *rset;
        int i, i_add = (to-from)/merge_chunk + 1;
        struct trunc_info *ti;
        int rscur = 0;
        int rsmax = (to-from)/i_add + 1;
        
        rset = xmalloc (sizeof(*rset) * rsmax);
        rsfd = xmalloc (sizeof(*rsfd) * rsmax);
        
        for (i = from; i < to; i += i_add)
        {
            if (i_add <= to - i)
                rset[rscur] = rset_trunc_r (zi, isam_p, i, i+i_add,
                                            merge_chunk);
            else
                rset[rscur] = rset_trunc_r (zi, isam_p, i, to,
                                            merge_chunk);
            rscur++;
        }
        ti = heap_init (rscur, sizeof(struct it_key), key_compare);
        for (i = rscur; --i >= 0; )
        {
            rsfd[i] = rset_open (rset[i], RSETF_READ|RSETF_SORT_SYSNO);
            if (rset_read (rset[i], rsfd[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
            {
                rset_close (rset[i], rsfd[i]);
                rset_delete (rset[i]);
            }
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);

            while (1)
            {
                if (!rset_read (rset[n], rsfd[n], ti->tmpbuf))
                {
                    heap_delete (ti);
                    rset_close (rset[n], rsfd[n]);
                    rset_delete (rset[n]);
                    break;
                }
                if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
                {
                    heap_delete (ti);
                    heap_insert (ti, ti->tmpbuf, n);
                    break;
                }
            }
        }
        xfree (rset);
        xfree (rsfd);
        heap_close (ti);
    }
    else if (zi->isam)
    {
        ISPT *ispt;
        int i;
        struct trunc_info *ti;

        ispt = xmalloc (sizeof(*ispt) * (to-from));

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = is_position (zi->isam, isam_p[from+i]);
            if (is_readkey (ispt[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
                is_pt_free (ispt[i]);
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);
#if 0
/* section that preserve all keys */
            heap_delete (ti);
            if (is_readkey (ispt[n], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, n);
            else
                is_pt_free (ispt[n]);
#else
/* section that preserve all keys with unique sysnos */
            while (1)
            {
                if (!is_readkey (ispt[n], ti->tmpbuf))
                {
                    heap_delete (ti);
                    is_pt_free (ispt[n]);
                    break;
                }
                if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
                {
                    heap_delete (ti);
                    heap_insert (ti, ti->tmpbuf, n);
                    break;
                }
            }
#endif
        }
        heap_close (ti);
        xfree (ispt);
    }
    else
    {
        ISAMC_PP *ispt;
        int i;
        struct trunc_info *ti;

        ispt = xmalloc (sizeof(*ispt) * (to-from));

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = isc_pp_open (zi->isamc, isam_p[from+i]);
            if (isc_read_key (ispt[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
                isc_pp_close (ispt[i]);
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);
#if 0
/* section that preserve all keys */
            heap_delete (ti);
            if (is_readkey (ispt[n], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, n);
            else
                isc_pp_close (ispt[n]);
#else
/* section that preserve all keys with unique sysnos */
            while (1)
            {
                if (!isc_read_key (ispt[n], ti->tmpbuf))
                {
                    heap_delete (ti);
                    isc_pp_close (ispt[n]);
                    break;
                }
                if ((*ti->cmp)(ti->tmpbuf, ti->heap[ti->ptr[1]]) > 1)
                {
                    heap_delete (ti);
                    heap_insert (ti, ti->tmpbuf, n);
                    break;
                }
            }
#endif
        }
        heap_close (ti);
        xfree (ispt);
    }
    rset_close (result, result_rsfd);
    return result;
}

static int isam_trunc_cmp (const void *p1, const void *p2)
{
    ISAM_P i1 = *(ISAM_P*) p1;
    ISAM_P i2 = *(ISAM_P*) p2;
    int d;

    d = is_type (i1) - is_type (i2);
    if (d)
        return d;
    return is_block (i1) - is_block (i2);
}

static int isamc_trunc_cmp (const void *p1, const void *p2)
{
    ISAMC_P i1 = *(ISAMC_P*) p1;
    ISAMC_P i2 = *(ISAMC_P*) p2;
    int d;

    d = isc_type (i1) - isc_type (i2);
    if (d)
        return d;
    return isc_block (i1) - isc_block (i2);
}

RSET rset_trunc (ZServerInfo *zi, ISAM_P *isam_p, int no)
{
    if (zi->isam)
    {
        if (no < 1)
            return rset_create (rset_kind_null, NULL);
        else if (no == 1)
        {
            rset_isam_parms parms;

            parms.pos = *isam_p;
            parms.is = zi->isam;
            return rset_create (rset_kind_isam, &parms);
        }
        qsort (isam_p, no, sizeof(*isam_p), isam_trunc_cmp);
    }
    else if (zi->isamc)
    {
        if (no < 1)
            return rset_create (rset_kind_null, NULL);
        else if (no == 1)
        {
            rset_isamc_parms parms;

            parms.pos = *isam_p;
            parms.is = zi->isamc;
            return rset_create (rset_kind_isamc, &parms);
        }
        qsort (isam_p, no, sizeof(*isam_p), isamc_trunc_cmp);
    }
    else
        logf (LOG_FATAL, "Neither isam nor isamc set in rset_trunc");
    return rset_trunc_r (zi, isam_p, 0, no, 100);
}

