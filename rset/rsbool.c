/* $Id: rsbool.c,v 1.19 2002-08-02 19:26:57 adam Exp $
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <rsbool.h>
#include <zebrautl.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_count (RSET ct);
static int r_read_and (RSFD rfd, void *buf, int *term_index);
static int r_read_or (RSFD rfd, void *buf, int *term_index);
static int r_read_not (RSFD rfd, void *buf, int *term_index);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control_and = 
{
    "and",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read_and,
    r_write,
};

static const struct rset_control control_or = 
{
    "or",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read_or,
    r_write,
};

static const struct rset_control control_not = 
{
    "not",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read_not,
    r_write,
};


const struct rset_control *rset_kind_and = &control_and;
const struct rset_control *rset_kind_or = &control_or;
const struct rset_control *rset_kind_not = &control_not;

struct rset_bool_info {
    int key_size;
    RSET rset_l;
    RSET rset_r;
    int term_index_s;
    int (*cmp)(const void *p1, const void *p2);
    struct rset_bool_rfd *rfd_list;
};

struct rset_bool_rfd {
    RSFD rfd_l;
    RSFD rfd_r;
    int  more_l;
    int  more_r;
    int term_index_l;
    int term_index_r;
    void *buf_l;
    void *buf_r;
    struct rset_bool_rfd *next;
    struct rset_bool_info *info;
};    

static void *r_create (RSET ct, const struct rset_control *sel, void *parms)
{
    rset_bool_parms *bool_parms = (rset_bool_parms *) parms;
    struct rset_bool_info *info;

    info = (struct rset_bool_info *) xmalloc (sizeof(*info));
    info->key_size = bool_parms->key_size;
    info->rset_l = bool_parms->rset_l;
    info->rset_r = bool_parms->rset_r;
    if (rset_is_volatile(info->rset_l) || rset_is_volatile(info->rset_r))
        ct->flags |= RSET_FLAG_VOLATILE;
    info->cmp = bool_parms->cmp;
    info->rfd_list = NULL;
    
    info->term_index_s = info->rset_l->no_rset_terms;
    ct->no_rset_terms =
	info->rset_l->no_rset_terms + info->rset_r->no_rset_terms;
    ct->rset_terms = (RSET_TERM *)
	xmalloc (sizeof (*ct->rset_terms) * ct->no_rset_terms);

    memcpy (ct->rset_terms, info->rset_l->rset_terms,
	    info->rset_l->no_rset_terms * sizeof(*ct->rset_terms));
    memcpy (ct->rset_terms + info->rset_l->no_rset_terms,
	    info->rset_r->rset_terms,
	    info->rset_r->no_rset_terms * sizeof(*ct->rset_terms));
    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_bool_info *info = (struct rset_bool_info *) ct->buf;
    struct rset_bool_rfd *rfd;

    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "bool set type is read-only");
	return NULL;
    }
    rfd = (struct rset_bool_rfd *) xmalloc (sizeof(*rfd));
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;

    rfd->buf_l = xmalloc (info->key_size);
    rfd->buf_r = xmalloc (info->key_size);
    rfd->rfd_l = rset_open (info->rset_l, RSETF_READ);
    rfd->rfd_r = rset_open (info->rset_r, RSETF_READ);
    rfd->more_l = rset_read (info->rset_l, rfd->rfd_l, rfd->buf_l,
			     &rfd->term_index_l);
    rfd->more_r = rset_read (info->rset_r, rfd->rfd_r, rfd->buf_r,
			     &rfd->term_index_r);
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_bool_info *info = ((struct rset_bool_rfd*)rfd)->info;
    struct rset_bool_rfd **rfdp;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            xfree ((*rfdp)->buf_l);
            xfree ((*rfdp)->buf_r);
            rset_close (info->rset_l, (*rfdp)->rfd_l);
            rset_close (info->rset_r, (*rfdp)->rfd_r);
            *rfdp = (*rfdp)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_bool_info *info = (struct rset_bool_info *) ct->buf;

    assert (info->rfd_list == NULL);
    xfree (ct->rset_terms);
    rset_delete (info->rset_l);
    rset_delete (info->rset_r);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{
    struct rset_bool_info *info = ((struct rset_bool_rfd*)rfd)->info;
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;

    logf (LOG_DEBUG, "rsbool_rewind");
    rset_rewind (info->rset_l, p->rfd_l);
    rset_rewind (info->rset_r, p->rfd_r);
    p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l, &p->term_index_l);
    p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r, &p->term_index_r);
}

static int r_count (RSET ct)
{
    return 0;
}

static int r_read_and (RSFD rfd, void *buf, int *term_index)
{
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;

    while (p->more_l && p->more_r)
    {
        int cmp;

        cmp = (*info->cmp)(p->buf_l, p->buf_r);
        if (!cmp)
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
            return 1;
        }
        else if (cmp == 1)
        {
            memcpy (buf, p->buf_r, info->key_size);

	    *term_index = p->term_index_r + info->term_index_s;
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
            return 1;
        }
        else if (cmp == -1)
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            return 1;
        }
        else if (cmp > 1)
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
        else
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
    }
    while (p->more_l)
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
    while (p->more_r)
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
    return 0;
}

static int r_read_or (RSFD rfd, void *buf, int *term_index)
{
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;

    while (p->more_l || p->more_r)
    {
        int cmp;

        if (p->more_l && p->more_r)
            cmp = (*info->cmp)(p->buf_l, p->buf_r);
        else if (p->more_r)
            cmp = 2;
        else
            cmp = -2;
        if (!cmp)
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
            return 1;
        }
        else if (cmp > 0)
        {
            memcpy (buf, p->buf_r, info->key_size);
	    *term_index = p->term_index_r + info->term_index_s;
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
            return 1;
        }
        else
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            return 1;
        }
    }
    return 0;
}

static int r_read_not (RSFD rfd, void *buf, int *term_index)
{
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;

    while (p->more_l || p->more_r)
    {
        int cmp;

        if (p->more_l && p->more_r)
            cmp = (*info->cmp)(p->buf_l, p->buf_r);
        else if (p->more_r)
            cmp = 2;
        else
            cmp = -2;
        if (cmp < -1)
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            return 1;
        }
        else if (cmp > 1)
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
        else
        {
            memcpy (buf, p->buf_l, info->key_size);
            do
            {
                p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				       &p->term_index_l);
                if (!p->more_l)
                    break;
                cmp = (*info->cmp)(p->buf_l, buf);
            } while (cmp >= -1 && cmp <= 1);
            do
            {
                p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				       &p->term_index_r);
                if (!p->more_r)
                    break;
                cmp = (*info->cmp)(p->buf_r, buf);
            } while (cmp >= -1 && cmp <= 1);
        }
    }
    return 0;
}


static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "bool set type is read-only");
    return -1;
}

