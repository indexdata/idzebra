/* $Id: commit.c,v 1.16.2.3 2006-12-05 21:14:38 adam Exp $
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
#include <stdlib.h>

#include <zebrautl.h>
#include <mfile.h>
#include "cfile.h"

#define CF_OPTIMIZE_COMMIT 0

#if CF_OPTIMIZE_COMMIT
struct map_cache_entity {
    int from;
    int to;
};

struct map_cache {
    int max;
    int no;

    struct map_cache_entity *map;
    char *buf;
    CFile cf;
};

static struct map_cache *map_cache_init (CFile cf)
{
    int mem_max = 2000000;
    struct map_cache *m_p;

    m_p = xmalloc (sizeof(*m_p));
    m_p->cf = cf;
    m_p->max = mem_max / cf->head.block_size;
    m_p->buf = xmalloc (mem_max);
    m_p->no = 0;
    m_p->map = xmalloc (sizeof(*m_p->map) * m_p->max);
    return m_p;
}

static int map_cache_cmp_from (const void *p1, const void *p2)
{
    return ((struct map_cache_entity*) p1)->from -
        ((struct map_cache_entity*) p2)->from;
}

static int map_cache_cmp_to (const void *p1, const void *p2)
{
    return ((struct map_cache_entity*) p1)->to -
        ((struct map_cache_entity*) p2)->to;
}

static void map_cache_flush (struct map_cache *m_p)
{
    int i;

    qsort (m_p->map, m_p->no, sizeof(*m_p->map), map_cache_cmp_from);
    assert (m_p->no < 2 || m_p->map[0].from < m_p->map[1].from);
    for (i = 0; i<m_p->no; i++)
    {
        if (!mf_read (m_p->cf->block_mf, m_p->map[i].from, 0, 0,
                      m_p->buf + i * m_p->cf->head.block_size))
        {
            yaz_log(YLOG_FATAL, "read commit block at position %d",
                  m_p->map[i].from);
            exit (1);
        }
        m_p->map[i].from = i;
    }
    qsort (m_p->map, m_p->no, sizeof(*m_p->map), map_cache_cmp_to);
    assert (m_p->no < 2 || m_p->map[0].to < m_p->map[1].to);
    for (i = 0; i<m_p->no; i++)
    {
        mf_write (m_p->cf->rmf, m_p->map[i].to, 0, 0,
                  m_p->buf + m_p->map[i].from * m_p->cf->head.block_size);
    }    
    m_p->no = 0;
}

static void map_cache_del (struct map_cache *m_p)
{
    map_cache_flush (m_p);
    xfree (m_p->map);
    xfree (m_p->buf);
    xfree (m_p);
}

static void map_cache_add (struct map_cache *m_p, int from, int to)
{
    int i = m_p->no;

    m_p->map[i].from = from;
    m_p->map[i].to = to;
    m_p->no = ++i;
    if (i == m_p->max)
        map_cache_flush (m_p);
}

/* CF_OPTIMIZE_COMMIT */
#endif

static void cf_commit_hash (CFile cf)
{ 
    int i, bucket_no;
    int hash_bytes;
    struct CFile_ph_bucket *p;
#if CF_OPTIMIZE_COMMIT
    struct map_cache *m_p;
#endif

#if CF_OPTIMIZE_COMMIT
    m_p = map_cache_init (cf);
#endif

    p = (struct CFile_ph_bucket *) xmalloc (sizeof(*p));
    hash_bytes = cf->head.hash_size * sizeof(int);
    bucket_no = cf->head.first_bucket;
    for (; bucket_no < cf->head.next_bucket; bucket_no++)
    {
        if (!mf_read (cf->hash_mf, bucket_no, 0, 0, p))
        {
            yaz_log(YLOG_FATAL, "read commit hash");
            exit (1);
        }
        for (i = 0; i<HASH_BUCKET && p->vno[i]; i++)
        {
#if CF_OPTIMIZE_COMMIT
            map_cache_add (m_p, p->vno[i], p->no[i]);
#else
            if (!mf_read (cf->block_mf, p->vno[i], 0, 0, cf->iobuf))
            {
                yaz_log(YLOG_FATAL, "read commit block");
                exit (1);
            }
            mf_write (cf->rmf, p->no[i], 0, 0, cf->iobuf);
#endif
        }
    }
#if CF_OPTIMIZE_COMMIT
    map_cache_del (m_p);
#endif
    xfree (p);
}

static void cf_commit_flat (CFile cf)
{
    int *fp;
    int hno;
    int i, vno = 0;

#if CF_OPTIMIZE_COMMIT
    struct map_cache *m_p;
#endif


#if CF_OPTIMIZE_COMMIT
    m_p = map_cache_init (cf);
#endif
    fp = (int *) xmalloc (HASH_BSIZE);
    for (hno = cf->head.next_bucket; hno < cf->head.flat_bucket; hno++)
    {
	for (i = 0; i < (int) (HASH_BSIZE/sizeof(int)); i++)
	    fp[i] = 0;
        if (!mf_read (cf->hash_mf, hno, 0, 0, fp) &&
            hno != cf->head.flat_bucket-1)
        {
            yaz_log(YLOG_FATAL, "read index block hno=%d (%d-%d) commit",
                  hno, cf->head.next_bucket, cf->head.flat_bucket-1);
        }
        for (i = 0; i < (int) (HASH_BSIZE/sizeof(int)); i++)
        {
            if (fp[i])
            {
#if CF_OPTIMIZE_COMMIT
                map_cache_add (m_p, fp[i], vno);
#else
                if (!mf_read (cf->block_mf, fp[i], 0, 0, cf->iobuf))
                {
                    yaz_log(YLOG_FATAL, "read data block hno=%d (%d-%d) "
                                     "i=%d commit block at %d (->%d)",
                          hno, cf->head.next_bucket, cf->head.flat_bucket-1,
                          i, fp[i], vno);
                    exit (1);
                }
                mf_write (cf->rmf, vno, 0, 0, cf->iobuf);

#endif
            }
            vno++;
        }
    }
#if CF_OPTIMIZE_COMMIT
    map_cache_del (m_p);
#endif
    xfree (fp);
}

void cf_commit (CFile cf)
{

    if (cf->bucket_in_memory)
    {
        yaz_log(YLOG_FATAL, "Cannot commit potential dirty cache");
        exit (1);
    }
    if (cf->head.state == 1)
        cf_commit_hash (cf);
    else if (cf->head.state == 2)
        cf_commit_flat (cf);
}

