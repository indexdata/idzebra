/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rssbool.c,v $
 * Revision 1.4  1996-10-29 13:55:27  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.3  1996/10/08 13:00:41  adam
 * Bug fix: result sets with ranked operands in boolean operations weren't
 * sorted.
 *
 * Revision 1.2  1996/05/15  18:35:17  adam
 * Implemented snot operation.
 *
 * Revision 1.1  1995/12/11  09:15:27  adam
 * New set types: sand/sor/snot - ranked versions of and/or/not in
 * ranked/semi-ranked result sets.
 * Note: the snot not finished yet.
 * New rset member: flag.
 * Bug fix: r_delete in rsrel.c did free bad memory block.
 *
 */

#include <stdio.h>
#include <assert.h>

#include <rsbool.h>
#include <zebrautl.h>

static void *r_create_and(const struct rset_control *sel, void *parms,
                         int *flags);
static void *r_create_or(const struct rset_control *sel, void *parms,
                         int *flags);
static void *r_create_not(const struct rset_control *sel, void *parms,
                         int *flags);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_count (RSET ct);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);
static int r_score (RSFD rfd, int *score);

static const rset_control control_sand = 
{
    "sand",
    r_create_and,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
    r_score
};

static const rset_control control_sor = 
{
    "sor",
    r_create_or,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
    r_score
};

static const rset_control control_snot = 
{
    "snot",
    r_create_not,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
    r_score
};


const rset_control *rset_kind_sand = &control_sand;
const rset_control *rset_kind_sor = &control_sor;
const rset_control *rset_kind_snot = &control_snot;

struct rset_bool_info {
    int key_size;
    RSET rset_l;
    RSET rset_r;
    char *key_buf;
    int *score_buf;
    int *score_idx;
    int key_no;
    int key_max;
    int (*cmp)(const void *p1, const void *p2);
    struct rset_bool_rfd *rfd_list;
};

struct rset_bool_rfd {
    struct rset_bool_rfd *next;
    struct rset_bool_info *info;
    int position;
    int last_pos;
    int flag;
};    

static void *r_create_common (const struct rset_control *sel, 
                              rset_bool_parms *bool_parms, int *flags);

static struct rset_bool_info *qsort_info;

static int qcomp (const void *p1, const void *p2)
{
    int i1 = *(int*) p1;
    int i2 = *(int*) p2;
    return qsort_info->score_buf[i2] - qsort_info->score_buf[i1];
}

static void key_add (struct rset_bool_info *info,
                     char *buf, int score)
{
    if (info->key_no == info->key_max)
        return;
    memcpy (info->key_buf + info->key_size * info->key_no,
            buf, info->key_size);
    info->score_buf[info->key_no] = score;
    info->score_idx[info->key_no] = info->key_no;
    (info->key_no)++;
}

static void *r_create_and (const struct rset_control *sel, void *parms,
                           int *flags)
{
    int more_l, more_r;
    RSFD fd_l, fd_r;
    char *buf_l, *buf_r;

    struct rset_bool_info *info;
    info = r_create_common (sel, parms, flags);

    buf_l = xmalloc (info->key_size);
    buf_r = xmalloc (info->key_size);
    fd_l = rset_open (info->rset_l, RSETF_SORT_SYSNO|RSETF_READ);
    fd_r = rset_open (info->rset_r, RSETF_SORT_SYSNO|RSETF_READ);

    more_l = rset_read(info->rset_l, fd_l, buf_l);
    more_r = rset_read(info->rset_r, fd_r, buf_r);

    while (more_l || more_r)
    {
        int cmp;
        int score, score_l, score_r;

        if (more_l && more_r)
            cmp = (*info->cmp)(buf_l, buf_r);
        else if (more_r)
            cmp = 2;
        else 
            cmp = -2;

        if (cmp >= -1 && cmp <= 1)
        {
            rset_score (info->rset_l, fd_l, &score_l);
            rset_score (info->rset_r, fd_r, &score_r);
            if (score_l == -1)
                score = score_r;
            else if (score_r == -1)
                score = score_l;
            else
                score = score_l > score_r ? score_r : score_l;
            key_add (info, buf_l, score);
    
            more_l = rset_read (info->rset_l, fd_l, buf_l);
            more_r = rset_read (info->rset_r, fd_r, buf_r);
        }
        else if (cmp > 1)
            more_r = rset_read (info->rset_r, fd_r, buf_r);
        else
            more_l = rset_read (info->rset_l, fd_l, buf_l);
    }
    rset_close (info->rset_l, fd_l);
    rset_close (info->rset_r, fd_r);
    rset_delete (info->rset_l);
    rset_delete (info->rset_r);
    xfree (buf_l);
    xfree (buf_r);
    qsort_info = info;
    qsort (info->score_idx, info->key_no, sizeof(*info->score_idx), qcomp);
    return info;
}

