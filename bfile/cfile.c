/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: cfile.c,v $
 * Revision 1.6  1995-12-11 09:03:53  adam
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

#include <alexutil.h>
#include <mfile.h>
#include "cfile.h"

static int write_head (CFile cf)
{
    int left = cf->head.hash_size * sizeof(int);
    int bno = 1;
    const char *tab = (char*) cf->array;

    while (left >= HASH_BSIZE)
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

    while (left >= HASH_BSIZE)
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
    char path[256];
    int i;
    CFile cf = xmalloc (sizeof(*cf));
    int hash_bytes;
   
    cf->rmf = mf; 
    sprintf (path, "%s.b", fname);
    if (!(cf->block_mf = mf_open (area, path, block_size, wflag)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "Failed to open %s", path);
        exit (1);
    }
    sprintf (path, "%s.h", fname);
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
        cf->head.hash_size = 401;
        hash_bytes = cf->head.hash_size * sizeof(int);
        cf->head.next_bucket =
            (hash_bytes+sizeof(cf->head))/HASH_BSIZE + 2;
        cf->head.next_block = 1;
        if (wflag)
            mf_write (cf->hash_mf, 0, 0, sizeof(cf->head), &cf->head);
        cf->array = xmalloc (hash_bytes);
        for (i = 0; i<cf->head.hash_size; i++)
            cf->array[i] = 0;
        if (wflag)
            write_head (cf);
    }
    else
    {
        *firstp = 0;
        assert (cf->head.block_size == block_size);
        assert (cf->head.hash_size > 2 && cf->head.hash_size < 200000);
        hash_bytes = cf->head.hash_size * sizeof(int);
        assert (cf->head.next_bucket > 0);
        cf->array = xmalloc (hash_bytes);
        read_head (cf);
    }
    cf->parray = xmalloc (cf->head.hash_size * sizeof(*cf->parray));
    for (i = 0; i<cf->head.hash_size; i++)
        cf->parray[i] = NULL;
    cf->bucket_lru_front = cf->bucket_lru_back = NULL;
    cf->bucket_in_memory = 0;
    cf->max_bucket_in_memory = 400;
    cf->dirty = 0;
    cf->iobuf = xmalloc (cf->head.block_size);
    memset (cf->iobuf, 0, cf->head.block_size);
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
    p = xmalloc (sizeof(*p));

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

static struct CFile_hash_bucket *new_bucket (CFile cf, int *block_no, int hno)
{
    struct CFile_hash_bucket *p;
    int i;

    *block_no = cf->head.next_bucket++;
    p = alloc_bucket (cf, *block_no, hno);

    for (i = 0; i<HASH_BUCKET; i++)
    {
        p->ph.vno[i] = 0;
        p->ph.no[i] = 0;
    }
    p->ph.next_bucket = 0;
    p->ph.this_bucket = *block_no;
    p->dirty = 1;
    return p;
}

int cf_lookup (CFile cf, int no)
{
    int hno = cf_hash (cf, no);
    struct CFile_hash_bucket *hb;
    int block_no, i;

    for (hb = cf->parray[hno]; hb; hb = hb->h_next)
    {
        for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
            if (hb->ph.no[i] == no)
                return hb->ph.vno[i];
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
        hb = get_bucket (cf, block_no, hno);
        for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
            if (hb->ph.no[i] == no)
                return hb->ph.vno[i];
    }
    return 0;
}

int cf_new (CFile cf, int no)
{
    int hno = cf_hash (cf, no);
    struct CFile_hash_bucket *hbprev = NULL, *hb = cf->parray[hno];
    int *bucketpp = &cf->array[hno];
    int i;
    int vno = (cf->head.next_block)++;
    
    for (hb = cf->parray[hno]; hb; hb = hb->h_next)
        if (!hb->ph.vno[HASH_BUCKET-1])
            for (i = 0; i<HASH_BUCKET; i++)
                if (!hb->ph.vno[i])
                {
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

int cf_read (CFile cf, int no, int offset, int num, void *buf)
{
    int block;
    
    assert (cf);
    if (!(block = cf_lookup (cf, no)))
        return -1;
    if (!mf_read (cf->block_mf, block, offset, num, buf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_read no=%d, block=%d", no, block);
        exit (1);
    }
    return 1;
}

int cf_write (CFile cf, int no, int offset, int num, const void *buf)
{
    int block;

    assert (cf);
    if (!(block = cf_lookup (cf, no)))
    {
        block = cf_new (cf, no);
        if (offset || num)
        {
            mf_read (cf->rmf, no, 0, 0, cf->iobuf);
            memcpy (cf->iobuf + offset, buf, num);
            buf = cf->iobuf;
            offset = 0;
            num = 0;
        }
    }
    if (mf_write (cf->block_mf, block, offset, num, buf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_write no=%d, block=%d", no, block);
        exit (1);
    }
    return 0;
}

int cf_close (CFile cf)
{
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
    xfree (cf);
    return 0;
}

