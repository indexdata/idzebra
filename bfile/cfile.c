/*
 * Copyright (C) 1995-2000, Index Data ApS
 * All rights reserved.
 *
 * $Log: cfile.c,v $
 * Revision 1.26  2000-03-20 19:08:35  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.25  1999/05/26 07:49:12  adam
 * C++ compilation.
 *
 * Revision 1.24  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.23  1998/10/15 13:09:29  adam
 * Minor changes.
 *
 * Revision 1.22  1998/10/13 20:07:22  adam
 * Changed some log messages.
 *
 * Revision 1.21  1998/08/24 17:29:52  adam
 * Minor changes.
 *
 * Revision 1.20  1998/08/07 15:07:13  adam
 * Fixed but in cf_commit_flat.
 *
 * Revision 1.19  1997/02/12 20:37:17  adam
 * Changed the messages logged. No real code changed.
 *
 * Revision 1.18  1996/10/29 13:56:15  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.17  1996/04/19 16:49:00  adam
 * Minor changes.
 *
 * Revision 1.16  1996/04/19  16:23:47  adam
 * Serious bug fix in shadow implementation; function new_bucket might
 * set wrong bucket number on new bucket.
 *
 * Revision 1.15  1996/04/18  16:02:56  adam
 * Changed logging a bit.
 * Removed warning message when commiting flat shadow files.
 *
 * Revision 1.14  1996/04/12  07:01:55  adam
 * Yet another bug fix (next_block was initialized to 0; now set to 1).
 *
 * Revision 1.13  1996/04/09 14:48:49  adam
 * Bug fix: offset calculation when using flat files was completely broken.
 *
 * Revision 1.12  1996/04/09  06:47:28  adam
 * Function scan_areadef doesn't use sscanf (%n fails on this Linux).
 *
 * Revision 1.11  1996/03/26 15:59:05  adam
 * The directory of the shadow table file can be specified by the new
 * bf_lockDir call.
 *
 * Revision 1.10  1996/02/07  14:03:46  adam
 * Work on flat indexed shadow files.
 *
 * Revision 1.9  1996/02/07  10:08:43  adam
 * Work on flat shadow (not finished yet).
 *
 * Revision 1.8  1995/12/15  12:36:52  adam
 * Moved hash file information to union.
 * Renamed commit files.
 *
 * Revision 1.7  1995/12/15  10:35:07  adam
 * Changed names of commit files.
 *
 * Revision 1.6  1995/12/11  09:03:53  adam
 * New function: cf_unlink.
 * New member of commit file head: state (0) deleted, (1) hash file.
 *
 * Revision 1.5  1995/12/08  16:21:14  adam
 * Work on commit/update.
 *
 * Revision 1.4  1995/12/01  16:24:28  adam
 * Commit files use separate meta file area.
 *
 * Revision 1.3  1995/12/01  11:37:22  adam
 * Cached/commit files implemented as meta-files.
 *
 * Revision 1.2  1995/11/30  17:00:49  adam
 * Several bug fixes. Commit system runs now.
 *
 * Revision 1.1  1995/11/30  08:33:11  adam
 * Started work on commit facility.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <zebrautl.h>
#include <mfile.h>
#include "cfile.h"

static int write_head (CFile cf)
{
    int left = cf->head.hash_size * sizeof(int);
    int bno = 1;
    const char *tab = (char*) cf->array;

    if (!tab)
        return 0;
    while (left >= (int) HASH_BSIZE)
    {
        mf_write (cf->hash_mf, bno++, 0, 0, tab);
        tab += HASH_BSIZE;
        left -= HASH_BSIZE;
    }
    if (left > 0)
        mf_write (cf->hash_mf, bno, 0, left, tab);
    return 0;
}

static int read_head (CFile cf)
{
    int left = cf->head.hash_size * sizeof(int);
    int bno = 1;
    char *tab = (char*) cf->array;

    if (!tab)
        return 0;
    while (left >= (int) HASH_BSIZE)
    {
        mf_read (cf->hash_mf, bno++, 0, 0, tab);
        tab += HASH_BSIZE;
        left -= HASH_BSIZE;
    }
    if (left > 0)
        mf_read (cf->hash_mf, bno, 0, left, tab);
    return 0;
}


CFile cf_open (MFile mf, MFile_area area, const char *fname,
               int block_size, int wflag, int *firstp)
{
    char path[1024];
    int i;
    CFile cf = (CFile) xmalloc (sizeof(*cf));
    int hash_bytes;
   
    cf->rmf = mf; 
    logf (LOG_DEBUG, "cf: open %s %s", cf->rmf->name, wflag ? "rdwr" : "rd");
    sprintf (path, "%s-b", fname);
    if (!(cf->block_mf = mf_open (area, path, block_size, wflag)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "Failed to open %s", path);
        exit (1);
    }
    sprintf (path, "%s-i", fname);
    if (!(cf->hash_mf = mf_open (area, path, HASH_BSIZE, wflag)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "Failed to open %s", path);
        exit (1);
    }
    assert (firstp);
    if (!mf_read (cf->hash_mf, 0, 0, sizeof(cf->head), &cf->head) ||
        !cf->head.state)
    {
        *firstp = 1;
        cf->head.state = 1;
        cf->head.block_size = block_size;
        cf->head.hash_size = 199;
        hash_bytes = cf->head.hash_size * sizeof(int);
        cf->head.flat_bucket = cf->head.next_bucket = cf->head.first_bucket = 
            (hash_bytes+sizeof(cf->head))/HASH_BSIZE + 2;
        cf->head.next_block = 1;
        if (wflag)
            mf_write (cf->hash_mf, 0, 0, sizeof(cf->head), &cf->head);
        cf->array = (int *) xmalloc (hash_bytes);
        for (i = 0; i<cf->head.hash_size; i++)
            cf->array[i] = 0;
        if (wflag)
            write_head (cf);
    }
    else
    {
        *firstp = 0;
        assert (cf->head.block_size == block_size);
        assert (cf->head.hash_size > 2);
        hash_bytes = cf->head.hash_size * sizeof(int);
        assert (cf->head.next_bucket > 0);
        assert (cf->head.next_block > 0);
        if (cf->head.state == 1)
            cf->array = (int *) xmalloc (hash_bytes);
        else
            cf->array = NULL;
        read_head (cf);
    }
    if (cf->head.state == 1)
    {
        cf->parray = (struct CFile_hash_bucket **)
	    xmalloc (cf->head.hash_size * sizeof(*cf->parray));
        for (i = 0; i<cf->head.hash_size; i++)
            cf->parray[i] = NULL;
    }
    else
        cf->parray = NULL;
    cf->bucket_lru_front = cf->bucket_lru_back = NULL;
    cf->bucket_in_memory = 0;
    cf->max_bucket_in_memory = 100;
    cf->dirty = 0;
    cf->iobuf = (char *) xmalloc (cf->head.block_size);
    memset (cf->iobuf, 0, cf->head.block_size);
    cf->no_hits = 0;
    cf->no_miss = 0;
    zebra_mutex_init (&cf->mutex);
    return cf;
}

static int cf_hash (CFile cf, int no)
{
    return (no>>3) % cf->head.hash_size;
}

static void release_bucket (CFile cf, struct CFile_hash_bucket *p)
{
    if (p->lru_prev)
        p->lru_prev->lru_next = p->lru_next;
    else
        cf->bucket_lru_back = p->lru_next;
    if (p->lru_next)
        p->lru_next->lru_prev = p->lru_prev;
    else
        cf->bucket_lru_front = p->lru_prev;

    *p->h_prev = p->h_next;
    if (p->h_next)
        p->h_next->h_prev = p->h_prev;
    
    --(cf->bucket_in_memory);
    xfree (p);
}

static void flush_bucket (CFile cf, int no_to_flush)
{
    int i;
    struct CFile_hash_bucket *p;

    for (i = 0; i != no_to_flush; i++)
    {
        p = cf->bucket_lru_back;
        if (!p)
            break;
        if (p->dirty)
        {
            mf_write (cf->hash_mf, p->ph.this_bucket, 0, 0, &p->ph);
            cf->dirty = 1;
        }
        release_bucket (cf, p);
    }
}

static struct CFile_hash_bucket *alloc_bucket (CFile cf, int block_no, int hno)
{
    struct CFile_hash_bucket *p, **pp;

    if (cf->bucket_in_memory == cf->max_bucket_in_memory)
        flush_bucket (cf, 1);
    assert (cf->bucket_in_memory < cf->max_bucket_in_memory);
    ++(cf->bucket_in_memory);
    p = (struct CFile_hash_bucket *) xmalloc (sizeof(*p));

    p->lru_next = NULL;
    p->lru_prev = cf->bucket_lru_front;
    if (cf->bucket_lru_front)
        cf->bucket_lru_front->lru_next = p;
    else
        cf->bucket_lru_back = p;
    cf->bucket_lru_front = p; 

    pp = cf->parray + hno;
    p->h_next = *pp;
    p->h_prev = pp;
    if (*pp)
        (*pp)->h_prev = &p->h_next;
    *pp = p;
    return p;
}

static struct CFile_hash_bucket *get_bucket (CFile cf, int block_no, int hno)
{
    struct CFile_hash_bucket *p;

    p = alloc_bucket (cf, block_no, hno);
    if (!mf_read (cf->hash_mf, block_no, 0, 0, &p->ph))
    {
        logf (LOG_FATAL|LOG_ERRNO, "read get_bucket");
        exit (1);
    }
    assert (p->ph.this_bucket == block_no);
    p->dirty = 0;
    return p;
}

static struct CFile_hash_bucket *new_bucket (CFile cf, int *block_nop, int hno)
{
    struct CFile_hash_bucket *p;
    int i, block_no;

    block_no = *block_nop = cf->head.next_bucket++;
    p = alloc_bucket (cf, block_no, hno);

    for (i = 0; i<HASH_BUCKET; i++)
    {
        p->ph.vno[i] = 0;
        p->ph.no[i] = 0;
    }
    p->ph.next_bucket = 0;
    p->ph.this_bucket = block_no;
    p->dirty = 1;
    return p;
}

static int cf_lookup_flat (CFile cf, int no)
{
    int hno = (no*sizeof(int))/HASH_BSIZE;
    int off = (no*sizeof(int)) - hno*HASH_BSIZE;
    int vno = 0;

    mf_read (cf->hash_mf, hno+cf->head.next_bucket, off, sizeof(int), &vno);
    return vno;
}

static int cf_lookup_hash (CFile cf, int no)
{
    int hno = cf_hash (cf, no);
    struct CFile_hash_bucket *hb;
    int block_no, i;

    for (hb = cf->parray[hno]; hb; hb = hb->h_next)
    {
        for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
            if (hb->ph.no[i] == no)
            {
                (cf->no_hits)++;
                return hb->ph.vno[i];
            }
    }
    for (block_no = cf->array[hno]; block_no; block_no = hb->ph.next_bucket)
    {
        for (hb = cf->parray[hno]; hb; hb = hb->h_next)
        {
            if (hb->ph.this_bucket == block_no)
                break;
        }
        if (hb)
            continue;
#if 0
        /* extra check ... */
        for (hb = cf->bucket_lru_back; hb; hb = hb->lru_next)
        {
            if (hb->ph.this_bucket == block_no)
            {
                logf (LOG_FATAL, "Found hash bucket on other chain (1)");
                abort ();
            }
            for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
                if (hb->ph.no[i] == no)
                {
                    logf (LOG_FATAL, "Found hash bucket on other chain (2)");
                    abort ();
                }
        }