static void *r_create_or (const struct rset_control *sel, void *parms,
                          int *flags)
{
    int more_l, more_r;
    RSFD fd_l, fd_r;
    char *buf_l, *buf_r;

    struct rset_bool_info *info;
    info = r_create_common (sel, parms, flags);

    buf_l = xmalloc (info->key_size);
    buf_r = xmalloc (info->key_size);
    fd_l = rset_open (info->rset_l, RSETF_SORT_SYSNO|RSETF_READ);
    fd_r = rset_open (info->rset_r, RSETF_SORT_SYSNO|RSETF_READ);

    more_l = rset_read(info->rset_l, fd_l, buf_l);
    more_r = rset_read(info->rset_r, fd_r, buf_r);

    while (more_l || more_r)
    {
        int cmp;
        int score, score_l, score_r;

        if (more_l && more_r)
            cmp = (*info->cmp)(buf_l, buf_r);
        else if (more_r)
            cmp = 2;
        else 
            cmp = -2;

        if (cmp >= -1 && cmp <= 1)
        {
            rset_score (info->rset_l, fd_l, &score_l);
            rset_score (info->rset_r, fd_r, &score_r);
            if (score_l == -1)
                score = score_r;
            else if (score_r == -1)
                score = score_l;
            else
                score = score_r > score_l ? score_r : score_l;
            key_add (info, buf_l, score);
    
            more_l = rset_read (info->rset_l, fd_l, buf_l);
            more_r = rset_read (info->rset_r, fd_r, buf_r);
        }
        else if (cmp > 1)
        {
            rset_score (info->rset_r, fd_r, &score_r);
            if (score_r != -1)
                key_add (info, buf_r, score_r / 2);
            more_r = rset_read (info->rset_r, fd_r, buf_r);
        }
        else
        {
            rset_score (info->rset_l, fd_l, &score_l);
            if (score_l != -1)
                key_add (info, buf_l, score_l / 2);
            more_l = rset_read (info->rset_l, fd_l, buf_l);
        }
    }
    rset_close (info->rset_l, fd_l);
    rset_close (info->rset_r, fd_r);
    rset_delete (info->rset_l);
    rset_delete (info->rset_r);
    xfree (buf_l);
    xfree (buf_r);
    qsort_info = info;
    qsort (info->score_idx, info->key_no, sizeof(*info->score_idx), qcomp);
    return info;
}

