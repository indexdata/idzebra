/* $Id: rsisamc.c,v 1.25 2004-09-09 10:08:06 heikki Exp $
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
#include <string.h>
#include <zebrautl.h>
#include <rset.h>

static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);
static void r_pos (RSFD rfd, double *current, double *total);

static const struct rset_control control = 
{
    "isamc",
    r_delete,
    r_open,
    r_close,
    r_rewind,
    rset_default_forward,
    r_pos,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_isamc = &control;

struct rset_pp_info {
    ISAMC_PP pt;
    void *buf;
};

struct rset_isamc_info {
    ISAMC   is;
    ISAMC_P pos;
};

RSET rsisamc_create( NMEM nmem, const struct key_control *kcontrol, int scope,
                             ISAMC is, ISAMC_P pos)
{
    RSET rnew=rset_create_base(&control, nmem, kcontrol, scope);
    struct rset_isamc_info *info;
    info = (struct rset_isamc_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->is=is;
    info->pos=pos;
    rnew->priv=info;
    return rnew;
}

static void r_delete (RSET ct)
{
    logf (LOG_DEBUG, "rsisamc_delete");
}


RSFD r_open (RSET ct, int flag)
{
    RSFD rfd;
    struct rset_pp_info *ptinfo;

    logf (LOG_DEBUG, "risamc_open");
    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "ISAMC set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        ptinfo=(struct rset_pp_info *)rfd->priv;
    else {
        ptinfo = (struct rset_pp_info *) nmem_malloc (ct->nmem,sizeof(*ptinfo));
        rfd->priv=ptinfo;
        ptinfo->buf = nmem_malloc (ct->nmem,ct->keycontrol->key_size);
    }
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_pp_info *p=(struct rset_pp_info *)(rfd->priv);

    isc_pp_close (p->pt);
    rfd_delete_base(rfd);
}


static void r_rewind (RSFD rfd)
{   
    logf (LOG_FATAL, "rsisamc_rewind");
    abort ();
}

static int r_read (RSFD rfd, void *buf)
{
    struct rset_pp_info *p=(struct rset_pp_info *)(rfd->priv);
    int r;
    r = isc_pp_read(p->pt, buf);
    return r;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "ISAMC set type is read-only");
    return -1;
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    *current=-1;  /* sorry, not implemented yet */
    *total=-1;
}

