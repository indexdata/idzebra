/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rstemp.c,v $
 * Revision 1.4  1995-09-05 11:43:24  adam
 * Complete version of temporary sets. Not tested yet though.
 *
 * Revision 1.3  1995/09/04  15:20:40  adam
 * More work on temp sets. is_open member removed.
 *
 * Revision 1.2  1995/09/04  09:10:56  adam
 * Minor changes.
 *
 * Revision 1.1  1994/11/04  13:21:30  quinn
 * Working.
 *
 */

#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

#include <alexutil.h>
#include <rstemp.h>

static struct rset_control *r_create(const struct rset_control *sel, 
                                     void *parms);
static int r_open(struct rset_control *ct, int wflag);
static void r_close(struct rset_control *ct);
static void r_delete(struct rset_control *ct);
static void r_rewind(struct rset_control *ct);
static int r_count(struct rset_control *ct);
static int r_read (rset_control *ct, void *buf);
static int r_write (rset_control *ct, const void *buf);

static const rset_control control = 
{
    "Temporary set",
    0,
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write
};

const rset_control *rset_kind_temp = &control;

struct rset_temp_private {
    int     fd;
    char   *fname;
    size_t  key_size;
    char   *buf_mem;
    size_t  buf_size;
    size_t  pos_end;
    size_t  pos_cur;
    size_t  pos_buf;
    size_t  pos_border;
    int     dirty;
};

static struct rset_control *r_create(const struct rset_control *sel,
                                     void *parms)
{
    rset_control *newct;
    rset_temp_parms *temp_parms = parms;
    struct rset_temp_private *info;
    
    logf (LOG_DEBUG, "ritemp_create(%s)", sel->desc);
    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));
    newct->buf = xmalloc (sizeof(struct rset_temp_private));
    info = newct->buf;

    info->fd = -1;
    info->fname = NULL;
    info->key_size = temp_parms->key_size;
    info->buf_size = 1024;
    info->buf_mem = xmalloc (info->buf_size);
    info->pos_cur = 0;
    info->pos_end = 0;
    info->pos_buf = 0;
    info->dirty = 0;

    return newct;
}

static int r_open(struct rset_control *ct, int wflag)
{
    struct rset_temp_private *info = ct->buf;

    assert (info->fd == -1);
    if (info->fname)
    {
        if (wflag)
            info->fd = open (info->fname, O_RDWR|O_CREAT, 0666);
        else
            info->fd = open (info->fname, O_RDONLY);
        if (info->fd == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "open %s", info->fname);
            exit (1);
        }
    }
    r_rewind (ct);
    return 0;
}

static void r_flush (struct rset_control *ct, int mk)
{
    struct rset_temp_private *info = ct->buf;

    if (!info->fname && mk)
    {
        char *s = (char*) tempnam (NULL, "zrs");

        info->fname = xmalloc (strlen(s)+1);
        strcpy (info->fname, s);

        info->fd = open (info->fname, O_RDWR|O_CREAT, 0666);
        if (info->fd == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "open %s", info->fname);
            exit (1);
        }
    }
    if (info->fname)
    {
        assert (info->fd != -1);
        if (info->dirty)
        {
            size_t r, count;

            if (lseek (info->fd, info->pos_buf, SEEK_SET) == -1)
            {
                logf (LOG_FATAL|LOG_ERRNO, "lseek %s", info->fname);
                exit (1);
            }
            count = info->buf_size;
            if (count > info->pos_end - info->pos_buf)
                count = info->pos_end - info->pos_buf;
            if ((r = write (info->fd, info->buf_mem, count)) < count)
            {
                if (r == -1)
                    logf (LOG_FATAL|LOG_ERRNO, "read %s", info->fname);
                else
                    logf (LOG_FATAL, "write of %ld but got %ld",
                          (long) count, (long) r);
                exit (1);
            }
            info->dirty = 0;
        }
    }
}

static void r_close (struct rset_control *ct)
{
    struct rset_temp_private *info = ct->buf;

    r_flush (ct, 0);
    if (info->fname)
    {
        assert (info->fd != -1);
        close (info->fd);
        info->fd = -1;
    }
}

static void r_delete (struct rset_control *ct)
{
    struct rset_temp_private *info = ct->buf;

    r_close (ct);
    if (info->fname)
        unlink (info->fname);        
    free (info->buf_mem);
    free (info->fname);
    free (info);
}

static void r_reread (struct rset_control *ct)
{
    struct rset_temp_private *info = ct->buf;

    if (info->fname)
    {
        size_t r, count;

        info->pos_border = info->pos_cur + info->buf_size;
        if (info->pos_border > info->pos_end)
            info->pos_border = info->pos_end;
        count = info->pos_border - info->pos_buf;
        if (count > 0)
            if ((r = read (info->fd, info->buf_mem, count)) < count)
            {
                if (r == -1)
                    logf (LOG_FATAL|LOG_ERRNO, "read %s", info->fname);
                else
                    logf (LOG_FATAL, "read of %ld but got %ld",
                          (long) count, (long) r);
                exit (1);
            }
    }
    else
        info->pos_border = info->pos_end;
}

static void r_rewind (struct rset_control *ct)
{
    struct rset_temp_private *info = ct->buf;

    r_flush (ct, 0);
    info->pos_cur = 0;
    info->pos_buf = 0;
    r_reread (ct);
}

static int r_count (struct rset_control *ct)
{
    struct rset_temp_private *info = ct->buf;

    return info->pos_end / info->key_size;
}

static int r_read (rset_control *ct, void *buf)
{
    struct rset_temp_private *info = ct->buf;
    size_t nc = info->pos_cur + info->key_size;

    if (nc > info->pos_border)
    {
        if (nc > info->pos_end)
            return 0;
        r_flush (ct, 0);
        info->pos_buf = info->pos_cur;
        r_reread (ct);
    }
    memcpy (buf, info->buf_mem + (info->pos_cur - info->pos_buf),
            info->key_size);
    info->pos_cur = nc;
    return 1;
}

static int r_write (rset_control *ct, const void *buf)
{
    struct rset_temp_private *info = ct->buf;
    size_t nc = info->pos_cur + info->key_size;

    if (nc > info->pos_buf + info->buf_size)
    {
        r_flush (ct, 1);
        info->pos_buf = info->pos_cur;
        r_reread (ct);
    }
    memcpy (info->buf_mem + (info->pos_cur - info->pos_buf), buf,
            info->key_size);
    info->dirty = 1;
    info->pos_border = info->pos_cur = nc;
    return 1;
}

