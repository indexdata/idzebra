/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <idzebra/util.h>
#include <yaz/yaz-util.h>
#include "mfile.h"
#include "cfile.h"

/** \brief set to 1 if extra commit/shadow check is to be performed */
#define EXTRA_CHECK 0

static int write_head(CFile cf)
{
    int left = cf->head.hash_size * sizeof(zint);
    int bno = 1;
    int r = 0;
    const char *tab = (char*) cf->array;

    if (!tab)
        return 0;
    while (left >= (int) HASH_BSIZE)
    {
        r = mf_write(cf->hash_mf, bno++, 0, 0, tab);
        if (r)
            return r;
        tab += HASH_BSIZE;
        left -= HASH_BSIZE;
    }
    if (left > 0)
        r = mf_write(cf->hash_mf, bno, 0, left, tab);
    return r;
}

static int read_head(CFile cf)
{
    int left = cf->head.hash_size * sizeof(zint);
    int bno = 1;
    char *tab = (char*) cf->array;

    if (!tab)
        return 0;
    while (left >= (int) HASH_BSIZE)
    {
        if (mf_read(cf->hash_mf, bno++, 0, 0, tab) == -1)
            return -1;
        tab += HASH_BSIZE;
        left -= HASH_BSIZE;
    }
    if (left > 0)
    {
        if (mf_read(cf->hash_mf, bno, 0, left, tab) == -1)
            return -1;
    }
    return 1;
}


CFile cf_open(MFile mf, MFile_area area, const char *fname,
              int block_size, int wflag, int *firstp)
{
    char path[1024];
    int i, ret;
    CFile cf = (CFile) xmalloc(sizeof(*cf));
    int hash_bytes;

    /* avoid valgrind warnings, but set to something nasty */
    memset(cf, 'Z', sizeof(*cf));

    yaz_log(YLOG_DEBUG, "cf: open %s %s", fname, wflag ? "rdwr" : "rd");

    cf->block_mf = 0;
    cf->hash_mf = 0;
    cf->rmf = mf;

    assert(firstp);

    cf->bucket_lru_front = cf->bucket_lru_back = NULL;
    cf->bucket_in_memory = 0;
    cf->max_bucket_in_memory = 100;
    cf->dirty = 0;
    cf->iobuf = (char *) xmalloc(block_size);
    memset(cf->iobuf, 0, block_size);
    cf->no_hits = 0;
    cf->no_miss = 0;
    cf->parray = 0;
    cf->array = 0;
    cf->block_mf = 0;
    cf->hash_mf = 0;

    zebra_mutex_init(&cf->mutex);

    sprintf(path, "%s-b", fname);
    if (!(cf->block_mf = mf_open(area, path, block_size, wflag)))
    {
        cf_close(cf);
        return 0;
    }
    sprintf(path, "%s-i", fname);
    if (!(cf->hash_mf = mf_open(area, path, HASH_BSIZE, wflag)))
    {
        cf_close(cf);
        return 0;
    }
    ret = mf_read(cf->hash_mf, 0, 0, sizeof(cf->head), &cf->head);

    if (ret == -1)
    {
        cf_close(cf);
        return 0;
    }
    if (ret == 0 || !cf->head.state)
    {
        *firstp = 1;
        cf->head.state = CFILE_STATE_HASH;
        cf->head.block_size = block_size;
        cf->head.hash_size = 199;
        hash_bytes = cf->head.hash_size * sizeof(zint);
        cf->head.flat_bucket = cf->head.next_bucket = cf->head.first_bucket =
            (hash_bytes+sizeof(cf->head))/HASH_BSIZE + 2;
        cf->head.next_block = 1;
        cf->array = (zint *) xmalloc(hash_bytes);
        for (i = 0; i<cf->head.hash_size; i++)
            cf->array[i] = 0;
        if (wflag)
        {
            if (mf_write(cf->hash_mf, 0, 0, sizeof(cf->head), &cf->head))
            {
                cf_close(cf);
                return 0;
            }
            if (write_head(cf))
            {
                cf_close(cf);
                return 0;
            }
        }
    }
    else
    {
        *firstp = 0;
        assert(cf->head.block_size == block_size);
        assert(cf->head.hash_size > 2);
        hash_bytes = cf->head.hash_size * sizeof(zint);
        assert(cf->head.next_bucket > 0);
        assert(cf->head.next_block > 0);
        if (cf->head.state == CFILE_STATE_HASH)
            cf->array = (zint *) xmalloc(hash_bytes);
        else
            cf->array = NULL;
        if (read_head(cf) == -1)
        {
            cf_close(cf);
            return 0;
        }
    }
    if (cf->head.state == CFILE_STATE_HASH)
    {
        cf->parray = (struct CFile_hash_bucket **)
	    xmalloc(cf->head.hash_size * sizeof(*cf->parray));
        for (i = 0; i<cf->head.hash_size; i++)
            cf->parray[i] = NULL;
    }
    return cf;
}

