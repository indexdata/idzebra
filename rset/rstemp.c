/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rstemp.c,v $
 * Revision 1.3  1995-09-04 15:20:40  adam
 * More work on temp sets. is_open member removed.
 *
 * Revision 1.2  1995/09/04  09:10:56  adam
 * Minor changes.
 *
 * Revision 1.1  1994/11/04  13:21:30  quinn
 * Working.
 *
 */

#include <stdio.h>

#include <alexutil.h>
#include <rstemp.h>

static struct rset_control *r_create(const struct rset_control *sel, 
                                     void *parms);
static int r_open(struct rset_control *ct, int wflag);
static void r_close(struct rset_control *ct);
static void r_delete(struct rset_control *ct);
static void r_rewind(struct rset_control *ct);
static int r_count(struct rset_control *ct);
static int r_read();
static int r_write();

static const rset_control control = 
{
    "Temporary set",
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

const rset_control *rset_kind_temp = &control;

struct rset_temp_private {
    int     fd;
    char   *fname;
    size_t  key_size;
    char   *buf_mem;
    size_t  buf_size;
    size_t  pos_end;
    size_t  pos_cur;
    size_t  pos_buf;
};

static struct rset_control *r_create(const struct rset_control *sel,
                                     void *parms)
{
    rset_control *newct;
    rset_temp_parms *temp_parms = parms;
    struct rset_temp_private *info;
    
    logf (LOG_DEBUG, "ritemp_create(%s)", sel->desc);
    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));
    newct->buf = xmalloc (sizeof(struct rset_temp_private));
    info = newct->buf;

    info->fd = -1;
    info->fname = NULL;
    info->key_size = temp_parms->key_size;
    info->buf_size = 1024;
    info->buf_mem = xmalloc (info->buf_size);
    info->pos_cur = 0;
    info->pos_end = 0;
    info->pos_buf = 0;

    return newct;
}

static int r_open(struct rset_control *ct, int wflag)
{
    struct rset_temp_private *info = ct->buf;
    info->pos_cur = 0;
    info->pos_buf = 0;
}

static void r_close(struct rset_control *ct)
{}

static void r_delete(struct rset_control *ct)
{}

static void r_rewind(struct rset_control *ct)
{}

static int r_count(struct rset_control *ct)
{}

static int r_read()
{}

static int r_write()
{}
