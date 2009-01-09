/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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
    "isamc",
    r_delete,
    rset_get_one_term,
    r_open,
    r_close,
    0, /* no forward */
    r_pos,
    r_read,
    r_write,
};

struct rset_pp_info {
    ISAMC_PP pt;
    void *buf;
};

struct rset_isamc_info {
    ISAMC   is;
    ISAM_P pos;
};

static int log_level = 0;
static int log_level_initialized = 0;

RSET rsisamc_create(NMEM nmem, struct rset_key_control *kcontrol,
		    int scope,
		    ISAMC is, ISAM_P pos, TERMID term)
{
    RSET rnew = rset_create_base(&control, nmem, kcontrol, scope, term, 0, 0);
    struct rset_isamc_info *info;
    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("rsisamc");
        log_level_initialized = 1;
    }
    info = (struct rset_isamc_info *) nmem_malloc(rnew->nmem, sizeof(*info));
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
        yaz_log(YLOG_FATAL, "ISAMC set type is read-only");
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
    ptinfo->pt = isamc_pp_open(info->is, info->pos);
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_pp_info *p = (struct rset_pp_info *)(rfd->priv);

    isamc_pp_close(p->pt);
}


static int r_read (RSFD rfd, void *buf, TERMID *term)
{
    struct rset_pp_info *p = (struct rset_pp_info *)(rfd->priv);
    int r;
    r = isamc_pp_read(p->pt, buf);
    if (term)
        *term = rfd->rset->term;
    yaz_log(log_level, "isamc.r_read");
    return r;
}

static int r_write (RSFD rfd, const void *buf)
{
    yaz_log(YLOG_FATAL, "ISAMC set type is read-only");
    return -1;
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    *current = -1;  /* sorry, not implemented yet */
    *total = -1;
}



/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