static int cf_hash(CFile cf, zint no)
{
    return (int) (((no >> 3) % cf->head.hash_size));
}

static void release_bucket(CFile cf, struct CFile_hash_bucket *p)
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
    xfree(p);
}

static int flush_bucket(CFile cf, int no_to_flush)
{
    int i;
    int ret = 0;
    struct CFile_hash_bucket *p;

    for (i = 0; i != no_to_flush; i++)
    {
        p = cf->bucket_lru_back;
        if (!p)
            break;
        if (p->dirty)
        {
            if (ret == 0)
            {
                if (mf_write(cf->hash_mf, p->ph.this_bucket, 0, 0, &p->ph))
                    ret = -1;
            }
            cf->dirty = 1;
        }
        release_bucket(cf, p);
    }
    return ret;
}

static struct CFile_hash_bucket *alloc_bucket(CFile cf, zint block_no, int hno)
{
    struct CFile_hash_bucket *p, **pp;

    if (cf->bucket_in_memory == cf->max_bucket_in_memory)
    {
        if (flush_bucket(cf, 1))
            return 0;
    }
    assert(cf->bucket_in_memory < cf->max_bucket_in_memory);
    ++(cf->bucket_in_memory);
    p = (struct CFile_hash_bucket *) xmalloc(sizeof(*p));

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

static struct CFile_hash_bucket *get_bucket(CFile cf, zint block_no, int hno)
{
    struct CFile_hash_bucket *p;

    p = alloc_bucket(cf, block_no, hno);
    if (!p)
        return 0;
    p->dirty = 0;
    if (mf_read(cf->hash_mf, block_no, 0, 0, &p->ph) != 1)
    {
        yaz_log(YLOG_FATAL, "read get_bucket");
        release_bucket(cf, p);
        return 0;
    }
    assert(p->ph.this_bucket == block_no);
    return p;
}

static struct CFile_hash_bucket *new_bucket(CFile cf, zint *block_nop, int hno)
{
    struct CFile_hash_bucket *p;
    int i;
    zint block_no;

    block_no = *block_nop = cf->head.next_bucket++;
    p = alloc_bucket(cf, block_no, hno);
    if (!p)
        return 0;
    p->dirty = 1;

