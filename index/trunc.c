/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: trunc.c,v $
 * Revision 1.19  2001-01-16 16:56:15  heikki
 * Searching in my isam-d
 *
 * Revision 1.18  2000/05/18 12:01:36  adam
 * System call times(2) used again. More 64-bit fixes.
 *
 * Revision 1.17  2000/03/15 15:00:30  adam
 * First work on threaded version.
 *
 * Revision 1.16  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.15  1999/07/20 13:59:18  adam
 * Fixed bug that occurred when phrases had 0 hits.
 *
 * Revision 1.14  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.13  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.12  1999/02/02 14:51:10  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.11  1998/03/25 13:48:02  adam
 * Fixed bug in rset_trunc_r.
 *
 * Revision 1.10  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.9  1998/01/12 15:04:09  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.8  1997/10/31 12:34:27  adam
 * Bug fix: memory leak.
 *
 * Revision 1.7  1997/09/29 09:07:29  adam
 * Minor change.
 *
 * Revision 1.6  1997/09/22 12:39:06  adam
 * Added get_pos method for the ranked result sets.
 *
 * Revision 1.5  1997/09/17 12:19:17  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.4  1996/12/23 15:30:44  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.3  1996/12/20 11:07:14  adam
 * Multi-or result set.
 *
 * Revision 1.2  1996/11/08 11:10:28  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.1  1996/11/04 14:07:40  adam
 * Moved truncation code to trunc.c.
 *
 */
#include <stdio.h>
#include <assert.h>

#define NEW_TRUNC 1

#include "zserver.h"
#include <rstemp.h>
#include <rsnull.h>
#include <rsisams.h>
#if ZMBOL
#include <rsisam.h>
#include <rsisamc.h>
#include <rsisamd.h>
#if NEW_TRUNC
#include <rsm_or.h>
#endif
#endif

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

static struct trunc_info *heap_init (int size, int key_size,
				     int (*cmp)(const void *p1,
						const void *p2))
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
    xfree (ti->ptr);
    xfree (ti->indx);
    xfree (ti->heap);
    xfree (ti->swapbuf);
    xfree (ti->tmpbuf);
    xfree (ti->buf);
    xfree (ti);
}

