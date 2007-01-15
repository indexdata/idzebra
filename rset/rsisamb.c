/* $Id: rsisamb.c,v 1.38 2007-01-15 15:10:19 adam Exp $
   Copyright (C) 1995-2007
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <assert.h>
#include <idzebra/util.h>
#include <rset.h>
#include <string.h>

static RSFD r_open(RSET ct, int flag);
static void r_close(RSFD rfd);
static void r_delete(RSET ct);
static int r_forward(RSFD rfd, void *buf, TERMID *term, const void *untilbuf);
static void r_pos(RSFD rfd, double *current, double *total);
static int r_read(RSFD rfd, void *buf, TERMID *term);
static int r_read_filter(RSFD rfd, void *buf, TERMID *term);
static int r_write(RSFD rfd, const void *buf);

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

static const struct rset_control control_filter = 
{
    "isamb",
    r_delete,
    rset_get_one_term,
    r_open,
    r_close,
    r_forward, 
    r_pos,
    r_read_filter,
    r_write,
};

struct rfd_private {
    ISAMB_PP pt;
    void *buf;
};

struct rset_private {
    ISAMB   is;
    ISAM_P pos;
};

static int log_level = 0;
static int log_level_initialized = 0;

RSET rsisamb_create(NMEM nmem, struct rset_key_control *kcontrol,
		    int scope,
		    ISAMB is, ISAM_P pos, TERMID term)
{
    RSET rnew = rset_create_base(
	kcontrol->filter_func ? &control_filter : &control,
	nmem, kcontrol, scope, term, 0, 0);
    struct rset_private *info;
    assert(pos);
    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("rsisamb");
        log_level_initialized = 1;
    }
    info = (struct rset_private *) nmem_malloc(rnew->nmem, sizeof(*info));
    info->is = is;
    info->pos = pos;
    rnew->priv = info;
    yaz_log(log_level, "rsisamb_create");
    return rnew;
}

static void r_delete(RSET ct)
{
    yaz_log(log_level, "rsisamb_delete");
}

RSFD r_open(RSET ct, int flag)
{
    struct rset_private *info = (struct rset_private *) ct->priv;
    RSFD rfd;
    struct rfd_private *ptinfo;

    if (flag & RSETF_WRITE)
    {
        yaz_log(YLOG_FATAL, "ISAMB set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        ptinfo = (struct rfd_private *) (rfd->priv);
    else {
        ptinfo = (struct rfd_private *) nmem_malloc(ct->nmem,sizeof(*ptinfo));
        ptinfo->buf = nmem_malloc(ct->nmem,ct->keycontrol->key_size);
        rfd->priv = ptinfo;
    }
    ptinfo->pt = isamb_pp_open(info->is, info->pos, ct->scope );
    yaz_log(log_level, "rsisamb_open");
    return rfd;
}

static void r_close(RSFD rfd)
{
    struct rfd_private *ptinfo = (struct rfd_private *)(rfd->priv);
    isamb_pp_close (ptinfo->pt);
    yaz_log(log_level, "rsisamb_close");
}


static int r_forward(RSFD rfd, void *buf, TERMID *term, const void *untilbuf)
{
    struct rfd_private *pinfo = (struct rfd_private *)(rfd->priv);
    int rc;
    rc = isamb_pp_forward(pinfo->pt, buf, untilbuf);
    if (rc && term)
        *term = rfd->rset->term;
    yaz_log(log_level, "rsisamb_forward");
    return rc; 
}

static void r_pos(RSFD rfd, double *current, double *total)
{
    struct rfd_private *pinfo = (struct rfd_private *)(rfd->priv);
    assert(rfd);
    isamb_pp_pos(pinfo->pt, current, total);
    yaz_log(log_level, "isamb.r_pos returning %0.1f/%0.1f",
              *current, *total);
}

static int r_read(RSFD rfd, void *buf, TERMID *term)
{
    struct rfd_private *pinfo = (struct rfd_private *)(rfd->priv);
    int rc;
    rc = isamb_pp_read(pinfo->pt, buf);
    if (rc && term)
        *term = rfd->rset->term;
    yaz_log(log_level, "isamb.r_read ");
    return rc;
}

static int r_read_filter(RSFD rfd, void *buf, TERMID *term)
{
    struct rfd_private *pinfo = (struct rfd_private *)rfd->priv;
    const struct rset_key_control *kctrl = rfd->rset->keycontrol;
    int rc;
    while((rc = isamb_pp_read(pinfo->pt, buf)))
    {
	int incl = (*kctrl->filter_func)(buf, kctrl->filter_data);
	if (incl)
	    break;
    }
    if (rc && term)
        *term = rfd->rset->term;
    yaz_log(log_level, "isamb.r_read_filter");
    return rc;
}

static int r_write(RSFD rfd, const void *buf)
{
    yaz_log(YLOG_FATAL, "ISAMB set type is read-only");
    return -1;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

