/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <idzebra/util.h>
#include <rset.h>

#ifndef RSET_DEBUG
#define RSET_DEBUG 0
#endif

static RSFD r_open(RSET ct, int flag);
static void r_close(RSFD rfd);
static void r_delete(RSET ct);
static int r_forward(RSFD rfd, void *buf, TERMID *term, const void *untilbuf);
static void r_pos(RSFD rfd, double *current, double *total); 
static int r_read_not(RSFD rfd, void *buf, TERMID *term);
static int r_write(RSFD rfd, const void *buf);
static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm);

static const struct rset_control control_not = 
{
    "not",
    r_delete,
    r_get_terms,
    r_open,
    r_close,
    r_forward, 
    r_pos,
    r_read_not,
    r_write,
};

struct rset_private {
    RSET rset_l;
    RSET rset_r;
};

struct rfd_private {
    zint hits;
    RSFD rfd_l;
    RSFD rfd_r;
    int  more_l;
    int  more_r;
    void *buf_l;
    void *buf_r;
    TERMID term_l;
    TERMID term_r;
    int tail;
};    

static RSET rsbool_create_base(const struct rset_control *ctrl,
			       NMEM nmem,
			       struct rset_key_control *kcontrol,
			       int scope, RSET rset_l, RSET rset_r)
{
    RSET children[2], rnew;
    struct rset_private *info;

    children[0] = rset_l;
    children[1] = rset_r;
    rnew = rset_create_base(ctrl, nmem, kcontrol, scope, 0, 2, children);
    info = (struct rset_private *) nmem_malloc(rnew->nmem, sizeof(*info));
    info->rset_l = rset_l;
    info->rset_r = rset_r;
    rnew->priv = info;
    return rnew;
}

RSET rset_create_not(NMEM nmem, struct rset_key_control *kcontrol,
                     int scope, RSET rset_l, RSET rset_r)
{
    return rsbool_create_base(&control_not, nmem, kcontrol,
                              scope, rset_l, rset_r);
}

static void r_delete(RSET ct)
{
}

