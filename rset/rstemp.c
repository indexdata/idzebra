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
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include <yaz/snprintf.h>

#include <idzebra/util.h>
#include <rset.h>

static RSFD r_open(RSET ct, int flag);
static void r_close(RSFD rfd);
static void r_delete(RSET ct);
static int r_read(RSFD rfd, void *buf, TERMID *term);
static int r_write(RSFD rfd, const void *buf);
static void r_pos(RSFD rfd, double *current, double  *total);
static void r_flush(RSFD rfd, int mk);
static void r_reread(RSFD rfd);

static const struct rset_control control =
{
    "temp",
    r_delete,
    rset_get_one_term,
    r_open,
    r_close,
    0, /* no forward */
    r_pos,
    r_read,
    r_write,
};

struct rset_private {
    int     fd;            /* file descriptor for temp file */
    char   *fname;         /* name of temp file */
    char   *buf_mem;       /* window buffer */
    size_t  buf_size;      /* size of window */
    size_t  pos_end;       /* last position in set */
    size_t  pos_buf;       /* position of first byte in window */
    size_t  pos_border;    /* position of last byte+1 in window */
    int     dirty;         /* window is dirty */
    zint    hits;          /* no of hits */
    char   *temp_path;
};

struct rfd_private {
    void *buf;
    size_t  pos_cur;       /* current position in set */
                           /* FIXME - term pos or what ??  */
    zint cur; /* number of the current hit */
};

static int log_level = 0;
static int log_level_initialized = 0;

RSET rset_create_temp(NMEM nmem, struct rset_key_control *kcontrol,
                      int scope, const char *temp_path, TERMID term)
{
    RSET rnew = rset_create_base(&control, nmem, kcontrol, scope, term,
                                 0, 0);
    struct rset_private *info;
    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("rstemp");
        log_level_initialized = 1;
    }
    info = (struct rset_private *) nmem_malloc(rnew->nmem, sizeof(*info));
    info->fd = -1;
    info->fname = NULL;
    info->buf_size = 4096;
    info->buf_mem = (char *) nmem_malloc(rnew->nmem, info->buf_size);
    info->pos_end = 0;
    info->pos_buf = 0;
    info->dirty = 0;
    info->hits = 0;

    if (!temp_path)
        info->temp_path = NULL;
    else
        info->temp_path = nmem_strdup(rnew->nmem, temp_path);
    rnew->priv = info;
    return rnew;
} /* rstemp_create */

static void r_delete(RSET ct)
{
    struct rset_private *info = (struct rset_private*) ct->priv;

    yaz_log(log_level, "r_delete: set size %ld", (long) info->pos_end);
    if (info->fname)
    {
        yaz_log(log_level, "r_delete: unlink %s", info->fname);
        unlink(info->fname);
    }
}

static RSFD r_open(RSET ct, int flag)
{
    struct rset_private *info = (struct rset_private *) ct->priv;
    RSFD rfd;
    struct rfd_private *prfd;

    if (info->fd == -1 && info->fname)
    {
        if (flag & RSETF_WRITE)
            info->fd = open(info->fname, O_BINARY|O_RDWR|O_CREAT, 0666);
        else
            info->fd = open(info->fname, O_BINARY|O_RDONLY);
        if (info->fd == -1)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "rstemp: open failed %s", info->fname);
            zebra_exit("r_open");
        }
    }
    rfd = rfd_create_base(ct);
    if (!rfd->priv)
    {
        prfd = (struct rfd_private *) nmem_malloc(ct->nmem, sizeof(*prfd));
        rfd->priv = (void *)prfd;
        prfd->buf = nmem_malloc(ct->nmem,ct->keycontrol->key_size);
    }
    else
        prfd= rfd->priv;
    r_flush(rfd, 0);
    prfd->pos_cur = 0;
    info->pos_buf = 0;
    r_reread(rfd);
    prfd->cur = 0;
    return rfd;
}

/* r_flush:
      flush current window to file if file is assocated with set
 */
static void r_flush(RSFD rfd, int mk)
{
    struct rset_private *info = rfd->rset->priv;

    if (!info->fname && mk)
    {
#if HAVE_MKSTEMP
        char template[1024];

        if (info->temp_path)
            yaz_snprintf(template, sizeof(template) - 80,
                "%s/", info->temp_path);
        else
            strcpy(template, "");
        strcat(template, "zrs_");
#if HAVE_UNISTD_H
        yaz_snprintf(template + strlen(template), 40, "%ld_", (long) getpid());
#endif
        strcat(template, "XXXXXX");

        info->fd = mkstemp(template);
        if (info->fd == -1)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "rstemp: mkstemp %s", template);
            zebra_exit("r_flush");
        }
        info->fname = nmem_strdup(rfd->rset->nmem, template);