#endif
        (cf->no_miss)++;
        hb = get_bucket (cf, block_no, hno);
        for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
            if (hb->ph.no[i] == no)
                return hb->ph.vno[i];
    }
    return 0;
}

static void cf_write_flat (CFile cf, int no, int vno)
{
    int hno = (no*sizeof(int))/HASH_BSIZE;
    int off = (no*sizeof(int)) - hno*HASH_BSIZE;

    hno += cf->head.next_bucket;
    if (hno >= cf->head.flat_bucket)
        cf->head.flat_bucket = hno+1;
    cf->dirty = 1;
    mf_write (cf->hash_mf, hno, off, sizeof(int), &vno);
}

static void cf_moveto_flat (CFile cf)
{
    struct CFile_hash_bucket *p;
    int i, j;

    logf (LOG_DEBUG, "cf: Moving to flat shadow: %s", cf->rmf->name);
    logf (LOG_DEBUG, "cf: hits=%d miss=%d bucket_in_memory=%d total=%d",
	cf->no_hits, cf->no_miss, cf->bucket_in_memory, 
        cf->head.next_bucket - cf->head.first_bucket);
    assert (cf->head.state == 1);
    flush_bucket (cf, -1);
    assert (cf->bucket_in_memory == 0);
    p = (struct CFile_hash_bucket *) xmalloc (sizeof(*p));
    for (i = cf->head.first_bucket; i < cf->head.next_bucket; i++)
    {
        if (!mf_read (cf->hash_mf, i, 0, 0, &p->ph))
        {
            logf (LOG_FATAL|LOG_ERRNO, "read bucket moveto flat");
            exit (1);
        }
        for (j = 0; j < HASH_BUCKET && p->ph.vno[j]; j++)
            cf_write_flat (cf, p->ph.no[j], p->ph.vno[j]);
    }
    xfree (p);
    xfree (cf->array);
    cf->array = NULL;
    xfree (cf->parray);
    cf->parray = NULL;
    cf->head.state = 2;
    cf->dirty = 1;
}

