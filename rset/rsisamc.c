/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 *
 * $Id: rsisamc.c,v 1.11 2002-04-05 08:46:26 adam Exp $
 */


#include <stdio.h>
#include <assert.h>
#include <zebrautl.h>
#include <rsisamc.h>

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
    "isamc",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_isamc = &control;

struct rset_pp_info {
    ISAMC_PP pt;
    struct rset_pp_info *next;
    struct rset_isamc_info *info;
    int *countp;
    void *buf;
};

struct rset_isamc_info {
    ISAMC   is;
    ISAMC_P pos;
    int key_size;
    int (*cmp)(const void *p1, const void *p2);
    struct rset_pp_info *ispt_list;
};

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_isamc_parms *pt = (rset_isamc_parms *) parms;
    struct rset_isamc_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
    info = (struct rset_isamc_info *) xmalloc (sizeof(*info));
    info->is = pt->is;
    info->pos = pt->pos;
    info->key_size = pt->key_size;
    info->cmp = pt->cmp;
    info->ispt_list = NULL;
    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *) xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = pt->rset_term;
    return info;
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_isamc_info *info = (struct rset_isamc_info *) ct->buf;
    struct rset_pp_info *ptinfo;

    logf (LOG_DEBUG, "risamc_open");
    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "ISAMC set type is read-only");
	return NULL;
    }
    ptinfo = (struct rset_pp_info *) xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = isc_pp_open (info->is, info->pos);
    ptinfo->info = info;
    if (ct->rset_terms[0]->nn < 0)
	ct->rset_terms[0]->nn = isc_pp_num (ptinfo->pt);
    ct->rset_terms[0]->count = 0;
    ptinfo->countp = &ct->rset_terms[0]->count;
    ptinfo->buf = xmalloc (info->key_size);
    return ptinfo;
}

static void r_close (RSFD rfd)
{
    struct rset_isamc_info *info = ((struct rset_pp_info*) rfd)->info;
    struct rset_pp_info **ptinfop;

    for (ptinfop = &info->ispt_list; *ptinfop; ptinfop = &(*ptinfop)->next)
        if (*ptinfop == rfd)
        {
            xfree ((*ptinfop)->buf);
            isc_pp_close ((*ptinfop)->pt);
            *ptinfop = (*ptinfop)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_isamc_info *info = (struct rset_isamc_info *) ct->buf;

    logf (LOG_DEBUG, "rsisamc_delete");
    assert (info->ispt_list == NULL);
    rset_term_destroy (ct->rset_terms[0]);
    xfree (ct->rset_terms);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{   
    logf (LOG_DEBUG, "rsisamc_rewind");
    abort ();
}

static int r_count (RSET ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf, int *term_index)
{
    struct rset_pp_info *pinfo = (struct rset_pp_info *) rfd;
    int r;
    *term_index = 0;
    r = isc_pp_read(pinfo->pt, buf);
    if (r > 0)
    {
        if (*pinfo->countp == 0 || (*pinfo->info->cmp)(buf, pinfo->buf) > 1)
        {
            memcpy (pinfo->buf, buf, pinfo->info->key_size);
            (*pinfo->countp)++;
        }
    }
    return r;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAMC set type is read-only");
    return -1;
}