static RSFD r_open(RSET ct, int flag)
{
    struct rset_private *info = (struct rset_private *) ct->priv;
    RSFD rfd;
    struct rfd_private *p;

    if (flag & RSETF_WRITE)
    {
        yaz_log(YLOG_FATAL, "bool set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        p = (struct rfd_private *)rfd->priv;
    else {
        p = nmem_malloc(ct->nmem,sizeof(*p));
        rfd->priv = p;
        p->buf_l = nmem_malloc(ct->nmem, ct->keycontrol->key_size);
        p->buf_r = nmem_malloc(ct->nmem, ct->keycontrol->key_size);
    }

    yaz_log(YLOG_DEBUG,"rsbool (%s) open [%p]", ct->control->desc, rfd);
    p->hits=0;

    p->rfd_l = rset_open (info->rset_l, RSETF_READ);
    p->rfd_r = rset_open (info->rset_r, RSETF_READ);
    p->more_l = rset_read(p->rfd_l, p->buf_l, &p->term_l);
    p->more_r = rset_read(p->rfd_r, p->buf_r, &p->term_r);
    p->tail = 0;
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rfd_private *prfd=(struct rfd_private *)rfd->priv;

    rset_close (prfd->rfd_l);
    rset_close (prfd->rfd_r);
}

static int r_forward(RSFD rfd, void *buf, TERMID *term,
                     const void *untilbuf)
{
    struct rfd_private *p = (struct rfd_private *)rfd->priv;
    const struct rset_key_control *kctrl=rfd->rset->keycontrol;

    if ( p->more_l && ((kctrl->cmp)(untilbuf,p->buf_l)>=rfd->rset->scope) )
        p->more_l = rset_forward(p->rfd_l, p->buf_l, &p->term_l, untilbuf);
    if ( p->more_r && ((kctrl->cmp)(untilbuf,p->buf_r)>=rfd->rset->scope))
        p->more_r = rset_forward(p->rfd_r, p->buf_r, &p->term_r, untilbuf);
    p->tail = 0; 
    return rset_read(rfd,buf,term); 
}


/*
    1,1         1,3
    1,9         2,1
    1,11        3,1
    2,9

  1,1     1,1
  1,3     1,3
          1,9
          1,11
  2,1     2,1
          2,9
          3,1
*/

static int r_read_not(RSFD rfd, void *buf, TERMID *term)
{
    struct rfd_private *p = (struct rfd_private *)rfd->priv;
    const struct rset_key_control *kctrl = rfd->rset->keycontrol;

    while (p->more_l)
    {
        int cmp;

        if (p->more_r)
            cmp = (*kctrl->cmp)(p->buf_l, p->buf_r);
        else
            cmp = -rfd->rset->scope;

        if (cmp <= -rfd->rset->scope)
        { /* cmp == -2 */
            memcpy (buf, p->buf_l, kctrl->key_size);
            if (term)
                *term=p->term_l;
            p->more_l = rset_read(p->rfd_l, p->buf_l, &p->term_l);
            p->hits++;
            return 1;
        }
        else if (cmp >= rfd->rset->scope)   /* cmp >1 */
        {
            p->more_r = rset_forward( p->rfd_r, p->buf_r, 
                                      &p->term_r, p->buf_l);
        }
        else
        { /* cmp== -1, 0, or 1 */
            memcpy (buf, p->buf_l, kctrl->key_size);
            if (term)
                *term = p->term_l;
            do
            { 
                p->more_l = rset_read(p->rfd_l, p->buf_l, &p->term_l);
                if (!p->more_l)
                    break;
                cmp = (*kctrl->cmp)(p->buf_l, buf);
            } while (abs(cmp)<rfd->rset->scope);
                /*  (cmp >= -1 && cmp <= 1) */
            do
            {
                p->more_r = rset_read(p->rfd_r, p->buf_r, &p->term_r);
                if (!p->more_r)
                    break;
                cmp = (*kctrl->cmp)(p->buf_r, buf);
            } while (abs(cmp)<rfd->rset->scope);
               /* (cmp >= -1 && cmp <= 1) */
        }
    }
    return 0;
}


static int r_write(RSFD rfd, const void *buf)
{
    yaz_log(YLOG_FATAL, "bool set type is read-only");
    return -1;
}

static void r_pos(RSFD rfd, double *current, double *total)
{
    struct rfd_private *p = (struct rfd_private *)rfd->priv;
    double lcur, ltot;
    double rcur, rtot;
    double r;
    ltot = -1;
    rtot = -1;
    rset_pos(p->rfd_l, &lcur, &ltot);
    rset_pos(p->rfd_r, &rcur, &rtot);
    if ( (rtot<0) && (ltot<0)) { /*no position */
        *current = rcur;  /* return same as you got */
        *total = rtot;    /* probably -1 for not available */
    }
    if (rtot < 0) 
	rtot = rcur = 0; /* if only one useful, use it */ 
    if (ltot < 0) 
	ltot = lcur = 0;
    if (rtot+ltot < 1) 
    {   /* empty rset */
        *current = *total = 0;
        return;
    }
    r = 1.0*(lcur+rcur)/(ltot+rtot); /* weighed average of l and r */
    *current = (double) (p->hits);
    *total = *current/r ; 
#if RSET_DEBUG
    yaz_log(YLOG_DEBUG,"bool_pos: (%s/%s) %0.1f/%0.1f= %0.4f ",
                    info->rset_l->control->desc, info->rset_r->control->desc,
                    *current, *total, r);
#endif
}

static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm)
{
    struct rset_private *info = (struct rset_private *) ct->priv;
    rset_getterms(info->rset_l, terms, maxterms, curterm);
    rset_getterms(info->rset_r, terms, maxterms, curterm);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

