/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisam.c,v $
 * Revision 1.9  1995-09-07 13:58:43  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.8  1995/09/06  16:11:56  adam
 * More work on boolean sets.
 *
 * Revision 1.7  1995/09/06  10:35:44  adam
 * Null set implemented.
 *
 * Revision 1.6  1995/09/05  11:43:24  adam
 * Complete version of temporary sets. Not tested yet though.
 *
 * Revision 1.5  1995/09/04  12:33:56  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.4  1995/09/04  09:10:55  adam
 * Minor changes.
 *
 * Revision 1.3  1994/11/22  13:15:37  quinn
 * Simple
 *
 * Revision 1.2  1994/11/04  14:53:12  quinn
 * Work
 *
 */

#include <stdio.h>
#include <assert.h>
#include <rsisam.h>
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
    "ISAM set type",
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

const rset_control *rset_kind_isam = &control;

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

static rset_control *r_create(const struct rset_control *sel, void *parms)
{
    rset_control *newct;
    rset_isam_parms *pt = parms;
    struct rset_isam_info *info;

    logf (LOG_DEBUG, "rsisam_create(%s)", sel->desc);
    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));

    if (!(newct->buf = xmalloc (sizeof(struct rset_isam_info))))
    	return 0;
    info = newct->buf;
    info->is = pt->is;
    info->pos = pt->pos;
    info->ispt_list = NULL;
    return newct;
}

RSFD r_open (rset_control *ct, int wflag)
{
    struct rset_isam_info *info = ct->buf;
    struct rset_ispt_info *ptinfo;

    logf (LOG_DEBUG, "risam_open");
    if (wflag)
    {
	logf (LOG_FATAL, "ISAM set type is read-only");
	return NULL;
    }
    ptinfo = xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = is_position (info->is, info->pos);
    ptinfo->info = info;
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
            free (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (rset_control *ct)
{
    struct rset_isam_info *info = ct->buf;

    logf (LOG_DEBUG, "rsisam_delete");
    assert (info->ispt_list == NULL);
    xfree (info);
    xfree (ct);
}

static void r_rewind (RSFD rfd)
{   
    logf (LOG_DEBUG, "rsisam_rewind");
    is_rewind( ((struct rset_ispt_info*) rfd)->pt);
}

static int r_count (rset_control *ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf)
{
    return is_readkey( ((struct rset_ispt_info*) rfd)->pt, buf);
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAM set type is read-only");
    return -1;
}
