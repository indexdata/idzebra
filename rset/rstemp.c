/* $Id: rstemp.c,v 1.38 2004-08-03 14:54:41 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include <fcntl.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#include <stdio.h>

#include <zebrautl.h>
#include <rstemp.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
/* static int r_count (RSET ct);*/
static int r_read (RSFD rfd, void *buf, int *term_index);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control = 
{
    "temp",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    rset_default_forward,
    rset_default_pos,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_temp = &control;

struct rset_temp_info {
    int     fd;
    char   *fname;
    size_t  key_size;      /* key size */
    char   *buf_mem;       /* window buffer */
    size_t  buf_size;      /* size of window */
    size_t  pos_end;       /* last position in set */
    size_t  pos_buf;       /* position of first byte in window */
    size_t  pos_border;    /* position of last byte+1 in window */
    int     dirty;         /* window is dirty */
    int     hits;          /* no of hits */
    char   *temp_path;
    int     (*cmp)(const void *p1, const void *p2);
    struct rset_temp_rfd *rfd_list;
};

struct rset_temp_rfd {
    struct rset_temp_info *info;
    struct rset_temp_rfd *next;
    int *countp;
    void *buf;
    size_t  pos_cur;       /* current position in set */
};

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_temp_parms *temp_parms = (rset_temp_parms *) parms;
    struct rset_temp_info *info;
   
    info = (struct rset_temp_info *) xmalloc (sizeof(struct rset_temp_info));
    info->fd = -1;
    info->fname = NULL;
    info->key_size = temp_parms->key_size;
    info->buf_size = 4096;
    info->buf_mem = (char *) xmalloc (info->buf_size);
    info->pos_end = 0;
    info->pos_buf = 0;
    info->dirty = 0;
    info->hits = -1;
    info->cmp = temp_parms->cmp;
    info->rfd_list = NULL;

    if (!temp_parms->temp_path)
	info->temp_path = NULL;
    else
    {
	info->temp_path = (char *) xmalloc (strlen(temp_parms->temp_path)+1);
	strcpy (info->temp_path, temp_parms->temp_path);
    }
    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *) xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = temp_parms->rset_term;

    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_temp_info *info = (struct rset_temp_info *) ct->buf;
    struct rset_temp_rfd *rfd;

    if (info->fd == -1 && info->fname)
    {
        if (flag & RSETF_WRITE)
            info->fd = open (info->fname, O_BINARY|O_RDWR|O_CREAT, 0666);
        else
            info->fd = open (info->fname, O_BINARY|O_RDONLY);
        if (info->fd == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "open %s", info->fname);
            exit (1);
        }
    }
    rfd = (struct rset_temp_rfd *) xmalloc (sizeof(*rfd));
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;
    r_rewind (rfd);

    rfd->countp = &ct->rset_terms[0]->count;
    *rfd->countp = 0;
    rfd->buf = xmalloc (info->key_size);

    return rfd;
}

/* r_flush:
      flush current window to file if file is assocated with set
 */
static void r_flush (RSFD rfd, int mk)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*) rfd)->info;

    if (!info->fname && mk)
    {
#if HAVE_MKSTEMP
        char template[1024];

        if (info->temp_path)
            sprintf (template, "%s/zrsXXXXXX", info->temp_path);
        else
            sprintf (template, "zrsXXXXXX");

        info->fd = mkstemp (template);

        if (info->fd == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "mkstemp %s", template);
            exit (1);
        }
        info->fname = (char *) xmalloc (strlen(template)+1);
        strcpy (info->fname, template);
#else
        char *s = (char*) tempnam (info->temp_path, "zrs");
        info->fname = (char *) xmalloc (strlen(s)+1);
        strcpy (info->fname, s);

        logf (LOG_DEBUG, "creating tempfile %s", info->fname);
        info->fd = open (info->fname, O_BINARY|O_RDWR|O_CREAT, 0666);
        if (info->fd == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "open %s", info->fname);
            exit (1);
        }
