/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 *
 * $Id: rsisam.c,v 1.22 2002-04-05 08:46:26 adam Exp $
 */
#include <stdio.h>
#include <assert.h>
#include <zebrautl.h>
#include <rsisam.h>

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
    "isam",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_isam = &control;

struct rset_ispt_info {
    ISPT   pt;
    struct rset_ispt_info *next;
    struct rset_isam_info *info;
};

struct rset_isam_info {
    ISAM   is;
    ISAM_P pos;
    struct rset_ispt_info *ispt_list;
};

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_isam_parms *pt = (rset_isam_parms *) parms;
    struct rset_isam_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
    info = (struct rset_isam_info *) xmalloc (sizeof(struct rset_isam_info));
    info->is = pt->is;
    info->pos = pt->pos;
    info->ispt_list = NULL;

    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *) xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = pt->rset_term;
    return info;
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_isam_info *info = (struct rset_isam_info *) ct->buf;
    struct rset_ispt_info *ptinfo;

    logf (LOG_DEBUG, "risam_open");
    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "ISAM set type is read-only");
	return NULL;
    }
    ptinfo = (struct rset_ispt_info *) xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = is_position (info->is, info->pos);
    ptinfo->info = info;

    if (ct->rset_terms[0]->nn < 0)
	ct->rset_terms[0]->nn = is_numkeys (ptinfo->pt);
    return ptinfo;
}

static void r_close (RSFD rfd)
{
    struct rset_isam_info *info = ((struct rset_ispt_info*) rfd)->info;
    struct rset_ispt_info **ptinfop;

    for (ptinfop = &info->ispt_list; *ptinfop; ptinfop = &(*ptinfop)->next)
        if (*ptinfop == rfd)
        {
            is_pt_free ((*ptinfop)->pt);
            *ptinfop = (*ptinfop)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_isam_info *info = (struct rset_isam_info *) ct->buf;

    logf (LOG_DEBUG, "rsisam_delete");
    assert (info->ispt_list == NULL);
    rset_term_destroy (ct->rset_terms[0]);
    xfree (ct->rset_terms);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{   
    logf (LOG_DEBUG, "rsisam_rewind");
    is_rewind( ((struct rset_ispt_info*) rfd)->pt);
}

static int r_count (RSET ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf, int *term_index)
{
    *term_index = 0;
    return is_readkey( ((struct rset_ispt_info*) rfd)->pt, buf);
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAM set type is read-only");
    return -1;
}
