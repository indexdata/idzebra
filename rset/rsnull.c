/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsnull.c,v $
 * Revision 1.1  1995-09-06 10:35:44  adam
 * Null set implemented.
 *
 */

#include <stdio.h>
#include <rsnull.h>
#include <alexutil.h>

static rset_control *r_create(const struct rset_control *sel, void *parms);
static int r_open (rset_control *ct, int wflag);
static void r_close (rset_control *ct);
static void r_delete (rset_control *ct);
static void r_rewind (rset_control *ct);
static int r_count (rset_control *ct);
static int r_read (rset_control *ct, void *buf);
static int r_write (rset_control *ct, const void *buf);

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
    r_write
};

const rset_control *rset_kind_null = &control;

static rset_control *r_create(const struct rset_control *sel, void *parms)
{
    rset_control *newct;

    logf (LOG_DEBUG, "rsnull_create(%s)", sel->desc);
    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));
    return newct;
}

static int r_open(rset_control *ct, int wflag)
{
    if (wflag)
    {
	logf (LOG_FATAL, "NULL set type is read-only");
	return -1;
    }
    return 0;
}

static void r_close(rset_control *ct)
{
    /* NOP */
}

static void r_delete(rset_control *ct)
{
    xfree(ct);
}

static void r_rewind(rset_control *ct)
{
    logf (LOG_DEBUG, "rsnull_rewind");
}

static int r_count (rset_control *ct)
{
    return 0;
}

static int r_read (rset_control *ct, void *buf)
{
    return 0;
}

static int r_write (rset_control *ct, const void *buf)
{
    logf (LOG_FATAL, "NULL set type is read-only");
    return -1;
}