    for (i = 0; i<HASH_BUCKET; i++)
    {
        p->ph.vno[i] = 0;
        p->ph.no[i] = 0;
    }
    p->ph.next_bucket = 0;
    p->ph.this_bucket = block_no;
    return p;
}

static int cf_lookup_flat(CFile cf, zint no, zint *vno)
{
    zint hno = (no*sizeof(zint))/HASH_BSIZE;
    int off = (int) ((no*sizeof(zint)) - hno*HASH_BSIZE);

    *vno = 0;
    if (mf_read(cf->hash_mf, hno+cf->head.next_bucket, off, sizeof(zint), vno)
        == -1)
        return -1;
    if (*vno)
        return 1;
    return 0;
}

static int cf_lookup_hash(CFile cf, zint no, zint *vno)
{
    int hno = cf_hash(cf, no);
    struct CFile_hash_bucket *hb;
    zint block_no;
    int i;

    for (hb = cf->parray[hno]; hb; hb = hb->h_next)
    {
        for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
            if (hb->ph.no[i] == no)
            {
                (cf->no_hits)++;
                *vno = hb->ph.vno[i];
                return 1;
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
#if EXTRA_CHECK
        for (hb = cf->bucket_lru_back; hb; hb = hb->lru_next)
        {
            if (hb->ph.this_bucket == block_no)
            {
                yaz_log(YLOG_FATAL, "Found hash bucket on other chain(1)");
                return -1;
            }
            for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
                if (hb->ph.no[i] == no)
                {
                    yaz_log(YLOG_FATAL, "Found hash bucket on other chain (2)");
                    return -1;
                }
        }
#endif
        (cf->no_miss)++;
        hb = get_bucket(cf, block_no, hno);
        if (!hb)
            return -1;
        for (i = 0; i<HASH_BUCKET && hb->ph.vno[i]; i++)
            if (hb->ph.no[i] == no)
            {
                *vno = hb->ph.vno[i];
                return 1;
            }
    }
    return 0;
}

static int cf_write_flat(CFile cf, zint no, zint vno)
{
    zint hno = (no*sizeof(zint))/HASH_BSIZE;
    int off = (int) ((no*sizeof(zint)) - hno*HASH_BSIZE);

    hno += cf->head.next_bucket;
    if (hno >= cf->head.flat_bucket)
        cf->head.flat_bucket = hno+1;
    cf->dirty = 1;
    return mf_write(cf->hash_mf, hno, off, sizeof(zint), &vno);
}

static int cf_moveto_flat(CFile cf)
{
    struct CFile_hash_bucket *p;
    int j;
    zint i;

    yaz_log(YLOG_DEBUG, "cf: Moving to flat shadow: %s", cf->rmf->name);
    yaz_log(YLOG_DEBUG, "cf: hits=%d miss=%d bucket_in_memory=" ZINT_FORMAT " total="
	  ZINT_FORMAT,
	cf->no_hits, cf->no_miss, cf->bucket_in_memory,
        cf->head.next_bucket - cf->head.first_bucket);
    assert(cf->head.state == CFILE_STATE_HASH);
    if (flush_bucket(cf, -1))
        return -1;
    assert(cf->bucket_in_memory == 0);
    p = (struct CFile_hash_bucket *) xmalloc(sizeof(*p));
    for (i = cf->head.first_bucket; i < cf->head.next_bucket; i++)
    {
        if (mf_read(cf->hash_mf, i, 0, 0, &p->ph) != 1)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "read bucket moveto flat");
            xfree(p);
            return -1;
        }
        for (j = 0; j < HASH_BUCKET && p->ph.vno[j]; j++)
        {
            if (cf_write_flat(cf, p->ph.no[j], p->ph.vno[j]))
            {
                xfree(p);
                return -1;
            }
        }
    }
    xfree(p);
    xfree(cf->array);
    cf->array = NULL;
    xfree(cf->parray);
    cf->parray = NULL;
    cf->head.state = CFILE_STATE_FLAT;
    cf->dirty = 1;
    return 0;
}

static int cf_lookup(CFile cf, zint no, zint *vno)
{
    if (cf->head.state > 1)
        return cf_lookup_flat(cf, no, vno);
    return cf_lookup_hash(cf, no, vno);
}

static zint cf_new_flat(CFile cf, zint no)
{
    zint vno = (cf->head.next_block)++;

    cf_write_flat(cf, no, vno);
    return vno;
}

static zint cf_new_hash(CFile cf, zint no)
{
    int hno = cf_hash(cf, no);
    struct CFile_hash_bucket *hbprev = NULL, *hb = cf->parray[hno];
    zint *bucketpp = &cf->array[hno];
    int i;
    zint vno = (cf->head.next_block)++;

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

#if EXTRA_CHECK
        for (hb = cf->bucket_lru_back; hb; hb = hb->lru_next)
        {
            if (hb->ph.this_bucket == *bucketpp)
            {
                yaz_log(YLOG_FATAL, "Found hash bucket on other chain");
                return 0;
            }
        }
#endif
        (cf->no_miss)++;
        hb = get_bucket(cf, *bucketpp, hno);
        if (!hb)
            return 0;
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
    hb = new_bucket(cf, bucketpp, hno);
    if (!hb)
        return 0;

    hb->ph.no[0] = no;
    hb->ph.vno[0] = vno;
    return vno;
}

