/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.c,v $
 * Revision 1.11  1995-12-06 13:58:26  adam
 * Improved flushing of records - all flushes except the last one
 * don't write the last accessed. Also flush takes place if record
 * info occupy more than about 256k.
 *
 * Revision 1.10  1995/12/06  12:41:24  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.9  1995/11/30  08:34:33  adam
 * Started work on commit facility.
 * Changed a few malloc/free to xmalloc/xfree.
 *
 * Revision 1.8  1995/11/28  14:26:21  adam
 * Bug fix: recordId with constant wasn't right.
 * Bug fix: recordId dictionary entry wasn't deleted when needed.
 *
 * Revision 1.7  1995/11/28  09:09:43  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.6  1995/11/25  10:24:06  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.5  1995/11/22  17:19:18  adam
 * Record management uses the bfile system.
 *
 * Revision 1.4  1995/11/20  16:59:46  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.3  1995/11/16  15:34:55  adam
 * Uses new record management system in both indexer and server.
 *
 * Revision 1.2  1995/11/15  19:13:08  adam
 * Work on record management.
 *
 * Revision 1.1  1995/11/15  14:46:20  adam
 * Started work on better record management system.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "recindxp.h"

static void rec_write_head (Records p)
{
    int r;

    assert (p);
    assert (p->index_BFile);

    r = bf_write (p->index_BFile, 0, 0, sizeof(p->head), &p->head);    
    if (r)
    {
        logf (LOG_FATAL|LOG_ERRNO, "write head of %s", p->index_fname);
        exit (1);
    }
}

static void rec_tmp_expand (Records p, int size, int dst_type)
{
    if (p->tmp_size < size + 256 ||
        p->tmp_size < p->head.block_size[dst_type]*2)
    {
        xfree (p->tmp_buf);
        p->tmp_size = size + p->head.block_size[dst_type]*2 + 256;
        p->tmp_buf = xmalloc (p->tmp_size);
    }
}

static int read_indx (Records p, int sysno, void *buf, int itemsize, 
                      int ignoreError)
{
    int r;
    int pos = (sysno-1)*itemsize;

    r = bf_read (p->index_BFile, 1+pos/128, pos%128, itemsize, buf);
    if (r != 1 && !ignoreError)
    {
        logf (LOG_FATAL|LOG_ERRNO, "read in %s at pos %ld",
              p->index_fname, (long) pos);
        abort ();
        exit (1);
    }
    return r;
}

static void write_indx (Records p, int sysno, void *buf, int itemsize)
{
    int pos = (sysno-1)*itemsize;

    bf_write (p->index_BFile, 1+pos/128, pos%128, itemsize, buf);
}

static void rec_release_blocks (Records p, int sysno)
{
    struct record_index_entry entry;
    int freeblock, freenext;
    int dst_type;

    if (read_indx (p, sysno, &entry, sizeof(entry), 1) != 1)
        return ;
    p->head.total_bytes -= entry.u.used.size;
    freeblock = entry.u.used.next;
    assert (freeblock > 0);
    dst_type = freeblock & 7;
    assert (dst_type < REC_BLOCK_TYPES);
    freeblock = freeblock / 8;
    while (freeblock)
    {
        if (bf_read (p->data_BFile[dst_type], freeblock, 0, sizeof(freenext),
                     &freenext) != 1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "read in rec_del_single");
            exit (1);
        }
        if (bf_write (p->data_BFile[dst_type], freeblock, 0, sizeof(freenext),
                      &p->head.block_free[dst_type]))
        {
            logf (LOG_FATAL|LOG_ERRNO, "write in rec_del_single");
            exit (1);
        }
        p->head.block_free[dst_type] = freeblock;
        freeblock = freenext;
        p->head.block_used[dst_type]--;
    }
}

static void rec_delete_single (Records p, Record rec)
{
    struct record_index_entry entry;

    rec_release_blocks (p, rec->sysno);

    entry.u.free.next = p->head.index_free;
    p->head.index_free = rec->sysno;
    write_indx (p, rec->sysno, &entry, sizeof(entry));
}


