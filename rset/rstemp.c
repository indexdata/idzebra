/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rstemp.c,v $
 * Revision 1.2  1995-09-04 09:10:56  adam
 * Minor changes.
 *
 * Revision 1.1  1994/11/04  13:21:30  quinn
 * Working.
 *
 */

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

static struct rset_control *r_create(const struct rset_control *sel, void *parms)
{}

static int r_open(struct rset_control *ct, int wflag)
{}

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