static int cf_lookup (CFile cf, int no)
{
    if (cf->head.state > 1)
        return cf_lookup_flat (cf, no);
    return cf_lookup_hash (cf, no);
}

static int cf_new_flat (CFile cf, int no)
{
    int vno = (cf->head.next_block)++;

    cf_write_flat (cf, no, vno);
    return vno;
}

static int cf_new_hash (CFile cf, int no)
{
    int hno = cf_hash (cf, no);
    struct CFile_hash_bucket *hbprev = NULL, *hb = cf->parray[hno];
    int *bucketpp = &cf->array[hno]; 
    int i, vno = (cf->head.next_block)++;
  
    for (hb = cf->parray[hno]; hb; hb = hb->h_next)
        if (!hb->ph.vno[HASH_BUCKET-1])
            for (i = 0; i<HASH_BUCKET; i++)
                if (!hb->ph.vno[i])
                {
                    (cf->no_hits)++;
                    hb->ph.no[i] = no;
                    hb->ph.vno[i] = vno;
                    hb->dirty = 1;
                    return vno;
                }

    while (*bucketpp)
    {
        for (hb = cf->parray[hno]; hb; hb = hb->h_next)
            if (hb->ph.this_bucket == *bucketpp)
            {
                bucketpp = &hb->ph.next_bucket;
                hbprev = hb;
                break;
            }
        if (hb)
            continue;

#if 0
        /* extra check ... */
        for (hb = cf->bucket_lru_back; hb; hb = hb->lru_next)
        {
            if (hb->ph.this_bucket == *bucketpp)
            {
                logf (LOG_FATAL, "Found hash bucket on other chain");
                abort ();
            }
        }
#endif
        (cf->no_miss)++;
        hb = get_bucket (cf, *bucketpp, hno);
        assert (hb);
        for (i = 0; i<HASH_BUCKET; i++)
            if (!hb->ph.vno[i])
            {
                hb->ph.no[i] = no;
                hb->ph.vno[i] = vno;
                hb->dirty = 1;
                return vno;
            }
        bucketpp = &hb->ph.next_bucket;
        hbprev = hb;
    }
    if (hbprev)
        hbprev->dirty = 1;
    hb = new_bucket (cf, bucketpp, hno);
    hb->ph.no[0] = no;
    hb->ph.vno[0] = vno;
    return vno;
}

