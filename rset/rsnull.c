/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsnull.c,v $
 * Revision 1.5  1995-10-10 14:00:04  adam
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
#include <alexutil.h>

static rset_control *r_create(const struct rset_control *sel, void *parms);
static RSFD r_open (rset_control *ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (rset_control *ct);
static void r_rewind (RSFD rfd);
static int r_count (rset_control *ct);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);
static int r_score (RSFD rfd, int *score);

static const rset_control control = 
{
    "NULL set type",
    0,
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write,
    r_score
};

const rset_control *rset_kind_null = &control;

static rset_control *r_create(const struct rset_control *sel, void *parms)
{
    rset_control *newct;

    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));
    return newct;
}

static RSFD r_open (rset_control *ct, int flag)
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

static void r_delete (rset_control *ct)
{
    xfree(ct);
}

static void r_rewind (RSFD rfd)
{
    logf (LOG_DEBUG, "rsnull_rewind");
}

static int r_count (rset_control *ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf)
{
    return 0;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "NULL set type is read-only");
    return -1;
}

static int r_score (RSFD rfd, int *score)
{
    *score = -1;
    return -1;
}

