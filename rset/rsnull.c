/* $Id: rsnull.c,v 1.20 2004-08-06 10:09:28 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#include <stdio.h>
#include <assert.h>
#include <zebrautl.h>
#include <rsnull.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static void r_pos (RSFD rfd, double *current, double *total);
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
    rset_default_forward,
    r_pos,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_null = &control;

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_null_parms *null_parms = (rset_null_parms *) parms;

    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *) xmalloc (sizeof(*ct->rset_terms));
    if (parms && null_parms->rset_term)
	ct->rset_terms[0] = null_parms->rset_term;
    else
	ct->rset_terms[0] = rset_term_create ("term", -1, "rank-0",
                                              0);
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

static void r_pos (RSFD rfd, double *current, double *total)
{
    assert(rfd);
    assert(current);
    assert(total);
    *total=0;
    *current=0;
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

