/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisamc.c,v $
 * Revision 1.4  1997-12-18 10:54:25  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.3  1997/10/31 12:37:01  adam
 * Code calls xfree() instead of free().
 *
 * Revision 1.2  1996/11/08 11:15:57  adam
 * Compressed isam fully supported.
 *
 * Revision 1.1  1996/10/29 13:41:48  adam
 * First use of isamc.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <rsisamc.h>
#include <zebrautl.h>

static void *r_create(const struct rset_control *sel, void *parms,
                      int *flags);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_count (RSET ct);
static int r_hits (RSET ct, void *oi);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);
static int r_score (RSFD rfd, int *score);

static const rset_control control = 
{
    "isamc",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_hits,
    r_read,
    r_write,
    r_score
};

const rset_control *rset_kind_isamc = &control;

struct rset_pp_info {
    ISAMC_PP pt;
    struct rset_pp_info *next;
    struct rset_isamc_info *info;
};

struct rset_isamc_info {
    ISAMC   is;
    ISAMC_P pos;
    struct rset_pp_info *ispt_list;
};

static void *r_create(const struct rset_control *sel, void *parms,
                      int *flags)
{
    rset_isamc_parms *pt = parms;
    struct rset_isamc_info *info;

    *flags |= RSET_FLAG_VOLATILE;
    info = xmalloc (sizeof(*info));
    info->is = pt->is;
    info->pos = pt->pos;
    info->ispt_list = NULL;
    return info;
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_isamc_info *info = ct->buf;
    struct rset_pp_info *ptinfo;

    logf (LOG_DEBUG, "risamc_open");
    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "ISAMC set type is read-only");
	return NULL;
    }
    ptinfo = xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = isc_pp_open (info->is, info->pos);
    ptinfo->info = info;
    return ptinfo;
}

static void r_close (RSFD rfd)
{
    struct rset_isamc_info *info = ((struct rset_pp_info*) rfd)->info;
    struct rset_pp_info **ptinfop;

    for (ptinfop = &info->ispt_list; *ptinfop; ptinfop = &(*ptinfop)->next)
        if (*ptinfop == rfd)
        {
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
    struct rset_isamc_info *info = ct->buf;

    logf (LOG_DEBUG, "rsisamc_delete");
    assert (info->ispt_list == NULL);
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

static int r_hits (RSET ct, void *oi)
{
    return -1;
}

static int r_read (RSFD rfd, void *buf)
{
    return isc_pp_read( ((struct rset_pp_info*) rfd)->pt, buf);
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAMC set type is read-only");
    return -1;
}

static int r_score (RSFD rfd, int *score)
{
    *score = -1;
    return -1;
}
