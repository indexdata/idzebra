/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rstemp.c,v $
 * Revision 1.26  1999-05-26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.25  1999/02/02 14:51:37  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.24  1998/03/05 08:36:28  adam
 * New result set model.
 *
 * Revision 1.23  1997/12/18 10:54:25  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.22  1997/10/31 12:38:12  adam
 * Bug fix: added missing xfree() call.
 *
 * Revision 1.21  1997/09/17 12:19:23  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.20  1997/09/09 13:38:17  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.19  1997/09/04 13:58:57  adam
 * Added O_BINARY for open calls.
 *
 * Revision 1.18  1996/10/29 13:54:52  adam
 * Changed name of setting tempSetDir to setTmpDir.
 *
 * Revision 1.17  1995/12/11 09:15:28  adam
 * New set types: sand/sor/snot - ranked versions of and/or/not in
 * ranked/semi-ranked result sets.
 * Note: the snot not finished yet.
 * New rset member: flag.
 * Bug fix: r_delete in rsrel.c did free bad memory block.
 *
 * Revision 1.16  1995/11/28  14:47:02  adam
 * New setting: tempSetPath. Location of temporary result sets.
 *
 * Revision 1.15  1995/10/12  12:41:58  adam
 * Private info (buf) moved from struct rset_control to struct rset.
 * Bug fixes in relevance.
 *
 * Revision 1.14  1995/10/10  14:00:04  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.13  1995/10/06  14:38:06  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.12  1995/09/28  09:52:11  adam
 * xfree/xmalloc used everywhere.
 *
 * Revision 1.11  1995/09/18  14:17:56  adam
 * Bug fixes.
 *
 * Revision 1.10  1995/09/15  14:45:39  adam
 * Bug fixes.
 *
 * Revision 1.9  1995/09/15  09:20:42  adam
 * Bug fixes.
 *
 * Revision 1.8  1995/09/08  14:52:42  adam
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
static int r_count (RSET ct);
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
    r_count,
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
    size_t  pos_cur;       /* current position in set */
    size_t  pos_buf;       /* position of first byte in window */
    size_t  pos_border;    /* position of last byte+1 in window */
    int     dirty;         /* window is dirty */
    int     hits;          /* no of hits */
    char   *temp_path;
};

struct rset_temp_rfd {
    struct rset_temp_info *info;
    struct rset_temp_rfd *next;
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
    info->pos_cur = 0;
    info->pos_end = 0;
    info->pos_buf = 0;
    info->dirty = 0;
    info->hits = -1;
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

    assert (info->fd == -1);
    if (info->fname)
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
    rfd->info = info;
    r_rewind (rfd);
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

    r_flush (rfd, 0);
    if (info->fname && info->fd != -1)
    {
        close (info->fd);
        info->fd = -1;
    }
    xfree (rfd);
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

        info->pos_border = info->pos_cur + info->buf_size;
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
    info->pos_cur = 0;
    info->pos_buf = 0;
    r_reread (rfd);
}

static int r_count (RSET ct)
{
    struct rset_temp_info *info = (struct rset_temp_info *) ct->buf;

    return info->pos_end / info->key_size;
}

static int r_read (RSFD rfd, void *buf, int *term_index)
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
    *term_index = 0;
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
        if (info->pos_buf < info->pos_end)
            r_reread (rfd);
    }
    info->dirty = 1;
    memcpy (info->buf_mem + (info->pos_cur - info->pos_buf), buf,
            info->key_size);
    info->pos_cur = nc;
    if (nc > info->pos_end)
        info->pos_border = info->pos_end = nc;
    return 1;
}