static RSET rset_trunc_r (ZebraHandle zi, const char *term, int length,
			 const char *flags, ISAMS_P *isam_p, int from, int to,
                         int merge_chunk)
{
    RSET result; 
    RSFD result_rsfd;
    rset_temp_parms parms;

    parms.key_size = sizeof(struct it_key);
    parms.temp_path = res_get (zi->service->res, "setTmpDir");
    parms.rset_term = rset_term_create (term, length, flags);
    result = rset_create (rset_kind_temp, &parms);
    result_rsfd = rset_open (result, RSETF_WRITE);

    if (to - from > merge_chunk)
    {
        RSFD *rsfd;
        RSET *rset;
	int term_index;
        int i, i_add = (to-from)/merge_chunk + 1;
        struct trunc_info *ti;
        int rscur = 0;
        int rsmax = (to-from)/i_add + 1;
        
        rset = (RSET *) xmalloc (sizeof(*rset) * rsmax);
        rsfd = (RSFD *) xmalloc (sizeof(*rsfd) * rsmax);
        
        for (i = from; i < to; i += i_add)
        {
            if (i_add <= to - i)
                rset[rscur] = rset_trunc_r (zi, term, length, flags,
				            isam_p, i, i+i_add, merge_chunk);
            else
                rset[rscur] = rset_trunc_r (zi, term, length, flags,
                                            isam_p, i, to, merge_chunk);
            rscur++;
        }
        ti = heap_init (rscur, sizeof(struct it_key), key_compare_it);
        for (i = rscur; --i >= 0; )
        {
            rsfd[i] = rset_open (rset[i], RSETF_READ);
            if (rset_read (rset[i], rsfd[i], ti->tmpbuf, &term_index))
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
                if (!rset_read (rset[n], rsfd[n], ti->tmpbuf, &term_index))
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
#if ZMBOL
    else if (zi->service->isam)
    {
        ISPT *ispt;
        int i;
        struct trunc_info *ti;

        ispt = (ISPT *) xmalloc (sizeof(*ispt) * (to-from));

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare_it);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = is_position (zi->service->isam, isam_p[from+i]);
            if (is_readkey (ispt[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
                is_pt_free (ispt[i]);
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);
#if 1
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
    else if (zi->service->isamc)
    {
        ISAMC_PP *ispt;
        int i;
        struct trunc_info *ti;

        ispt = (ISAMC_PP *) xmalloc (sizeof(*ispt) * (to-from));

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare_it);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = isc_pp_open (zi->service->isamc, isam_p[from+i]);
            if (isc_pp_read (ispt[i], ti->tmpbuf))
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
            if (isc_pp_read (ispt[n], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, n);
            else
                isc_pp_close (ispt[n]);
#else
/* section that preserve all keys with unique sysnos */
            while (1)
            {
                if (!isc_pp_read (ispt[n], ti->tmpbuf))
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

    else if (zi->service->isamd)
    {
        ISAMD_PP *ispt;
        int i;
        struct trunc_info *ti;

        ispt = (ISAMD_PP *) xmalloc (sizeof(*ispt) * (to-from));

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare_it);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = isamd_pp_open (zi->service->isamd, isam_p[from+i]);
            if (isamd_pp_read (ispt[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
                isamd_pp_close (ispt[i]);
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);
#if 0
/* section that preserve all keys */
            heap_delete (ti);
            if (isamd_pp_read (ispt[n], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, n);
            else
                isamd_pp_close (ispt[n]);
#else
/* section that preserve all keys with unique sysnos */
            while (1)
            {
                if (!isamd_pp_read (ispt[n], ti->tmpbuf))
                {
                    heap_delete (ti);
                    isamd_pp_close (ispt[n]);
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

#endif
    else if (zi->service->isams)
    {
        ISAMS_PP *ispt;
        int i;
        struct trunc_info *ti;

        ispt = (ISAMS_PP *) xmalloc (sizeof(*ispt) * (to-from));

        ti = heap_init (to-from, sizeof(struct it_key),
                        key_compare_it);
        for (i = to-from; --i >= 0; )
        {
            ispt[i] = isams_pp_open (zi->service->isams, isam_p[from+i]);
            if (isams_pp_read (ispt[i], ti->tmpbuf))
                heap_insert (ti, ti->tmpbuf, i);
            else
                isams_pp_close (ispt[i]);
        }
        while (ti->heapnum)
        {
            int n = ti->indx[ti->ptr[1]];

            rset_write (result, result_rsfd, ti->heap[ti->ptr[1]]);
            while (1)
            {
                if (!isams_pp_read (ispt[n], ti->tmpbuf))
                {
                    heap_delete (ti);
                    isams_pp_close (ispt[n]);
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
        heap_close (ti);
        xfree (ispt);
    }
    else
        logf (LOG_WARN, "Unknown isam set in rset_trunc_r");

    rset_close (result, result_rsfd);
    return result;
}

static int isams_trunc_cmp (const void *p1, const void *p2)
{
    ISAMS_P i1 = *(ISAMS_P*) p1;
    ISAMS_P i2 = *(ISAMS_P*) p2;

    return i1 - i2;
}

#if ZMBOL
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
static int isamd_trunc_cmp (const void *p1, const void *p2)
{
    ISAMD_P i1 = *(ISAMD_P*) p1;
    ISAMD_P i2 = *(ISAMD_P*) p2;
    int d;

    d = isamd_type (i1) - isamd_type (i2);
    if (d)
        return d;
    return isamd_block (i1) - isamd_block (i2);
}
#endif

RSET rset_trunc (ZebraHandle zi, ISAMS_P *isam_p, int no,
		 const char *term, int length, const char *flags)
{
    logf (LOG_DEBUG, "rset_trunc no=%d", no);
    if (no < 1)
    {
	rset_null_parms parms;
	parms.rset_term = rset_term_create (term, length, flags);
	return rset_create (rset_kind_null, &parms);
    }
    if (zi->service->isams)
    {
        if (no == 1)
        {
            rset_isams_parms parms;

            parms.pos = *isam_p;
            parms.is = zi->service->isams;
	    parms.rset_term = rset_term_create (term, length, flags);
            return rset_create (rset_kind_isams, &parms);
        }
        qsort (isam_p, no, sizeof(*isam_p), isams_trunc_cmp);
    }
#if ZMBOL
    else if (zi->service->isam)
    {
        if (no == 1)
        {
            rset_isam_parms parms;

            parms.pos = *isam_p;
            parms.is = zi->service->isam;
	    parms.rset_term = rset_term_create (term, length, flags);
            return rset_create (rset_kind_isam, &parms);
        }
        qsort (isam_p, no, sizeof(*isam_p), isam_trunc_cmp);
    }
    else if (zi->service->isamc)
    {
        if (no == 1)
        {
            rset_isamc_parms parms;

            parms.pos = *isam_p;
            parms.is = zi->service->isamc;
	    parms.rset_term = rset_term_create (term, length, flags);
            return rset_create (rset_kind_isamc, &parms);
        }
#if NEW_TRUNC
        else if (no < 10000)
        {
            rset_m_or_parms parms;

            parms.key_size = sizeof(struct it_key);
            parms.cmp = key_compare_it;
            parms.isc = zi->service->isamc;
            parms.isam_positions = isam_p;
            parms.no_isam_positions = no;
            parms.no_save_positions = 100000;
	    parms.rset_term = rset_term_create (term, length, flags);
            return rset_create (rset_kind_m_or, &parms);
        }
#endif
        qsort (isam_p, no, sizeof(*isam_p), isamc_trunc_cmp);
    }
    else if (zi->service->isamd)
    {
        if (no == 1)
        {
            rset_isamd_parms parms;

            parms.pos = *isam_p;
            parms.is = zi->service->isamd;
	    parms.rset_term = rset_term_create (term, length, flags);
            return rset_create (rset_kind_isamd, &parms);
        }
#if NEW_TRUNC_NOT_DONE_FOR_ISAM_D
        else if (no < 10000)
        {
            rset_m_or_parms parms;

            parms.key_size = sizeof(struct it_key);
            parms.cmp = key_compare_it;
            parms.isc = 0;
            parms.isamd=zi->service->isamd;
            parms.isam_positions = isam_p;
            parms.no_isam_positions = no;
            parms.no_save_positions = 100000;
	    parms.rset_term = rset_term_create (term, length, flags);
            return rset_create (rset_kind_m_or, &parms);
        }
#endif
        qsort (isam_p, no, sizeof(*isam_p), isamd_trunc_cmp);
    }
#endif
    else
    {
        logf (LOG_WARN, "Unknown isam set in rset_trunc");
	return rset_create (rset_kind_null, NULL);
    }
    return rset_trunc_r (zi, term, length, flags, isam_p, 0, no, 100);
}

