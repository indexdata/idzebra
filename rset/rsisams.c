/* $Id: rsisams.c,v 1.8 2004-08-20 14:44:46 heikki Exp $
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
#include <rsisams.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control = 
{
    "isams",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    rset_default_forward,
    rset_default_pos,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_isams = &control;

struct rset_pp_info {
    ISAMS_PP pt;
    struct rset_pp_info *next;
    struct rset_isams_info *info;
};

struct rset_isams_info {
    ISAMS   is;
    ISAMS_P pos;
    struct rset_pp_info *ispt_list;
};

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_isams_parms *pt = (struct rset_isams_parms *) parms;
    struct rset_isams_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
    info = (struct rset_isams_info *) xmalloc (sizeof(*info));
    info->is = pt->is;
    info->pos = pt->pos;
    info->ispt_list = NULL;
    return info;
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_isams_info *info = (struct rset_isams_info *) ct->buf;
    struct rset_pp_info *ptinfo;

    logf (LOG_DEBUG, "risams_open");
    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "ISAMS set type is read-only");
        return NULL;
    }
    ptinfo = (struct rset_pp_info *) xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = isams_pp_open (info->is, info->pos);
    ptinfo->info = info;
    return ptinfo;
}

static void r_close (RSFD rfd)
{
    struct rset_isams_info *info = ((struct rset_pp_info*) rfd)->info;
    struct rset_pp_info **ptinfop;

    for (ptinfop = &info->ispt_list; *ptinfop; ptinfop = &(*ptinfop)->next)
        if (*ptinfop == rfd)
        {
            isams_pp_close ((*ptinfop)->pt);
            *ptinfop = (*ptinfop)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_isams_info *info = (struct rset_isams_info *) ct->buf;

    logf (LOG_DEBUG, "rsisams_delete");
    assert (info->ispt_list == NULL);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{   
    logf (LOG_DEBUG, "rsisams_rewind");
    abort ();
}


static int r_read (RSFD rfd, void *buf)
{
    return isams_pp_read( ((struct rset_pp_info*) rfd)->pt, buf);
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAMS set type is read-only");
    return -1;
}