static void rec_write_single (Records p, Record rec)
{
    int i, size = 0;
    char *cptr;
    int dst_type = 0;
    int no_written = 0;
    int block_prev = -1, block_free;
    struct record_index_entry entry;

    for (i = 0; i < REC_NO_INFO; i++)
        if (!rec->info[i])
            size += sizeof(*rec->size);
        else
            size += sizeof(*rec->size) + rec->size[i];

    for (i = 1; i<REC_BLOCK_TYPES; i++)
        if (size >= p->head.block_move[i])
            dst_type = i;

    rec_tmp_expand (p, size, dst_type);

    cptr = p->tmp_buf + sizeof(int);           /* a hack! */
    for (i = 0; i < REC_NO_INFO; i++)
    {
        memcpy (cptr, &rec->size[i], sizeof(*rec->size));
        cptr += sizeof(*rec->size);
        if (rec->info[i])
        {
            memcpy (cptr, rec->info[i], rec->size[i]);
            cptr += rec->size[i];
        }
    }
    cptr = p->tmp_buf;
    while (no_written < size)
    {
        block_free = p->head.block_free[dst_type];
        if (block_free)
        {
            if (bf_read (p->data_BFile[dst_type],
                         block_free, 0, sizeof(*p->head.block_free),
                         &p->head.block_free[dst_type]) != 1)
            {
                logf (LOG_FATAL|LOG_ERRNO, "read in %s at free block %d",
                      p->data_fname[dst_type], block_free);
            }
        }
        else
            block_free = p->head.block_last[dst_type]++;
        if (block_prev == -1)
        {
            entry.u.used.next = block_free*8 + dst_type;
            entry.u.used.size = size;
            p->head.total_bytes += size;
            write_indx (p, rec->sysno, &entry, sizeof(entry));
        }
        else
        {
            memcpy (cptr, &block_free, sizeof(int));
            bf_write (p->data_BFile[dst_type], block_prev, 0, 0, cptr);
            cptr = p->tmp_buf + no_written;
        }
        block_prev = block_free;
        no_written += p->head.block_size[dst_type] - sizeof(int);
        p->head.block_used[dst_type]++;
    }
    assert (block_prev != -1);
    block_free = 0;
    memcpy (cptr, &block_free, sizeof(int));
    bf_write (p->data_BFile[dst_type], block_prev, 0,
              sizeof(int) + (p->tmp_buf+size) - cptr, cptr);
}

static void rec_update_single (Records p, Record rec)
{
    rec_release_blocks (p, rec->sysno);
    rec_write_single (p, rec);
}

Records rec_open (int rw)
{
    Records p;
    int i, r;

    p = xmalloc (sizeof(*p));
    p->rw = rw;
    p->tmp_size = 1024;
    p->tmp_buf = xmalloc (p->tmp_size);
    p->index_fname = "recindex";
    p->index_BFile = bf_open (p->index_fname, 128, rw);
    if (p->index_BFile == NULL)
    {
        logf (LOG_FATAL|LOG_ERRNO, "open %s", p->index_fname);
        exit (1);
    }
    r = bf_read (p->index_BFile, 0, 0, 0, p->tmp_buf);
    switch (r)
    {
    case 0:
        memcpy (p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic));
        p->head.index_free = 0;
        p->head.index_last = 1;
        p->head.no_records = 0;
        p->head.total_bytes = 0;
        for (i = 0; i<REC_BLOCK_TYPES; i++)
        {
            p->head.block_free[i] = 0;
            p->head.block_last[i] = 1;
            p->head.block_used[i] = 0;
        }
        p->head.block_size[0] = 128;
        p->head.block_move[0] = 0;
        for (i = 1; i<REC_BLOCK_TYPES; i++)
        {
            p->head.block_size[i] = p->head.block_size[i-1] * 4;
            p->head.block_move[i] = p->head.block_size[i] * 3;
        }
        if (rw)
            rec_write_head (p);
        break;
    case 1:
        memcpy (&p->head, p->tmp_buf, sizeof(p->head));
        if (memcmp (p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic)))
        {
            logf (LOG_FATAL, "read %s. bad header", p->index_fname);
            exit (1);
        }
        break;
    }
    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        char str[80];
        sprintf (str, "recdata%c", i + 'A');
        p->data_fname[i] = xmalloc (strlen(str)+1);
        strcpy (p->data_fname[i], str);
        p->data_BFile[i] = NULL;
    }
    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (!(p->data_BFile[i] = bf_open (p->data_fname[i],
                                          p->head.block_size[i],
                                          rw)))
        {
            logf (LOG_FATAL|LOG_ERRNO, "bf_open %s", p->data_fname[i]);
            exit (1);
        }
    }
    p->cache_max = 10;
    p->cache_cur = 0;
    p->record_cache = xmalloc (sizeof(*p->record_cache)*p->cache_max);
    return p;
}

