/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.c,v $
 * Revision 1.8  1995-11-28 14:26:21  adam
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
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "recindex.h"

#define USE_BF 1

#if USE_BF
#include <bfile.h>

#define REC_BLOCK_TYPES 2
#define REC_HEAD_MAGIC "recindx"

struct records_info {
    int rw;

    char *index_fname;
    BFile index_BFile;


    char *data_fname[REC_BLOCK_TYPES];
    BFile data_BFile[REC_BLOCK_TYPES];

    char *tmp_buf;
    int tmp_size;

    struct record_cache_entry *record_cache;
    int cache_size;
    int cache_cur;
    int cache_max;

    struct records_head {
        char magic[8];
        int block_size[REC_BLOCK_TYPES];
        int block_free[REC_BLOCK_TYPES];
        int block_last[REC_BLOCK_TYPES];
        int block_used[REC_BLOCK_TYPES];
        int block_move[REC_BLOCK_TYPES];

        int index_last;
        int index_free;
        int no_records;

    } head;
};

enum recordCacheFlag { recordFlagNop, recordFlagWrite, recordFlagNew,
                       recordFlagDelete };

struct record_cache_entry {
    Record rec;
    enum recordCacheFlag flag;
};

struct record_index_entry {
    union {
        struct {
            int next;
            int size;
        } used;
        struct {
            int next;
        } free;
    } u;
};


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
        free (p->tmp_buf);
        p->tmp_size = size + p->head.block_size[dst_type]*2 +
            256;
        if (!(p->tmp_buf = malloc (p->tmp_size)))
        {
            logf (LOG_FATAL|LOG_ERRNO, "malloc");
            exit (1);
        }
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

    if (!(p = malloc (sizeof(*p))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    p->rw = rw;
    p->tmp_size = 1024;
    p->tmp_buf = malloc (p->tmp_size);
    if (!p->tmp_buf)
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
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
        p->data_fname[i] = malloc (strlen(str)+1);
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
    if (!(p->record_cache = malloc (sizeof(*p->record_cache)*p->cache_max)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    return p;
}

static void rec_cache_flush (Records p)
{
    int i;
    for (i = 0; i<p->cache_cur; i++)
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
    p->cache_cur = 0;
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
        rec_cache_flush (p);
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

    rec_cache_flush (p);
    free (p->record_cache);

    if (p->rw)
        rec_write_head (p);

    if (p->index_BFile)
        bf_close (p->index_BFile);

    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (p->data_BFile[i])
            bf_close (p->data_BFile[i]);
        free (p->data_fname[i]);
    }
    free (p->tmp_buf);
    free (p);
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
    
    if (!(rec = malloc (sizeof(*rec))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
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
            rec->info[i] = malloc (rec->size[i]);
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
    if (!(rec = malloc (sizeof(*rec))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
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
        free ((*recpp)->info[i]);
    free (*recpp);
    *recpp = NULL;
}

Record rec_cp (Record rec)
{
    Record n;
    int i;

    if (!(n = malloc (sizeof(*n))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
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
            if (!(n->info[i] = malloc (rec->size[i])))
            {
                logf (LOG_FATAL|LOG_ERRNO, "malloc. rec_cp");
                exit (1);
            }
            memcpy (n->info[i], rec->info[i], rec->size[i]);
        }
    return n;
}

/* no BF --------------------------------------------------- */
#else

struct records_info {
    int rw;
    int index_fd;
    char *index_fname;
    int data_fd;
    char *data_fname;
    struct records_head {
        char magic[8];
	int no_records;
        int index_free;
        int index_last;
        int data_size;
        int data_slack;
        int data_used;
    } head;
    char *tmp_buf;
    int tmp_size;
    int cache_size;
    int cache_cur;
    int cache_max;
    struct record_cache_entry *record_cache;
};

struct record_cache_entry {
    Record rec;
    int dirty;
};

struct record_index_entry {
    union {
        struct {
            int offset;
            int size;
        } used;
        struct {
            int next;
        } free;
    } u;
};

#define REC_HEAD_MAGIC "rechead"

static void rec_write_head (Records p)
{
    int r;

    assert (p);
    assert (p->index_fd != -1);
    if (lseek (p->index_fd, (off_t) 0, SEEK_SET) == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "lseek to 0 in %s", p->index_fname);
        exit (1);
    }
    r = write (p->index_fd, &p->head, sizeof(p->head));    
    switch (r)
    {
    case -1:
        logf (LOG_FATAL|LOG_ERRNO, "write head of %s", p->index_fname);
        exit (1);
    case sizeof(p->head):
        break;
    default:
        logf (LOG_FATAL, "write head of %s. wrote %d", p->index_fname, r);
        exit (1);
    }
}

Records rec_open (int rw)
{
    Records p;
    int r;

    if (!(p = malloc (sizeof(*p))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    p->rw = rw;
    p->tmp_buf = NULL;
    p->tmp_size = 0;
    p->data_fname = "recdata";
    p->data_fd = -1;
    p->index_fname = "recindex";
    p->index_fd = open (p->index_fname,
                        rw ? (O_RDWR|O_CREAT) : O_RDONLY, 0666);
    if (p->index_fd == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "open %s", p->index_fname);
        exit (1);
    }
    r = read (p->index_fd, &p->head, sizeof(p->head));
    switch (r)
    {
    case -1:
        logf (LOG_FATAL|LOG_ERRNO, "read %s", p->index_fname);
        exit (1);
    case 0:
        memcpy (p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic));
        p->head.index_free = 0;
        p->head.index_last = 1;
        p->head.no_records = 0;
        p->head.data_size = 0;
        p->head.data_slack = 0;
        p->head.data_used = 0;
        if (rw)
            rec_write_head (p);
        break;
    case sizeof(p->head):
        if (memcmp (p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic)))
        {
            logf (LOG_FATAL, "read %s. bad header", p->index_fname);
            exit (1);
        }
        break;
    default:
        logf (LOG_FATAL, "read head of %s. expected %d. got %d",
	      p->index_fname, sizeof(p->head), r);
        exit (1);
    }
    p->data_fd = open (p->data_fname,
                       rw ? (O_RDWR|O_CREAT) : O_RDONLY, 0666);
    if (p->data_fd == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "open %s", p->data_fname);
        exit (1);
    }
    p->cache_max = 10;
    p->cache_cur = 0;
    if (!(p->record_cache = malloc (sizeof(*p->record_cache)*p->cache_max)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    return p;
}

static void read_indx (Records p, int sysno, void *buf, int itemsize)
{
    int r;
    off_t pos = (sysno-1)*itemsize + sizeof(p->head);

    if (lseek (p->index_fd, pos, SEEK_SET) == (pos) -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "seek in %s to pos %ld",
              p->index_fname, (long) pos);
        exit (1);
    }
    r = read (p->index_fd, buf, itemsize);
    if (r != itemsize)
    {
        if (r == -1)
            logf (LOG_FATAL|LOG_ERRNO, "read in %s at pos %ld",
                  p->index_fname, (long) pos);
        else
            logf (LOG_FATAL, "read in %s at pos %ld",
                  p->index_fname, (long) pos);
        exit (1);
    }
}

static void rec_write_single (Records p, Record rec)
{
    struct record_index_entry entry;
    int r, i, size = 0, got;
    char *cptr;
    off_t pos = (rec->sysno-1)*sizeof(entry) + sizeof(p->head);

    for (i = 0; i < REC_NO_INFO; i++)
        if (!rec->info[i])
            size += sizeof(*rec->size);
        else
            size += sizeof(*rec->size) + rec->size[i];
    
    entry.u.used.offset = p->head.data_size;
    entry.u.used.size = size;
    p->head.data_size += size;
    p->head.data_used += size;

    if (lseek (p->index_fd, pos, SEEK_SET) == (pos) -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "seek in %s to pos %ld",
              p->index_fname, (long) pos);
        exit (1);
    }
    r = write (p->index_fd, &entry, sizeof(entry));
    if (r != sizeof(entry))
    {
        if (r == -1)
            logf (LOG_FATAL|LOG_ERRNO, "write of %s at pos %ld",
                  p->index_fname, (long) pos);
        else
            logf (LOG_FATAL, "write of %s at pos %ld",
                  p->index_fname, (long) pos);
        exit (1);
    }
    if (lseek (p->data_fd, entry.u.used.offset, SEEK_SET) == -1) 
    {
        logf (LOG_FATAL|LOG_ERRNO, "lseek in %s to pos %ld",
              p->data_fname, entry.u.used.offset);
        exit (1);
    }
    if (p->tmp_size < entry.u.used.size) 
    {
        free (p->tmp_buf);
        p->tmp_size = entry.u.used.size + 16384;
        if (!(p->tmp_buf = malloc (p->tmp_size)))
        {
            logf (LOG_FATAL|LOG_ERRNO, "malloc");
            exit (1);
        }
    }
    cptr = p->tmp_buf;
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
    for (got = 0; got < entry.u.used.size; got += r)
    {
        r = write (p->data_fd, p->tmp_buf + got, entry.u.used.size - got);
        if (r <= 0)
        {
            logf (LOG_FATAL|LOG_ERRNO, "write of %s", p->data_fname);
            exit (1);
        }
    }
}

static void rec_cache_flush (Records p)
{
    int i;
    for (i = 0; i<p->cache_cur; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        if (e->dirty)
            rec_write_single (p, e->rec);
        rec_rm (&e->rec);
    }
    p->cache_cur = 0;
}

static Record *rec_cache_lookup (Records p, int sysno, int dirty)
{
    int i;
    for (i = 0; i<p->cache_cur; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        if (e->rec->sysno == sysno)
        {
            if (dirty)
                e->dirty = 1;
            return &e->rec;
        }
    }
    return NULL;
}

static void rec_cache_insert (Records p, Record rec, int dirty)
{
    struct record_cache_entry *e;

    if (p->cache_cur == p->cache_max)
        rec_cache_flush (p);
    assert (p->cache_cur < p->cache_max);

    e = p->record_cache + (p->cache_cur)++;
    e->dirty = dirty;
    e->rec = rec_cp (rec);
}

void rec_close (Records *p)
{
    assert (*p);

    rec_cache_flush (*p);
    free ((*p)->record_cache);

    if ((*p)->rw)
        rec_write_head (*p);

    if ((*p)->index_fd != -1)
        close ((*p)->index_fd);

    if ((*p)->data_fd != -1)
        close ((*p)->data_fd);

    free ((*p)->tmp_buf);

    free (*p);
    *p = NULL;
}

Record rec_get (Records p, int sysno)
{
    int i;
    Record rec, *recp;
    struct record_index_entry entry;
    int r, got;
    char *nptr;

    assert (sysno > 0);
    assert (p);

    if ((recp = rec_cache_lookup (p, sysno, 0)))
        return rec_cp (*recp);

    read_indx (p, sysno, &entry, sizeof(entry));
    
    if (!(rec = malloc (sizeof(*rec))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    if (lseek (p->data_fd, entry.u.used.offset, SEEK_SET) == -1) 
    {
        logf (LOG_FATAL|LOG_ERRNO, "lseek in %s to pos %ld",
              p->data_fname, entry.u.used.offset);
        exit (1);
    }
    if (p->tmp_size < entry.u.used.size) 
    {
        free (p->tmp_buf);
        p->tmp_size = entry.u.used.size + 16384;
        if (!(p->tmp_buf = malloc (p->tmp_size)))
        {
            logf (LOG_FATAL|LOG_ERRNO, "malloc");
            exit (1);
        }
    }
    for (got = 0; got < entry.u.used.size; got += r)
    {
        r = read (p->data_fd, p->tmp_buf + got, entry.u.used.size - got);
        if (r <= 0)
        {
            logf (LOG_FATAL|LOG_ERRNO, "read of %s", p->data_fname);
            exit (1);
        }
    }
    rec->sysno = sysno;

    nptr = p->tmp_buf;
    for (i = 0; i < REC_NO_INFO; i++)
    {
        memcpy (&rec->size[i], nptr, sizeof(*rec->size));
        nptr += sizeof(*rec->size);
        if (rec->size[i])
        {
            rec->info[i] = malloc (rec->size[i]);
            memcpy (rec->info[i], nptr, rec->size[i]);
            nptr += rec->size[i];
        }
        else
            rec->info[i] = NULL;
    }
    rec_cache_insert (p, rec, 0);
    return rec;
}

Record rec_new (Records p)
{
    int sysno, i;
    Record rec;

    assert (p);
    if (!(rec = malloc (sizeof(*rec))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    if (p->head.index_free == 0)
        sysno = (p->head.index_last)++;
    else
    {
        struct record_index_entry entry;

        read_indx (p, p->head.index_free, &entry, sizeof(entry));
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
    rec_cache_insert (p, rec, 1);
    return rec;
}

void rec_put (Records p, Record *recpp)
{
    Record *recp;

    if ((recp = rec_cache_lookup (p, (*recpp)->sysno, 1)))
    {
        rec_rm (recp);
        *recp = *recpp;
    }
    else
    {
        rec_cache_insert (p, *recpp, 1);
        rec_rm (recpp);
    }
    *recpp = NULL;
}

void rec_rm (Record *recpp)
{
    int i;
    for (i = 0; i < REC_NO_INFO; i++)
        free ((*recpp)->info[i]);
    free (*recpp);
    *recpp = NULL;
}

Record rec_cp (Record rec)
{
    Record n;
    int i;

    if (!(n = malloc (sizeof(*n))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
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
            if (!(n->info[i] = malloc (rec->size[i])))
            {
                logf (LOG_FATAL|LOG_ERRNO, "malloc. rec_cp");
                exit (1);
            }
            memcpy (n->info[i], rec->info[i], rec->size[i]);
        }
    return n;
}

void rec_del (Records p, Record *recpp)
{
    assert (0);
}


#endif

char *rec_strdup (const char *s, size_t *len)
{
    char *p;

    if (!s)
    {
        *len = 0;
        return NULL;
    }
    *len = strlen(s)+1;
    p = malloc (*len);
    if (!p)
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    strcpy (p, s);
    return p;
}

