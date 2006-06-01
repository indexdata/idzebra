/* $Id: rsisams.c,v 1.25 2006-06-01 13:05:52 adam Exp $
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

static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static int r_read (RSFD rfd, void *buf, TERMID *term);
static int r_write (RSFD rfd, const void *buf);
static void r_pos (RSFD rfd, double *current, double *total);

static const struct rset_control control = 
{
    "isams",
    r_delete,
    rset_get_one_term,
    r_open,
    r_close,
    0, /* no foward */
    r_pos,
    r_read,
    r_write,
};

struct rfd_private {
    ISAMS_PP pt;
};

struct rset_private {
    ISAMS   is;
    ISAM_P pos;
};


RSET rsisams_create(NMEM nmem, struct rset_key_control *kcontrol,
		    int scope,
		    ISAMS is, ISAM_P pos, TERMID term)
{
    RSET rnew = rset_create_base(&control, nmem, kcontrol, scope, term, 0, 0);
    struct rset_private *info;
    info = (struct rset_private *) nmem_malloc(rnew->nmem,sizeof(*info));
    rnew->priv = info;
    info->is = is;
    info->pos = pos;
    return rnew;
}

static void r_delete (RSET ct)
{
    yaz_log (YLOG_DEBUG, "rsisams_delete");
}

RSFD r_open (RSET ct, int flag)
{
    struct rset_private *info = (struct rset_private *) ct->priv;
    RSFD rfd;
    struct rfd_private *ptinfo;

    yaz_log (YLOG_DEBUG, "risams_open");
    if (flag & RSETF_WRITE)
    {
        yaz_log (YLOG_FATAL, "ISAMS set type is read-only");
        return NULL;
    }
    rfd=rfd_create_base(ct);
    if (rfd->priv)
        ptinfo=(struct rfd_private *)(rfd->priv);
    else {
        ptinfo = (struct rfd_private *) nmem_malloc(ct->nmem,sizeof(*ptinfo));
        rfd->priv=ptinfo;
    }
    ptinfo->pt = isams_pp_open (info->is, info->pos);
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rfd_private *ptinfo = (struct rfd_private *)(rfd->priv);

    isams_pp_close (ptinfo->pt);
}


static int r_read (RSFD rfd, void *buf, TERMID *term)
{
    struct rfd_private *ptinfo = (struct rfd_private *)(rfd->priv);
    int rc;
    rc=isams_pp_read(ptinfo->pt, buf);
    if (rc && term)
        *term=rfd->rset->term;
    return rc;
}

static int r_write (RSFD rfd, const void *buf)
{
    yaz_log (YLOG_FATAL, "ISAMS set type is read-only");
    return -1;
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    *current=-1;  /* sorry, not implemented yet */
    *total=-1;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