static void rec_cache_flush (Records p, int saveCount)
{
    int i, j;

    if (saveCount >= p->cache_cur)
        saveCount = 0;
    for (i = 0; i<p->cache_cur - saveCount; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        switch (e->flag)
        {
        case recordFlagNop:
            break;
        case recordFlagNew:
            rec_write_single (p, e->rec);
            break;
        case recordFlagWrite:
            rec_update_single (p, e->rec);
            break;
        case recordFlagDelete:
            rec_delete_single (p, e->rec);
            break;
        }
        rec_rm (&e->rec);
    }
    for (j = 0; j<saveCount; j++, i++)
        memcpy (p->record_cache+j, p->record_cache+i,
                sizeof(*p->record_cache));
    p->cache_cur = saveCount;
}

static Record *rec_cache_lookup (Records p, int sysno,
                                 enum recordCacheFlag flag)
{
    int i;
    for (i = 0; i<p->cache_cur; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        if (e->rec->sysno == sysno)
        {
            if (flag != recordFlagNop && e->flag == recordFlagNop)
                e->flag = flag;
            return &e->rec;
        }
    }
    return NULL;
}

static void rec_cache_insert (Records p, Record rec, enum recordCacheFlag flag)
{
    struct record_cache_entry *e;

    if (p->cache_cur == p->cache_max)
        rec_cache_flush (p, 1);
    else if (p->cache_cur > 2)
    {
        int i, j;
        int used = 0;
        for (i = 0; i<p->cache_cur; i++)
        {
            Record r = (p->record_cache + i)->rec;
            for (j = 0; j<REC_NO_INFO; j++)
                used += r->size[j];
        }
        if (used > 256000)
            rec_cache_flush (p, 1);
    }
    assert (p->cache_cur < p->cache_max);

    e = p->record_cache + (p->cache_cur)++;
    e->flag = flag;
    e->rec = rec_cp (rec);
}

void rec_close (Records *pp)
{
    Records p = *pp;
    int i;

    assert (p);

    rec_cache_flush (p, 0);
    xfree (p->record_cache);

    if (p->rw)
        rec_write_head (p);

    if (p->index_BFile)
        bf_close (p->index_BFile);

    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (p->data_BFile[i])
            bf_close (p->data_BFile[i]);
        xfree (p->data_fname[i]);
    }
    xfree (p->tmp_buf);
    xfree (p);
    *pp = NULL;
}


