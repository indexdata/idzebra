/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisam.c,v $
 * Revision 1.20  1999-05-26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.19  1999/02/02 14:51:34  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.18  1998/03/05 08:36:28  adam
 * New result set model.
 *
 * Revision 1.17  1997/12/18 10:54:25  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.16  1997/10/31 12:37:01  adam
 * Code calls xfree() instead of free().
 *
 * Revision 1.15  1996/10/29 13:55:22  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.14  1995/12/11 09:15:24  adam
 * New set types: sand/sor/snot - ranked versions of and/or/not in
 * ranked/semi-ranked result sets.
 * Note: the snot not finished yet.
 * New rset member: flag.
 * Bug fix: r_delete in rsrel.c did free bad memory block.
 *
 * Revision 1.13  1995/10/12  12:41:56  adam
 * Private info (buf) moved from struct rset_control to struct rset.
 * Bug fixes in relevance.
 *
 * Revision 1.12  1995/10/10  14:00:04  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.11  1995/10/06  14:38:05  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.10  1995/09/08  14:52:42  adam
 * Work on relevance feedback.
 *
 * Revision 1.9  1995/09/07  13:58:43  adam
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