#else
        char *s = (char*) tempnam(info->temp_path, "zrs");

        yaz_log(log_level, "creating tempfile %s", s);
        info->fd = open(s, O_BINARY|O_RDWR|O_CREAT, 0666);
        if (info->fd == -1)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "rstemp: open %s", s);
            zebra_exit("r_flush");
        }
        info->fname= nmem_strdup(rfd->rset->nmem, s);
#endif
    }
    if (info->fname && info->fd != -1 && info->dirty)
    {
        size_t count;
        int r;

        if (lseek(info->fd, info->pos_buf, SEEK_SET) == -1)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "rstemp: lseek (1) %s", info->fname);
            zebra_exit("r_flusxh");
        }
        count = info->buf_size;
        if (count > info->pos_end - info->pos_buf)
            count = info->pos_end - info->pos_buf;
        if ((r = write(info->fd, info->buf_mem, count)) < (int) count)
        {
            if (r == -1)
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "rstemp: write %s", info->fname);
            else
                yaz_log(YLOG_FATAL, "rstemp: write of %ld but got %ld",
                      (long) count, (long) r);
            zebra_exit("r_flush");
        }
        info->dirty = 0;
    }
}

static void r_close(RSFD rfd)
{
    struct rset_private *info = (struct rset_private *)rfd->rset->priv;
    if (rfd_is_last(rfd))
    {
        r_flush(rfd, 0);
        if (info->fname && info->fd != -1)
        {
            close(info->fd);
            info->fd = -1;
        }
    }
}


/* r_reread:
      read from file to window if file is assocated with set -
      indicated by fname
 */
static void r_reread(RSFD rfd)
{
    struct rfd_private *mrfd = (struct rfd_private*) rfd->priv;
    struct rset_private *info = (struct rset_private *)rfd->rset->priv;

    if (info->fname)
    {
        size_t count;
        int r;

        info->pos_border = mrfd->pos_cur +
            info->buf_size;
        if (info->pos_border > info->pos_end)
            info->pos_border = info->pos_end;
        count = info->pos_border - info->pos_buf;
        if (count > 0)
        {
            if (lseek(info->fd, info->pos_buf, SEEK_SET) == -1)
            {
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "rstemp: lseek (2) %s fd=%d", info->fname, info->fd);
                zebra_exit("r_reread");
            }
            if ((r = read(info->fd, info->buf_mem, count)) < (int) count)
            {
                if (r == -1)
                    yaz_log(YLOG_FATAL|YLOG_ERRNO, "rstemp: read %s", info->fname);
                else
                    yaz_log(YLOG_FATAL, "read of %ld but got %ld",
                          (long) count, (long) r);
                zebra_exit("r_reread");
            }
        }
    }
    else
        info->pos_border = info->pos_end;
}

static int r_read(RSFD rfd, void *buf, TERMID *term)
{
    struct rfd_private *mrfd = (struct rfd_private*) rfd->priv;
    struct rset_private *info = (struct rset_private *)rfd->rset->priv;

    size_t nc = mrfd->pos_cur + rfd->rset->keycontrol->key_size;

    if (mrfd->pos_cur < info->pos_buf || nc > info->pos_border)
    {
        if (nc > info->pos_end)
            return 0;
        r_flush(rfd, 0);
        info->pos_buf = mrfd->pos_cur;
        r_reread(rfd);
    }
    memcpy(buf, info->buf_mem + (mrfd->pos_cur - info->pos_buf),
            rfd->rset->keycontrol->key_size);
    if (term)
        *term = rfd->rset->term;
        /* FIXME - should we store and return terms ?? */
    mrfd->pos_cur = nc;
    mrfd->cur++;
    return 1;
}

static int r_write(RSFD rfd, const void *buf)
{
    struct rfd_private *mrfd = (struct rfd_private*) rfd->priv;
    struct rset_private *info = (struct rset_private *)rfd->rset->priv;

    size_t nc = mrfd->pos_cur + rfd->rset->keycontrol->key_size;

    if (nc > info->pos_buf + info->buf_size)
    {
        r_flush(rfd, 1);
        info->pos_buf = mrfd->pos_cur;
        if (info->pos_buf < info->pos_end)
            r_reread(rfd);
    }
    info->dirty = 1;
    memcpy(info->buf_mem + (mrfd->pos_cur - info->pos_buf), buf,
            rfd->rset->keycontrol->key_size);
    mrfd->pos_cur = nc;
    if (nc > info->pos_end)
        info->pos_border = info->pos_end = nc;
    info->hits++;
    return 1;
}

static void r_pos(RSFD rfd, double  *current, double  *total)
{
    struct rfd_private *mrfd = (struct rfd_private*) rfd->priv;
    struct rset_private *info = (struct rset_private *)rfd->rset->priv;

    *current = (double) mrfd->cur;
    *total = (double) info->hits;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