Record rec_get (Records p, int sysno)
{
    int i;
    Record rec, *recp;
    struct record_index_entry entry;
    int freeblock, dst_type;
    char *nptr, *cptr;

    assert (sysno > 0);
    assert (p);

    if ((recp = rec_cache_lookup (p, sysno, recordFlagNop)))
        return rec_cp (*recp);

    read_indx (p, sysno, &entry, sizeof(entry), 0);

    dst_type = entry.u.used.next & 7;
    assert (dst_type < REC_BLOCK_TYPES);
    freeblock = entry.u.used.next / 8;

    assert (freeblock > 0);
    
    rec = xmalloc (sizeof(*rec));
    rec_tmp_expand (p, entry.u.used.size, dst_type);

    cptr = p->tmp_buf;
    bf_read (p->data_BFile[dst_type], freeblock, 0, 0, cptr);
    memcpy (&freeblock, cptr, sizeof(freeblock));

    while (freeblock)
    {
        int tmp;

        cptr += p->head.block_size[dst_type] - sizeof(freeblock);
        
        memcpy (&tmp, cptr, sizeof(tmp));
        bf_read (p->data_BFile[dst_type], freeblock, 0, 0, cptr);
        memcpy (&freeblock, cptr, sizeof(freeblock));
        memcpy (cptr, &tmp, sizeof(tmp));
    }

    rec->sysno = sysno;
    nptr = p->tmp_buf + sizeof(freeblock);
    for (i = 0; i < REC_NO_INFO; i++)
    {
        memcpy (&rec->size[i], nptr, sizeof(*rec->size));
        nptr += sizeof(*rec->size);
        if (rec->size[i])
        {
            rec->info[i] = xmalloc (rec->size[i]);
            memcpy (rec->info[i], nptr, rec->size[i]);
            nptr += rec->size[i];
        }
        else
            rec->info[i] = NULL;
    }
    rec_cache_insert (p, rec, recordFlagNop);
    return rec;
}

Record rec_new (Records p)
{
    int sysno, i;
    Record rec;

    assert (p);
    rec = xmalloc (sizeof(*rec));
    if (p->head.index_free == 0)
        sysno = (p->head.index_last)++;
    else
    {
        struct record_index_entry entry;

        read_indx (p, p->head.index_free, &entry, sizeof(entry), 0);
        sysno = p->head.index_free;
        p->head.index_free = entry.u.free.next;
    }
    (p->head.no_records)++;
    rec->sysno = sysno;
    for (i = 0; i < REC_NO_INFO; i++)
    {
        rec->info[i] = NULL;
        rec->size[i] = 0;
    }
    rec_cache_insert (p, rec, recordFlagNew);
    return rec;
}

void rec_del (Records p, Record *recpp)
{
    Record *recp;

    if ((recp = rec_cache_lookup (p, (*recpp)->sysno, recordFlagDelete)))
    {
        rec_rm (recp);
        *recp = *recpp;
    }
    else
    {
        rec_cache_insert (p, *recpp, recordFlagDelete);
        rec_rm (recpp);
    }
    *recpp = NULL;
}

void rec_put (Records p, Record *recpp)
{
    Record *recp;

    if ((recp = rec_cache_lookup (p, (*recpp)->sysno, recordFlagWrite)))
    {
        rec_rm (recp);
        *recp = *recpp;
    }
    else
    {
        rec_cache_insert (p, *recpp, recordFlagWrite);
        rec_rm (recpp);
    }
    *recpp = NULL;
}

void rec_rm (Record *recpp)
{
    int i;
    for (i = 0; i < REC_NO_INFO; i++)
        xfree ((*recpp)->info[i]);
    xfree (*recpp);
    *recpp = NULL;
}

Record rec_cp (Record rec)
{
    Record n;
    int i;

    n = xmalloc (sizeof(*n));
    n->sysno = rec->sysno;
    for (i = 0; i < REC_NO_INFO; i++)
        if (!rec->info[i])
        {
            n->info[i] = NULL;
            n->size[i] = 0;
        }
        else
        {
            n->size[i] = rec->size[i];
            n->info[i] = xmalloc (rec->size[i]);
            memcpy (n->info[i], rec->info[i], rec->size[i]);
        }
    return n;
}


char *rec_strdup (const char *s, size_t *len)
{
    char *p;

    if (!s)
    {
        *len = 0;
        return NULL;
    }
    *len = strlen(s)+1;
    p = xmalloc (*len);
    strcpy (p, s);
    return p;
}

