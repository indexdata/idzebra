/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rstemp.c,v $
 * Revision 1.8  1995-09-08 14:52:42  adam
 * Work on relevance feedback.
 *
 * Revision 1.7  1995/09/07  13:58:44  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.6  1995/09/06  16:11:56  adam
 * More work on boolean sets.
 *
 * Revision 1.5  1995/09/05  16:36:59  adam
 * Minor changes.
 *
 * Revision 1.4  1995/09/05  11:43:24  adam
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

static rset_control *r_create(const struct rset_control *sel, void *parms);
static RSFD r_open (rset_control *ct, int wflag);
static void r_close (RSFD rfd);
static void r_delete (rset_control *ct);
static void r_rewind (RSFD rfd);
static int r_count (rset_control *ct);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);

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

struct rset_temp_info {
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

struct rset_temp_rfd {
    struct rset_temp_info *info;
    struct rset_temp_rfd *next;
};

static struct rset_control *r_create(const struct rset_control *sel,
                                     void *parms)
{
    rset_control *newct;
    rset_temp_parms *temp_parms = parms;
    struct rset_temp_info *info;
    
    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));
    newct->buf = xmalloc (sizeof(struct rset_temp_info));
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

static RSFD r_open (struct rset_control *ct, int wflag)
{
    struct rset_temp_info *info = ct->buf;
    struct rset_temp_rfd *rfd;

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
    rfd = xmalloc (sizeof(*rfd));
    rfd->info = info;
    r_rewind (ct);
    return rfd;
}

static void r_flush (RSFD rfd, int mk)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*) rfd)->info;

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
    if (info->fname && info->fd != -1 && info->dirty)
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

static void r_close (RSFD rfd)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*)rfd)->info;

    r_flush (rfd, 0);
    if (info->fname && info->fd != -1)
    {
        close (info->fd);
        info->fd = -1;
    }
}

static void r_delete (struct rset_control *ct)
{
    struct rset_temp_info *info = ct->buf;

    r_close (ct);
    if (info->fname)
        unlink (info->fname);        
    free (info->buf_mem);
    free (info->fname);
    free (info);
}

static void r_reread (RSFD rfd)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*)rfd)->info;

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

static void r_rewind (RSFD rfd)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*)rfd)->info;

    r_flush (rfd, 0);
    info->pos_cur = 0;
    info->pos_buf = 0;
    r_reread (rfd);
}

static int r_count (struct rset_control *ct)
{
    struct rset_temp_info *info = ct->buf;

    return info->pos_end / info->key_size;
}

static int r_read (RSFD rfd, void *buf)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*)rfd)->info;

    size_t nc = info->pos_cur + info->key_size;

    if (nc > info->pos_border)
    {
        if (nc > info->pos_end)
            return 0;
        r_flush (rfd, 0);
        info->pos_buf = info->pos_cur;
        r_reread (rfd);
    }
    memcpy (buf, info->buf_mem + (info->pos_cur - info->pos_buf),
            info->key_size);
    info->pos_cur = nc;
    return 1;
}

static int r_write (RSFD rfd, const void *buf)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*)rfd)->info;

    size_t nc = info->pos_cur + info->key_size;

    if (nc > info->pos_buf + info->buf_size)
    {
        r_flush (rfd, 1);
        info->pos_buf = info->pos_cur;
        r_reread (rfd);
    }
    memcpy (info->buf_mem + (info->pos_cur - info->pos_buf), buf,
            info->key_size);
    info->dirty = 1;
    info->pos_border = info->pos_cur = nc;
    return 1;
}