static void *r_create_not (const struct rset_control *sel, void *parms,
                           int *flags)
{
    char *buf_l, *buf_r;
    int more_l, more_r;
    RSFD fd_l, fd_r;

    struct rset_bool_info *info;
    info = r_create_common (sel, parms, flags);

    buf_l = xmalloc (info->key_size);
    buf_r = xmalloc (info->key_size);

    fd_l = rset_open (info->rset_l, RSETF_SORT_SYSNO|RSETF_READ);
    fd_r = rset_open (info->rset_r, RSETF_SORT_SYSNO|RSETF_READ);

    more_l = rset_read(info->rset_l, fd_l, buf_l);
    more_r = rset_read(info->rset_r, fd_r, buf_r);

    while (more_l || more_r)
    {
        int cmp;
        int score;

        if (more_l && more_r)
            cmp = (*info->cmp)(buf_l, buf_r);
        else if (more_r)
            cmp = 2;
        else 
            cmp = -2;

        if (cmp >= -1 && cmp <= 1)
            more_l = rset_read (info->rset_l, fd_l, buf_l);
        else if (cmp > 1)
        {
            more_r = rset_read (info->rset_r, fd_r, buf_r);
        }
        else
        {
            rset_score (info->rset_l, fd_l, &score);
            key_add (info, buf_l, score == -1 ? 1 : score);
            more_l = rset_read (info->rset_l, fd_l, buf_l);
        }
    }
    rset_close (info->rset_l, fd_l);
    rset_close (info->rset_r, fd_r);

    rset_delete (info->rset_l);
    rset_delete (info->rset_r);
    xfree (buf_l);
    xfree (buf_r);
    qsort_info = info;
    qsort (info->score_idx, info->key_no, sizeof(*info->score_idx), qcomp);
    return info;
}

static void *r_create_common (const struct rset_control *sel, 
                              rset_bool_parms *bool_parms, int *flags)
{
    struct rset_bool_info *info;

    info = xmalloc (sizeof(*info));
    info->key_size = bool_parms->key_size;
    info->rset_l = bool_parms->rset_l;
    info->rset_r = bool_parms->rset_r;
    info->cmp = bool_parms->cmp;
    info->rfd_list = NULL;
    
    if (rset_is_ranked(info->rset_l) || rset_is_ranked(info->rset_r))
        *flags |= RSET_FLAG_RANKED;

    info->key_max = rset_count(bool_parms->rset_l)
                   +rset_count(bool_parms->rset_r);
    if (!info->key_max)
        info->key_max = 1;
    if (info->key_max > 1000)
        info->key_max = 1000;
    info->key_buf = xmalloc (info->key_size * info->key_max);
    info->score_buf = xmalloc (info->key_max * sizeof(*info->score_buf));
    info->score_idx = xmalloc (info->key_max * sizeof(*info->score_idx));
    info->key_no = 0;

    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_bool_info *info = ct->buf;
    struct rset_bool_rfd *rfd;

    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "sbool set type is read-only");
	return NULL;
    }
    rfd = xmalloc (sizeof(*rfd));
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;

    rfd->position = 0;
    rfd->last_pos = 0;
    rfd->flag = flag;

    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_bool_info *info = ((struct rset_bool_rfd*)rfd)->info;
    struct rset_bool_rfd **rfdp;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            *rfdp = (*rfdp)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_bool_info *info = ct->buf;

    assert (info->rfd_list == NULL);
    xfree (info->score_buf);
    xfree (info->score_idx);
    xfree (info->key_buf);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{
    struct rset_bool_rfd *p = rfd;

    logf (LOG_DEBUG, "rsbool_rewind");
    p->position = p->last_pos = 0;
}

static int r_count (RSET ct)
{
    struct rset_bool_info *info = ct->buf;

    return info->key_no;
}

static int r_read (RSFD rfd, void *buf)
{
    struct rset_bool_rfd *p = rfd;
    struct rset_bool_info *info = p->info;

    if (p->position >= info->key_no)
        return 0;
    if (p->flag & RSETF_SORT_RANK)
        p->last_pos = info->score_idx[(p->position)++];
    else
        p->last_pos = (p->position)++;
    memcpy (buf, info->key_buf + info->key_size * p->last_pos,
            info->key_size);
    return 1;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "sbool set type is read-only");
    return -1;
}

static int r_score (RSFD rfd, int *score)
{
    struct rset_bool_rfd *p = rfd;
    struct rset_bool_info *info = p->info;

    *score = info->score_buf[p->last_pos];
    return 1;
}

