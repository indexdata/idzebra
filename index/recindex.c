/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.c,v $
 * Revision 1.2  1995-11-15 19:13:08  adam
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

char *rec_strdup (const char *s)
{
    char *p;

    if (!s)
        return NULL;
    p = malloc (strlen(s)+1);
    if (!p)
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    strcpy (p, s);
    return p;
}

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
    p->cache_max = 100;
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
            size++;
        else
            size += strlen(rec->info[i])+1;
    
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
        if (!rec->info[i])
            *cptr++ = '\0';
        else
        {
            strcpy (cptr, rec->info[i]);
            cptr += strlen(rec->info[i]) + 1;
        }
    for (got = 0; got < entry.u.used.size; got += r)
    {
        r = write (p->data_fd, p->tmp_buf + got, entry.u.used.size - got);
        if (r <= 0)
        {
            logf (LOG_FATAL|LOG_ERRNO, "write of %s", p->data_fname);
            exit (1);
        }
        got += r;
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
        rec_rm (e->rec);
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
    e->dirty = 1;
    e->rec = rec_cp (rec);
}

void rec_close (Records *p)
{
    assert (*p);

    rec_cache_flush (*p);
    free ((*p)->record_cache);

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
        got += r;
    }
    rec->sysno = sysno;

    nptr = p->tmp_buf;
    for (i = 0; i < REC_NO_INFO; i++)
        if (*nptr)
        {
            rec->info[i] = rec_strdup (nptr);
            nptr += strlen(nptr)+1;
        }
        else
        {
            nptr++;
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
        rec->info[i] = NULL;
    rec_cache_insert (p, rec, 1);
    return rec;
}

void rec_put (Records p, Record rec)
{
    Record *recp;

    if ((recp = rec_cache_lookup (p, rec->sysno, 1)))
    {
        rec_rm (*recp);
        *recp = rec_cp (rec);
    }
    else
        rec_cache_insert (p, rec, 1);
}

void rec_rm (Record rec)
{
    int i;
    for (i = 0; i < REC_NO_INFO; i++)
        free (rec->info[i]);
    free (rec);
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
        n->info[i] = rec_strdup (rec->info[i]);
    return n;
}
