/* $Id: rsisamb.c,v 1.31 2005-04-13 13:03:48 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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
#include <idzebra/util.h>
#include <rset.h>
#include <string.h>


static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static int r_forward(RSFD rfd, void *buf, TERMID *term, const void *untilbuf);
static void r_pos (RSFD rfd, double *current, double *total);
static int r_read (RSFD rfd, void *buf, TERMID *term);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control = 
{
    "isamb",
    r_delete,
    rset_get_one_term,
    r_open,
    r_close,
    r_forward, 
    r_pos,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_isamb = &control;

struct rset_pp_info {
    ISAMB_PP pt;
    void *buf;
};

struct rset_isamb_info {
    ISAMB   is;
    ISAM_P pos;
};

static int log_level = 0;
static int log_level_initialized = 0;

RSET rsisamb_create(NMEM nmem, const struct key_control *kcontrol, int scope,
		    ISAMB is, ISAM_P pos, TERMID term)
{
    RSET rnew = rset_create_base(&control, nmem, kcontrol, scope, term);
    struct rset_isamb_info *info;
    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("rsisamb");
        log_level_initialized = 1;
    }
    info = (struct rset_isamb_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->is = is;
    info->pos = pos;
    rnew->priv = info;
    yaz_log(log_level,"rsisamb_create");
    return rnew;
}

static void r_delete (RSET ct)
{
    yaz_log(log_level, "rsisamb_delete");
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_isamb_info *info = (struct rset_isamb_info *) ct->priv;
    RSFD rfd;
    struct rset_pp_info *ptinfo;

    if (flag & RSETF_WRITE)
    {
        yaz_log(YLOG_FATAL, "ISAMB set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        ptinfo = (struct rset_pp_info *) (rfd->priv);
    else {
        ptinfo = (struct rset_pp_info *)nmem_malloc(ct->nmem,sizeof(*ptinfo));
        ptinfo->buf = nmem_malloc (ct->nmem,ct->keycontrol->key_size);
        rfd->priv = ptinfo;
    }
    ptinfo->pt = isamb_pp_open (info->is, info->pos, ct->scope );
    yaz_log(log_level,"rsisamb_open");
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_pp_info *ptinfo = (struct rset_pp_info *)(rfd->priv);
    isamb_pp_close (ptinfo->pt);
    rfd_delete_base(rfd);
    yaz_log(log_level,"rsisamb_close");
}


static int r_forward(RSFD rfd, void *buf, TERMID *term, const void *untilbuf)
{
    struct rset_pp_info *pinfo = (struct rset_pp_info *)(rfd->priv);
    int rc;
    rc = isamb_pp_forward(pinfo->pt, buf, untilbuf);
    if (rc && term)
        *term = rfd->rset->term;
    yaz_log(log_level,"rsisamb_forward");
    return rc; 
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    struct rset_pp_info *pinfo = (struct rset_pp_info *)(rfd->priv);
    assert(rfd);
    isamb_pp_pos(pinfo->pt, current, total);
    yaz_log(log_level,"isamb.r_pos returning %0.1f/%0.1f",
              *current, *total);
}

static int r_read (RSFD rfd, void *buf, TERMID *term)
{
    struct rset_pp_info *pinfo = (struct rset_pp_info *)(rfd->priv);
    int rc;
    rc = isamb_pp_read(pinfo->pt, buf);
    if (rc && term)
        *term = rfd->rset->term;
    yaz_log(log_level,"isamb.r_read ");
    return rc;
}

static int r_write (RSFD rfd, const void *buf)
{
    yaz_log(YLOG_FATAL, "ISAMB set type is read-only");
    return -1;
}
