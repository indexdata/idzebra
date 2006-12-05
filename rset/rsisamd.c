/* $Id: rsisamd.c,v 1.8.2.2 2006-12-05 21:14:45 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/




#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <zebrautl.h>
#include <rsisamd.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_read (RSFD rfd, void *buf, int *term_index);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control = 
{
    "isamd",
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

const struct rset_control *rset_kind_isamd = &control;

struct rset_pp_info {
    ISAMD_PP pt;
    struct rset_pp_info *next;
    struct rset_isamd_info *info;
};

struct rset_isamd_info {
    ISAMD   is; 
    /* ISAMD_P pos; */
    char dictentry[ISAMD_MAX_DICT_LEN];
    int dictlen;
    struct rset_pp_info *ispt_list;
};

static void *r_create(RSET ct, const struct rset_control *sel, void *parms)
{
    rset_isamd_parms *pt = (rset_isamd_parms *) parms;
    struct rset_isamd_info *info;

    ct->flags |= RSET_FLAG_VOLATILE;
    info = (struct rset_isamd_info *) xmalloc (sizeof(*info));
    info->is = pt->is;
    /*info->pos = pt->pos;*/
    info->dictlen = pt->dictlen;
    memcpy(info->dictentry, pt->dictentry, pt->dictlen);
    info->ispt_list = NULL;
    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *) xmalloc (sizeof(*ct->rset_terms));
    ct->rset_terms[0] = pt->rset_term;
    return info;
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_isamd_info *info = (struct rset_isamd_info *) ct->buf;
    struct rset_pp_info *ptinfo;

    yaz_log(YLOG_DEBUG, "risamd_open");
    if (flag & RSETF_WRITE)
    {
	yaz_log(YLOG_FATAL, "ISAMD set type is read-only");
	return NULL;
    }
    ptinfo = (struct rset_pp_info *) xmalloc (sizeof(*ptinfo));
    ptinfo->next = info->ispt_list;
    info->ispt_list = ptinfo;
    ptinfo->pt = isamd_pp_open (info->is, info->dictentry, info->dictlen);
    ptinfo->info = info;
    if (ct->rset_terms[0]->nn < 0)
	ct->rset_terms[0]->nn = isamd_pp_num (ptinfo->pt);
    return ptinfo;
}

static void r_close (RSFD rfd)
{
    struct rset_isamd_info *info = ((struct rset_pp_info*) rfd)->info;
    struct rset_pp_info **ptinfop;

    for (ptinfop = &info->ispt_list; *ptinfop; ptinfop = &(*ptinfop)->next)
        if (*ptinfop == rfd)
        {
            isamd_pp_close ((*ptinfop)->pt);
            *ptinfop = (*ptinfop)->next;
            xfree (rfd);
            return;
        }
    yaz_log(YLOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_isamd_info *info = (struct rset_isamd_info *) ct->buf;

    yaz_log(YLOG_DEBUG, "rsisamd_delete");
    assert (info->ispt_list == NULL);
    rset_term_destroy (ct->rset_terms[0]);
    xfree (ct->rset_terms);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{   
    yaz_log(YLOG_DEBUG, "rsisamd_rewind");
    abort ();
}


static int r_read (RSFD rfd, void *buf, int *term_index)
{
    *term_index = 0;
    return isamd_pp_read( ((struct rset_pp_info*) rfd)->pt, buf);
}

static int r_write (RSFD rfd, const void *buf)
{
    yaz_log(YLOG_FATAL, "ISAMD set type is read-only");
    return -1;
}
