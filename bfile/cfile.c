/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: cfile.c,v $
 * Revision 1.2  1995-11-30 17:00:49  adam
 * Several bug fixes. Commit system runs now.
 *
 * Revision 1.1  1995/11/30  08:33:11  adam
 * Started work on commit facility.
 *
 */

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <alexutil.h>
#include <mfile.h>
#include "cfile.h"

static int hash_write (CFile cf, const void *buf, size_t bytes)
{
    int r;

    r = write (cf->hash_fd, buf, bytes);
    if (r == bytes)
        return bytes;
    if (r == -1)
        logf (LOG_FATAL|LOG_ERRNO, "write in commit hash file");
    else
        logf (LOG_FATAL, "write in commit hash file. "
                     "%d bytes instead of %d bytes", r, bytes);
    exit (1);
    return 0;
}

static int hash_read (CFile cf, void *buf, size_t bytes)
{
    int r;

    r = read (cf->hash_fd, buf, bytes);
    if (r == bytes)
        return bytes;
    if (r == -1)
        logf (LOG_FATAL|LOG_ERRNO, "read in commit hash file");
    else
        logf (LOG_FATAL, "read in commit hash file. "
                     "%d bytes instead of %d bytes", r, bytes);
    abort ();
    return 0;
}

CFile cf_open (MFile mf, const char *cname, const char *fname,
               int block_size, int wflag, int *firstp)
{
    char path[256];
    int r, i;
    CFile cf = xmalloc (sizeof(*cf));
    int hash_bytes;
   
    cf->mf = mf; 
    sprintf (path, "%s.%s.b", cname, fname);
    if ((cf->block_fd = 
        open (path, wflag ? O_RDWR|O_CREAT : O_RDONLY, 0666)) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Failed to open %s", path);
        exit (1);
    }
    sprintf (path, "%s.%s.h", cname, fname);
    if ((cf->hash_fd = 
        open (path, wflag ? O_RDWR|O_CREAT : O_RDONLY, 0666)) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Failed to open %s", path);
        exit (1);
    }
    r = read (cf->hash_fd, &cf->head, sizeof(cf->head));
    if (r != sizeof(cf->head))
    {
        *firstp = 1;
        if (r == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "read head of %s", path);
            exit (1);
        }
        if (r != 0)
        {
            logf (LOG_FATAL, "illegal head of %s", path);
            exit (1);
        }
        cf->head.block_size = block_size;
        cf->head.hash_size = 401;
        hash_bytes = cf->head.hash_size * sizeof(int);
        cf->head.next_bucket =
            (hash_bytes+sizeof(cf->head))/HASH_BSIZE + 2;
        cf->head.next_block = 1;
        if (wflag)
            hash_write (cf, &cf->head, sizeof(cf->head));
        cf->array = xmalloc (hash_bytes);
        for (i = 0; i<cf->head.hash_size; i++)
            cf->array[i] = 0;
        if (wflag)
            hash_write (cf, cf->array, hash_bytes);
    }
    else
    {
        *firstp = 0;
        assert (cf->head.block_size == block_size);
        assert (cf->head.hash_size > 2 && cf->head.hash_size < 200000);
        hash_bytes = cf->head.hash_size * sizeof(int);
        assert (cf->head.next_bucket > 0);
        cf->array = xmalloc (hash_bytes);
        hash_read (cf, cf->array, hash_bytes);
    }
    cf->parray = xmalloc (cf->head.hash_size * sizeof(*cf->parray));
    for (i = 0; i<cf->head.hash_size; i++)
        cf->parray[i] = NULL;
    cf->bucket_lru_front = cf->bucket_lru_back = NULL;
    cf->bucket_in_memory = 0;
    cf->max_bucket_in_memory = 200;
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
            if (lseek (cf->hash_fd, p->ph.this_bucket*HASH_BSIZE, SEEK_SET) < 0)
            {
                logf (LOG_FATAL|LOG_ERRNO, "lseek in flush_bucket");
                exit (1);
            }
            hash_write (cf, &p->ph, HASH_BSIZE);
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

    if (lseek (cf->hash_fd, block_no * HASH_BSIZE, SEEK_SET) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "lseek in get_bucket");
        exit (1);
    }
    hash_read (cf, &p->ph, HASH_BSIZE);
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
    int tor, block, r;
    
    assert (cf);
    if (!(block = cf_lookup (cf, no)))
        return -1;
    if (lseek (cf->block_fd, cf->head.block_size * block + offset,
         SEEK_SET) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_read, lseek no=%d, block=%d",
              no, block);
        exit (1);
    }
    tor = num ? num : cf->head.block_size;
    r = read (cf->block_fd, buf, tor);
    if (r != tor)
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_read, read no=%d, block=%d",
              no, block);
        exit (1);
    }
    return 1;
}

int cf_write (CFile cf, int no, int offset, int num, const void *buf)
{
    int block, r, tow;

    assert (cf);
    if (!(block = cf_lookup (cf, no)))
    {
        block = cf_new (cf, no);
        if (offset || num)
        {
            mf_read (cf->mf, no, 0, 0, cf->iobuf);
            memcpy (cf->iobuf + offset, buf, num);
            buf = cf->iobuf;
            offset = 0;
            num = 0;
        }
    }
    if (lseek (cf->block_fd, cf->head.block_size * block + offset,
               SEEK_SET) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_write, lseek no=%d, block=%d",
              no, block);
        exit (1);
    }

    tow = num ? num : cf->head.block_size;
    r = write (cf->block_fd, buf, tow);
    if (r != tow)
    {
        logf (LOG_FATAL|LOG_ERRNO, "cf_write, read no=%d, block=%d",
              no, block);
        exit (1);
    }
    return 0;
}

int cf_close (CFile cf)
{
    flush_bucket (cf, -1);
    if (cf->dirty)
    {
        int hash_bytes = cf->head.hash_size * sizeof(int);
        if (lseek (cf->hash_fd, 0L, SEEK_SET) < 0)
        {
            logf (LOG_FATAL|LOG_ERRNO, "seek in hash fd");
            exit (1);
        }
        hash_write (cf, &cf->head, sizeof(cf->head));
        hash_write (cf, cf->array, hash_bytes);
    }
    if (close (cf->hash_fd) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "close hash fd");
        exit (1);
    }
    if (close (cf->block_fd) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "close block fd");
        exit (1);
    }
    xfree (cf->array);
    xfree (cf->parray);
    xfree (cf->iobuf);
    xfree (cf);
    return 0;
}

