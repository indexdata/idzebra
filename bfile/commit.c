/*
 * Copyright (C) 1995-1998, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: commit.c,v $
 * Revision 1.15  1999-05-26 07:49:12  adam
 * C++ compilation.
 *
 * Revision 1.14  1998/08/07 15:07:16  adam
 * Fixed but in cf_commit_flat.
 *
 * Revision 1.13  1996/10/29 13:56:16  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.12  1996/04/24 13:29:16  adam
 * Work on optimized on commit operation.
 *
 * Revision 1.11  1996/04/23  12:36:41  adam
 * Started work on more efficient commit operation.
 *
 * Revision 1.10  1996/04/18  16:02:56  adam
 * Changed logging a bit.
 * Removed warning message when commiting flat shadow files.
 *
 * Revision 1.9  1996/04/12  07:01:57  adam
 * Yet another bug fix (next_block was initialized to 0; now set to 1).
 *
 * Revision 1.8  1996/02/07 14:03:49  adam
 * Work on flat indexed shadow files.
 *
 * Revision 1.7  1996/02/07  10:08:46  adam
 * Work on flat shadow (not finished yet).
 *
 * Revision 1.6  1995/12/15  12:36:53  adam
 * Moved hash file information to union.
 * Renamed commit files.
 *
 * Revision 1.5  1995/12/12  15:57:55  adam
 * Implemented mf_unlink. cf_unlink uses mf_unlink.
 *
 * Revision 1.4  1995/12/11  09:03:55  adam
 * New function: cf_unlink.
 * New member of commit file head: state (0) deleted, (1) hash file.
 *
 * Revision 1.3  1995/12/01  16:24:29  adam
 * Commit files use separate meta file area.
 *
 * Revision 1.2  1995/12/01  11:37:24  adam
 * Cached/commit files implemented as meta-files.
 *
 * Revision 1.1  1995/11/30  08:33:13  adam
 * Started work on commit facility.
 *
 */

#include <assert.h>
#include <stdlib.h>

#include <zebrautl.h>
#include <mfile.h>
#include "cfile.h"

#define CF_OPTIMIZE_COMMIT 0

void cf_unlink (CFile cf)
{
    if (cf->bucket_in_memory)
    {
        logf (LOG_FATAL, "Cannot unlink potential dirty cache");
        exit (1);
    }
    cf->head.state = 0;
    cf->dirty = 1;
    mf_unlink (cf->block_mf);
    mf_unlink (cf->hash_mf);
}


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
            logf (LOG_FATAL, "read commit block at position %d",
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
            logf (LOG_FATAL, "read commit hash");
            exit (1);
        }
        for (i = 0; i<HASH_BUCKET && p->vno[i]; i++)
        {
#if CF_OPTIMIZE_COMMIT
            map_cache_add (m_p, p->vno[i], p->no[i]);
#else
            if (!mf_read (cf->block_mf, p->vno[i], 0, 0, cf->iobuf))
            {
                logf (LOG_FATAL, "read commit block");
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
            logf (LOG_FATAL, "read index block hno=%d (%d-%d) commit",
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
                    logf (LOG_FATAL, "read data block hno=%d (%d-%d) "
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
        logf (LOG_FATAL, "Cannot commit potential dirty cache");
        exit (1);
    }
    if (cf->head.state == 1)
        cf_commit_hash (cf);
    else if (cf->head.state == 2)
        cf_commit_flat (cf);
}

