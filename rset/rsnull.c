/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsnull.c,v $
 * Revision 1.10  1998-03-05 08:36:28  adam
 * New result set model.
 *
 * Revision 1.9  1997/12/18 10:54:25  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.8  1996/10/29 13:55:24  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.7  1995/12/11 09:15:25  adam
 * New set types: sand/sor/snot - ranked versions of and/or/not in
 * ranked/semi-ranked result sets.
 * Note: the snot not finished yet.
 * New rset member: flag.
 * Bug fix: r_delete in rsrel.c did free bad memory block.
 *
 * Revision 1.6  1995/10/12  12:41:57  adam
 * Private info (buf) moved from struct rset_control to struct rset.
 * Bug fixes in relevance.
 *
 * Revision 1.5  1995/10/10  14:00:04  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.4  1995/10/06  14:38:06  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.3  1995/09/08  14:52:42  adam
 * Work on relevance feedback.
 *
 * Revision 1.2  1995/09/07  13:58:43  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.1  1995/09/06  10:35:44  adam
 * Null set implemented.
 *
 */

#include <stdio.h>
#include <rsnull.h>
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
    "null",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_null = &control;

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_null_parms *null_parms = parms;

    ct->no_rset_terms = 1;
    ct->rset_terms = xmalloc (sizeof(*ct->rset_terms));
    if (parms)
	ct->rset_terms[0] = null_parms->rset_term;
    else
	ct->rset_terms[0] = rset_term_create ("term", -1, "rank-0");
    ct->rset_terms[0]->nn = 0;

    return NULL;
}

static RSFD r_open (RSET ct, int flag)
{
    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "NULL set type is read-only");
	return NULL;
    }
    return "";
}

static void r_close (RSFD rfd)
{
}

static void r_delete (RSET ct)
{
    rset_term_destroy (ct->rset_terms[0]);
    xfree (ct->rset_terms);
}

static void r_rewind (RSFD rfd)
{
    logf (LOG_DEBUG, "rsnull_rewind");
}

static int r_count (RSET ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf, int *term_index)
{
    *term_index = -1;
    return 0;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "NULL set type is read-only");
    return -1;
}