zint cf_new(CFile cf, zint no)
{
    if (cf->head.state > 1)
        return cf_new_flat(cf, no);
    if (cf->no_miss*2 > cf->no_hits)
    {
        if (cf_moveto_flat(cf))
            return -1;
        assert(cf->head.state > 1);
        return cf_new_flat(cf, no);
    }
    return cf_new_hash(cf, no);
}


/** \brief reads block from commit area
    \param cf commit file
    \param no block number
    \param offset offset in block
    \param nbytes number of bytes to read
    \param buf buffer for content (if read was succesful)
    \retval 0 block could not be fully read
    \retval 1 block could be read
    \retval -1 error
*/
int cf_read(CFile cf, zint no, int offset, int nbytes, void *buf)
{
    zint block;
    int ret;

    assert(cf);
    zebra_mutex_lock(&cf->mutex);
    ret = cf_lookup(cf, no, &block);
    zebra_mutex_unlock(&cf->mutex);
    if (ret == -1)
    {
        /* error */
        yaz_log(YLOG_FATAL, "cf_lookup failed");
        return -1;
    }
    else if (ret == 0)
    {
        /* block could not be read */
        return ret;
    }
    else if (mf_read(cf->block_mf, block, offset, nbytes, buf) != 1)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "mf_read no=" ZINT_FORMAT " block=" ZINT_FORMAT, no, block);
        return -1;
    }
    return 1;
}

/** \brief writes block to commit area
    \param cf commit file
    \param no block number
    \param offset offset in block
    \param nbytes number of bytes to be written
    \param buf buffer to be written
    \retval 0 block written
    \retval -1 error
*/
int cf_write(CFile cf, zint no, int offset, int nbytes, const void *buf)
{
    zint block;
    int ret;

    assert(cf);
    zebra_mutex_lock(&cf->mutex);

    ret = cf_lookup(cf, no, &block);

    if (ret == -1)
    {
        zebra_mutex_unlock(&cf->mutex);
        return ret;
    }
    if (ret == 0)
    {
        block = cf_new(cf, no);
        if (!block)
        {
            zebra_mutex_unlock(&cf->mutex);
            return -1;
        }
        if (offset || nbytes)
        {
            if (mf_read(cf->rmf, no, 0, 0, cf->iobuf) == -1)
                return -1;
            memcpy(cf->iobuf + offset, buf, nbytes);
            buf = cf->iobuf;
            offset = 0;
            nbytes = 0;
        }
    }
    zebra_mutex_unlock(&cf->mutex);
    return mf_write(cf->block_mf, block, offset, nbytes, buf);
}

int cf_close(CFile cf)
{
    int ret = 0;
    yaz_log(YLOG_DEBUG, "cf: close hits=%d miss=%d bucket_in_memory=" ZINT_FORMAT
	  " total=" ZINT_FORMAT,
          cf->no_hits, cf->no_miss, cf->bucket_in_memory,
          cf->head.next_bucket - cf->head.first_bucket);
    if (flush_bucket(cf, -1))
        ret = -1;
    if (cf->hash_mf)
    {
        if (cf->dirty)
        {
            if (mf_write(cf->hash_mf, 0, 0, sizeof(cf->head), &cf->head))
                ret = -1;
            if (write_head(cf))
                ret = -1;
        }
        mf_close(cf->hash_mf);
    }
    if (cf->block_mf)
        mf_close(cf->block_mf);
    xfree(cf->array);
    xfree(cf->parray);
    xfree(cf->iobuf);
    zebra_mutex_destroy(&cf->mutex);
    xfree(cf);
    return ret;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

