/* $Id: rsisamc.c,v 1.32 2004-11-15 23:13:12 adam Exp $
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
static int r_read (RSFD rfd, void *buf, TERMID *term);
static int r_write (RSFD rfd, const void *buf);
static void r_pos (RSFD rfd, double *current, double *total);

static const struct rset_control control = 
{
    "isamc",
    r_delete,
    rset_get_one_term,
    r_open,
    r_close,
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

static int log_level = 0;
static int log_level_initialized = 0;

RSET rsisamc_create( NMEM nmem, const struct key_control *kcontrol, int scope,
                             ISAMC is, ISAMC_P pos, TERMID term)
{
    RSET rnew = rset_create_base(&control, nmem, kcontrol, scope,term);
    struct rset_isamc_info *info;
    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("rsisamc");
        log_level_initialized = 1;
    }
    info = (struct rset_isamc_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->is = is;
    info->pos = pos;
    rnew->priv = info;
    yaz_log(log_level, "create: term=%p", term);
    return rnew;
}

static void r_delete (RSET ct)
{
    yaz_log(log_level, "rsisamc_delete");
}


RSFD r_open (RSET ct, int flag)
{
    struct rset_isamc_info *info = (struct rset_isamc_info *) ct->priv;
    RSFD rfd;
    struct rset_pp_info *ptinfo;

    yaz_log(log_level, "risamc_open");
    if (flag & RSETF_WRITE)
    {
        yaz_log(LOG_FATAL, "ISAMC set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        ptinfo = (struct rset_pp_info *)rfd->priv;
    else {
        ptinfo = (struct rset_pp_info *) nmem_malloc (ct->nmem,sizeof(*ptinfo));
        rfd->priv = ptinfo;
        ptinfo->buf = nmem_malloc (ct->nmem,ct->keycontrol->key_size);
    }
    ptinfo->pt = isc_pp_open(info->is, info->pos);
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_pp_info *p = (struct rset_pp_info *)(rfd->priv);

    isc_pp_close(p->pt);
    rfd_delete_base(rfd);
}


static int r_read (RSFD rfd, void *buf, TERMID *term)
{
    struct rset_pp_info *p = (struct rset_pp_info *)(rfd->priv);
    int r;
    r = isc_pp_read(p->pt, buf);
    if (term)
        *term = rfd->rset->term;
    yaz_log(log_level,"read returning term %p", *term);
    return r;
}

static int r_write (RSFD rfd, const void *buf)
{
    yaz_log(LOG_FATAL, "ISAMC set type is read-only");
    return -1;
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    *current = -1;  /* sorry, not implemented yet */
    *total = -1;
}



