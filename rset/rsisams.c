/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisams.c,v $
 * Revision 1.1  1999-05-12 15:24:25  adam
 * First version of ISAMS.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <rsisams.h>
#include <zebrautl.h>

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
    "isams",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_isams = &control;

struct rset_pp_info {
    ISAMS_PP pt;
    struct rset_pp_info *next;
    struct rset_isams_info *info;
};

struct rset_isams_info {
    ISAMS   is;
    ISAMS_P pos;
    struct rset_pp_info *ispt_list;
};

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_isams_parms *pt = parms;
    struct rset_isams_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
    info = xmalloc (sizeof(*info));
    info->is = pt->is;
    info->pos = pt->pos;
    info->ispt_list = NULL;
    ct->no_rset_terms = 1;
    ct->rset_terms = xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = pt->rset_term;
    return info;
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_isams_info *info = ct->buf;
    struct rset_pp_info *ptinfo;

    logf (LOG_DEBUG, "risams_open");
    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "ISAMS set type is read-only");
	return NULL;
    }
    ptinfo = xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = isams_pp_open (info->is, info->pos);
    ptinfo->info = info;
    if (ct->rset_terms[0]->nn < 0)
	ct->rset_terms[0]->nn = isams_pp_num (ptinfo->pt);
    return ptinfo;
}

static void r_close (RSFD rfd)
{
    struct rset_isams_info *info = ((struct rset_pp_info*) rfd)->info;
    struct rset_pp_info **ptinfop;

    for (ptinfop = &info->ispt_list; *ptinfop; ptinfop = &(*ptinfop)->next)
        if (*ptinfop == rfd)
        {
            isams_pp_close ((*ptinfop)->pt);
            *ptinfop = (*ptinfop)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_isams_info *info = ct->buf;

    logf (LOG_DEBUG, "rsisams_delete");
    assert (info->ispt_list == NULL);
    rset_term_destroy (ct->rset_terms[0]);
    xfree (ct->rset_terms);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{   
    logf (LOG_DEBUG, "rsisams_rewind");
    abort ();
}

static int r_count (RSET ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf, int *term_index)
{
    *term_index = 0;
    return isams_pp_read( ((struct rset_pp_info*) rfd)->pt, buf);
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAMS set type is read-only");
    return -1;
}
