/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsrel.c,v $
 * Revision 1.2  1995-09-11 13:09:41  adam
 * More work on relevance feedback.
 *
 * Revision 1.1  1995/09/08  14:52:42  adam
 * Work on relevance feedback.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <isam.h>
#include <rsrel.h>
#include <alexutil.h>

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
    "relevance set type",
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

const rset_control *rset_kind_relevance = &control;

struct rset_rel_info {
    int     key_size;
    int     max_rec;
    int     no_rec;
    int     (*cmp)(const void *p1, const void *p2);
    void    *key_buf;
    int     *score_buf;

    struct rset_rel_rfd *rfd_list;
};

struct rset_rel_rfd {
    int     position;
    struct rset_rel_rfd *next;
    struct rset_rel_info *info;
};

static void relevance (struct rset_rel_info *info, rset_relevance_parms *parms)
{
    char **isam_buf;
    char *isam_tmp_buf;
    int  *isam_r;
    int  *max_tf;
    ISPT *isam_pt;
    double *wgt;
    int i;

    logf (LOG_DEBUG, "relevance");
    isam_buf = xmalloc (parms->no_isam_positions * sizeof(*isam_buf));
    isam_r = xmalloc (sizeof (*isam_r) * parms->no_isam_positions);
    isam_pt = xmalloc (sizeof (*isam_pt) * parms->no_isam_positions);
    isam_tmp_buf = xmalloc (info->key_size);
    max_tf = xmalloc (sizeof (*max_tf) * parms->no_isam_positions);
    wgt = xmalloc (sizeof (*wgt) * parms->no_isam_positions);

    for (i = 0; i<parms->no_isam_positions; i++)
    {
        isam_buf[i] = xmalloc (info->key_size);
        isam_pt[i] = is_position (parms->is, parms->isam_positions[i]);
        max_tf [i] = is_numkeys (isam_pt[i]);
        isam_r[i] = is_readkey (isam_pt[i], isam_buf[i]);
        logf (LOG_DEBUG, "max tf %d = %d", i, max_tf[i]);
    }
    while (1)
    {
        int min = -1, i;
        double length, similarity;

        /* find min with lowest sysno */
        for (i = 0; i<parms->no_isam_positions; i++)
            if (isam_r[i] && 
               (min < 0 || (*parms->cmp)(isam_buf[i], isam_buf[min]) < 1))
                min = i;
        if (min < 0)
            break;
        memcpy (isam_tmp_buf, isam_buf[min], info->key_size);
        logf (LOG_LOG, "calc rel for");
        key_logdump (LOG_LOG, isam_tmp_buf);
        /* calculate for all with those sysno */
        length = 0.0;
        for (i = 0; i<parms->no_isam_positions; i++)
        {
            int r;

            if (isam_r[i])
                r = (*parms->cmp)(isam_buf[i], isam_tmp_buf);
            else 
                r = 2;
            if (r > 1 || r < -1)
                wgt[i] = 0.0;
            else
            {
                int tf = 0;
                do
                {
                    tf++;
                    isam_r[i] = is_readkey (isam_pt[i], isam_buf[i]);
                } while (isam_r[i] && 
                         (*parms->cmp)(isam_buf[i], isam_tmp_buf) <= 1);
                logf (LOG_DEBUG, "tf%d = %d", i, tf);
                wgt[i] = 0.5+tf*0.5/max_tf[i];
                length += wgt[i] * wgt[i];
            }
        }
        /* calculate relevance value */
        length = sqrt (length);
        similarity = 0.0;
        for (i = 0; i<parms->no_isam_positions; i++)
             similarity += wgt[i]/length;
        logf (LOG_LOG, " %f", similarity);
        /* if value is in the top score, then save it - don't emit yet */
    }
    for (i = 0; i<parms->no_isam_positions; i++)
    {
        is_pt_free (isam_pt[i]);
        xfree (isam_buf[i]);
    }
    xfree (max_tf);
    xfree (isam_tmp_buf);
    xfree (isam_buf);
    xfree (isam_r);
    xfree (isam_pt);
    xfree (wgt);
}

static rset_control *r_create (const struct rset_control *sel, void *parms)
{
    rset_control *newct;
    rset_relevance_parms *r_parms = parms;
    struct rset_rel_info *info;

    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));
    newct->buf = xmalloc (sizeof(struct rset_rel_info));

    info = newct->buf;
    info->key_size = r_parms->key_size;
    assert (info->key_size > 1);
    info->max_rec = r_parms->max_rec;
    assert (info->max_rec > 1);
    info->cmp = r_parms->cmp;

    info->key_buf = xmalloc (info->key_size * info->max_rec);
    info->score_buf = xmalloc (sizeof(*info->score_buf) * info->max_rec);
    info->no_rec = 0;
    info->rfd_list = NULL;

    relevance (info, r_parms);
    return newct;
}

static RSFD r_open (rset_control *ct, int wflag)
{
    struct rset_rel_rfd *rfd;
    struct rset_rel_info *info = ct->buf;

    if (wflag)
    {
	logf (LOG_FATAL, "relevance set type is read-only");
	return NULL;
    }
    rfd = xmalloc (sizeof(*rfd));
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->position = 0;
    rfd->info = info;
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_rel_info *info = ((struct rset_rel_rfd*)rfd)->info;
    struct rset_rel_rfd **rfdp;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            *rfdp = (*rfdp)->next;
            free (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (rset_control *ct)
{
    struct rset_rel_info *info = ct->buf;

    assert (info->rfd_list == NULL);
    xfree (info->key_buf);
    xfree (info->score_buf);
    xfree (info);
    xfree (ct);
}

static void r_rewind (RSFD rfd)
{
    ((struct rset_rel_rfd*) rfd)->position = 0;
}

static int r_count (rset_control *ct)
{
    struct rset_rel_info *info = ct->buf;

    return info->no_rec;
}

static int r_read (RSFD rfd, void *buf)
{
    struct rset_rel_rfd *p = rfd;
    struct rset_rel_info *info = p->info;

    if (p->position >= info->max_rec)
        return 0;
    memcpy ((char*) buf + sizeof(*info->score_buf),
            (char*) info->key_buf + info->key_size * p->position,
            info->key_size);
    memcpy ((char*) buf,
            info->score_buf + p->position, sizeof(*info->score_buf));
    ++(p->position);
    return 1;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "relevance set type is read-only");
    return -1;
}