#endif
    }
    if (info->fname && info->fd != -1 && info->dirty)
    {
        size_t count;
	int r;
        
        if (lseek (info->fd, info->pos_buf, SEEK_SET) == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "lseek %s", info->fname);
            exit (1);
        }
        count = info->buf_size;
        if (count > info->pos_end - info->pos_buf)
            count = info->pos_end - info->pos_buf;
        if ((r = write (info->fd, info->buf_mem, count)) < (int) count)
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
    struct rset_temp_rfd **rfdp;

    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            r_flush (*rfdp, 0);
            xfree ((*rfdp)->buf);

            *rfdp = (*rfdp)->next;
            xfree (rfd);

            if (!info->rfd_list && info->fname && info->fd != -1)
            {
                close (info->fd);
                info->fd = -1;
            }
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_temp_info *info = (struct rset_temp_info*) ct->buf;

    if (info->fname)
        unlink (info->fname);        
    xfree (info->buf_mem);
    logf (LOG_DEBUG, "r_delete: set size %ld", (long) info->pos_end);
    if (info->fname)
    {
        logf (LOG_DEBUG, "r_delete: unlink %s", info->fname);
        unlink (info->fname);
        xfree (info->fname);
    }
    if (info->temp_path)
	xfree (info->temp_path);
    rset_term_destroy (ct->rset_terms[0]);
    xfree (ct->rset_terms);
    xfree (info);
}

/* r_reread:
      read from file to window if file is assocated with set -
      indicated by fname
 */
static void r_reread (RSFD rfd)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*)rfd)->info;

    if (info->fname)
    {
        size_t count;
	int r;

        info->pos_border = ((struct rset_temp_rfd *)rfd)->pos_cur +
            info->buf_size;
        if (info->pos_border > info->pos_end)
            info->pos_border = info->pos_end;
        count = info->pos_border - info->pos_buf;
        if (count > 0)
        {
            if (lseek (info->fd, info->pos_buf, SEEK_SET) == -1)
            {
                logf (LOG_FATAL|LOG_ERRNO, "lseek %s", info->fname);
                exit (1);
            }
            if ((r = read (info->fd, info->buf_mem, count)) < (int) count)
            {
                if (r == -1)
                    logf (LOG_FATAL|LOG_ERRNO, "read %s", info->fname);
                else
                    logf (LOG_FATAL, "read of %ld but got %ld",
                          (long) count, (long) r);
                exit (1);
            }
        }
    }
    else
        info->pos_border = info->pos_end;
}

static void r_rewind (RSFD rfd)
{
    struct rset_temp_info *info = ((struct rset_temp_rfd*)rfd)->info;

    r_flush (rfd, 0);
    ((struct rset_temp_rfd *)rfd)->pos_cur = 0;
    info->pos_buf = 0;
    r_reread (rfd);
}

/*
static int r_count (RSET ct)
{
    struct rset_temp_info *info = (struct rset_temp_info *) ct->buf;

    return info->pos_end / info->key_size;
}
*/
static int r_read (RSFD rfd, void *buf, int *term_index)
{
    struct rset_temp_rfd *mrfd = (struct rset_temp_rfd*) rfd;
    struct rset_temp_info *info = mrfd->info;

    size_t nc = mrfd->pos_cur + info->key_size;

    if (mrfd->pos_cur < info->pos_buf || nc > info->pos_border)
    {
        if (nc > info->pos_end)
            return 0;
        r_flush (rfd, 0);
        info->pos_buf = mrfd->pos_cur;
        r_reread (rfd);
    }
    memcpy (buf, info->buf_mem + (mrfd->pos_cur - info->pos_buf),
            info->key_size);
    mrfd->pos_cur = nc;
    *term_index = 0;

    if (*mrfd->countp == 0 || (*info->cmp)(buf, mrfd->buf) > 1)
    {
        memcpy (mrfd->buf, buf, mrfd->info->key_size);
        (*mrfd->countp)++;
    }
    return 1;
}

static int r_write (RSFD rfd, const void *buf)
{
    struct rset_temp_rfd *mrfd = (struct rset_temp_rfd*) rfd;
    struct rset_temp_info *info = mrfd->info;

    size_t nc = mrfd->pos_cur + info->key_size;

    if (nc > info->pos_buf + info->buf_size)
    {
        r_flush (rfd, 1);
        info->pos_buf = mrfd->pos_cur;
        if (info->pos_buf < info->pos_end)
            r_reread (rfd);
    }
    info->dirty = 1;
    memcpy (info->buf_mem + (mrfd->pos_cur - info->pos_buf), buf,
            info->key_size);
    mrfd->pos_cur = nc;
    if (nc > info->pos_end)
        info->pos_border = info->pos_end = nc;
    return 1;
}
