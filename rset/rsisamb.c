/* $Id: rsisamb.c,v 1.17 2004-08-24 14:25:16 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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
#include <rsisamb.h>
#include <string.h>
#include <../index/index.h> /* for log_keydump. Debugging only */

#ifndef RSET_DEBUG
#define RSET_DEBUG 0
#endif

static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_forward(RSET ct, RSFD rfd, void *buf,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);
static void r_pos (RSFD rfd, double *current, double *total);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control = 
{
    "isamb",
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_forward, /* rset_default_forward, */
    r_pos,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_isamb = &control;

struct rset_pp_info {
    ISAMB_PP pt;
    struct rset_pp_info *next;
    struct rset_isamb_info *info;
    void *buf;
};

struct rset_isamb_info {
    ISAMB   is;
    ISAMB_P pos;
    int key_size;
    int (*cmp)(const void *p1, const void *p2);
    struct rset_pp_info *ispt_list;
};

RSET rsisamb_create( NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            ISAMB is, ISAMB_P pos)
{
    RSET rnew=rset_create_base(&control, nmem);
    struct rset_isamb_info *info;
    info = (struct rset_isamb_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->key_size = key_size;
    info->cmp = cmp;
    info->is=is;
    info->pos=pos;
    info->ispt_list = NULL;
    rnew->priv=info;
    return rnew;
}

static void r_delete (RSET ct)
{
    struct rset_isamb_info *info = (struct rset_isamb_info *) ct->priv;

    logf (LOG_DEBUG, "rsisamb_delete");
    assert (info->ispt_list == NULL);
}

#if 0
static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_isamb_parms *pt = (rset_isamb_parms *) parms;
    struct rset_isamb_info *info;

    info = (struct rset_isamb_info *) xmalloc (sizeof(*info));
    info->is = pt->is;
    info->pos = pt->pos;
    info->key_size = pt->key_size;
    info->cmp = pt->cmp;
    info->ispt_list = NULL;
    return info;
}
#endif

RSFD r_open (RSET ct, int flag)
{
    struct rset_isamb_info *info = (struct rset_isamb_info *) ct->priv;
    struct rset_pp_info *ptinfo;

    logf (LOG_DEBUG, "risamb_open");
    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "ISAMB set type is read-only");
	return NULL;
    }
    ptinfo = (struct rset_pp_info *) xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = isamb_pp_open (info->is, info->pos);
    ptinfo->info = info;
    ptinfo->buf = xmalloc (info->key_size);
    return ptinfo;
}

static void r_close (RSFD rfd)
{
    struct rset_isamb_info *info = ((struct rset_pp_info*) rfd)->info;
    struct rset_pp_info **ptinfop;

    for (ptinfop = &info->ispt_list; *ptinfop; ptinfop = &(*ptinfop)->next)
        if (*ptinfop == rfd)
        {
            xfree ((*ptinfop)->buf);
            isamb_pp_close ((*ptinfop)->pt);
            *ptinfop = (*ptinfop)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}


static void r_rewind (RSFD rfd)
{   
    logf (LOG_DEBUG, "rsisamb_rewind");
    abort ();
}

static int r_forward(RSET ct, RSFD rfd, void *buf, 
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf)
{
    int i; 
    struct rset_pp_info *pinfo = (struct rset_pp_info *) rfd;
#if RSET_DEBUG
    logf (LOG_DEBUG, "rset_rsisamb_forward starting '%s' (ct=%p rfd=%p)",
                      ct->control->desc, ct,rfd);
    key_logdump(LOG_DEBUG, untilbuf);
    key_logdump(LOG_DEBUG, buf);
#endif

    i=isamb_pp_forward(pinfo->pt, buf, untilbuf);
#if RSET_DEBUG
    logf (LOG_DEBUG, "rset_rsisamb_forward returning %d",i);
#endif
    return i;
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    struct rset_pp_info *pinfo = (struct rset_pp_info *) rfd;
    assert(rfd);
    isamb_pp_pos(pinfo->pt, current, total);
#if RSET_DEBUG
    logf(LOG_DEBUG,"isamb.r_pos returning %0.1f/%0.1f",
              *current, *total);
#endif
}

static int r_read (RSFD rfd, void *buf)
{
    struct rset_pp_info *pinfo = (struct rset_pp_info *) rfd;
    int r;

    r = isamb_pp_read(pinfo->pt, buf);
    return r;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAMB set type is read-only");
    return -1;
}