int cf_new (CFile cf, int no)
{
    if (cf->head.state > 1)
        return cf_new_flat (cf, no);
    if (cf->no_miss*2 > cf->no_hits)
    {
        cf_moveto_flat (cf);
        assert (cf->head.state > 1);
        return cf_new_flat (cf, no);
    }
    return cf_new_hash (cf, no);
}


int cf_read (CFile cf, int no, int offset, int nbytes, void *buf)
{
    int block;
    
    assert (cf);
    zebra_mutex_lock (&cf->mutex);
    if (!(block = cf_lookup (cf, no)))
    {
	zebra_mutex_unlock (&cf->mutex);
        return -1;
    }
    zebra_mutex_unlock (&cf->mutex);
    if (!mf_read (cf->block_mf, block, offset, nbytes, buf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_read no=%d, block=%d", no, block);
        exit (1);
    }
    return 1;
}

int cf_write (CFile cf, int no, int offset, int nbytes, const void *buf)
{
    int block;

    assert (cf);
    zebra_mutex_lock (&cf->mutex);
    if (!(block = cf_lookup (cf, no)))
    {
        block = cf_new (cf, no);
        if (offset || nbytes)
        {
            mf_read (cf->rmf, no, 0, 0, cf->iobuf);
            memcpy (cf->iobuf + offset, buf, nbytes);
            buf = cf->iobuf;
            offset = 0;
            nbytes = 0;
        }
    }
    zebra_mutex_unlock (&cf->mutex);
    if (mf_write (cf->block_mf, block, offset, nbytes, buf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_write no=%d, block=%d", no, block);
        exit (1);
    }
    return 0;
}

int cf_close (CFile cf)
{
    logf (LOG_DEBUG, "cf: close hits=%d miss=%d bucket_in_memory=%d total=%d",
          cf->no_hits, cf->no_miss, cf->bucket_in_memory,
          cf->head.next_bucket - cf->head.first_bucket);
    flush_bucket (cf, -1);
    if (cf->dirty)
    {
        mf_write (cf->hash_mf, 0, 0, sizeof(cf->head), &cf->head);
        write_head (cf);
    }
    mf_close (cf->hash_mf);
    mf_close (cf->block_mf);
    xfree (cf->array);
    xfree (cf->parray);
    xfree (cf->iobuf);
    zebra_mutex_destroy (&cf->mutex);
    xfree (cf);
    return 0;
}

