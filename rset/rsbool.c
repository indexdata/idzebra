/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsbool.c,v $
 * Revision 1.2  1995-09-06 16:11:55  adam
 * More work on boolean sets.
 *
 * Revision 1.1  1995/09/06  13:27:15  adam
 * New set type: bool. Not finished yet.
 *
 */

#include <stdio.h>
#include <rsbool.h>
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
    "BOOL set type",
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

const rset_control *rset_kind_bool = &control;

struct rset_bool_info {
    int key_size;
    int op;
    RSET rset_l;
    RSET rset_r;
    int more_l;
    int more_r;
    void *buf_l;
    void *buf_r;
    int (*cmp)(const void *p1, const void *p2);
};

static rset_control *r_create(const struct rset_control *sel, void *parms)
{
    rset_control *newct;
    rset_bool_parms *bool_parms = parms;
    struct rset_bool_info *info;

    logf (LOG_DEBUG, "rsbool_create(%s)", sel->desc);
    newct = xmalloc(sizeof(*newct));
    memcpy (newct, sel, sizeof(*sel));
    newct->buf = xmalloc (sizeof(struct rset_bool_info));
    info = (struct rset_bool_info*) newct->buf;
    info->key_size = bool_parms->key_size;
    info->op = bool_parms->op;
    info->rset_l = bool_parms->rset_l;
    info->rset_r = bool_parms->rset_r;
    info->cmp = bool_parms->cmp;
    info->buf_l = xmalloc (info->key_size);
    info->buf_r = xmalloc (info->key_size);
    return newct;
}

static int r_open(rset_control *ct, int wflag)
{
    struct rset_bool_info *info = ct->buf;

    if (wflag)
    {
	logf (LOG_FATAL, "bool set type is read-only");
	return -1;
    }
    rset_open (info->rset_l, wflag);
    rset_open (info->rset_r, wflag);
    info->more_l = rset_read (info->rset_l, info->buf_l);
    info->more_r = rset_read (info->rset_r, info->buf_r);
    return 0;
}

static void r_close(rset_control *ct)
{
    struct rset_bool_info *info = ct->buf;

    rset_close (info->rset_l);
    rset_close (info->rset_r);
}

static void r_delete(rset_control *ct)
{
    struct rset_bool_info *info = ct->buf;

    rset_delete (info->rset_l);
    rset_delete (info->rset_r);
    xfree (info->buf_l);
    xfree (info->buf_r);
    xfree (ct->buf);
    xfree (ct);
}

static void r_rewind(rset_control *ct)
{
    struct rset_bool_info *info = ct->buf;

    logf (LOG_DEBUG, "rsbool_rewind");
    rset_rewind (info->rset_l);
    rset_rewind (info->rset_r);
}

static int r_count (rset_control *ct)
{
    return 0;
}

static int r_read (rset_control *ct, void *buf)
{
    struct rset_bool_info *info = ct->buf;

    if (!info->more_l && !info->more_r)
        return 0;
    return 0;
}

static int r_write (rset_control *ct, const void *buf)
{
    logf (LOG_FATAL, "bool set type is read-only");
    return -1;
}
