/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
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
static int r_read(RSFD rfd, void *buf, TERMID *term);
static void r_pos(RSFD rfd, double *current, double *total);
static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm);

static const struct rset_control control =
{
    "prox",
    r_delete,
    r_get_terms,
    r_open,
    r_close,
    r_forward,
    r_pos,
    r_read,
    rset_no_write,
};

struct rset_prox_info {
    int ordered;
    int exclusion;
    int relation;
    int distance;
};

struct rset_prox_rfd {
    RSFD *rfd;
    char **buf;  /* lookahead key buffers */
    char *more;  /* more in each lookahead? */
    TERMID *terms; /* lookahead terms */
    zint hits;
};


RSET rset_create_prox(NMEM nmem, struct rset_key_control *kcontrol,
                      int scope,
                      int rset_no, RSET *rset,
                      int ordered, int exclusion,
                      int relation, int distance)
{
    RSET rnew = rset_create_base(&control, nmem, kcontrol, scope, 0,
                                 rset_no, rset);
    struct rset_prox_info *info;
    info = (struct rset_prox_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->ordered = ordered;
    info->exclusion = exclusion;
    info->relation = relation;
    info->distance = distance;
    rnew->priv = info;
    return rnew;
}

static void r_delete(RSET ct)
{
}

static RSFD r_open(RSET ct, int flag)
{
    RSFD rfd;
    struct rset_prox_rfd *p;
    int i;

    if (flag & RSETF_WRITE)
    {
        yaz_log(YLOG_FATAL, "prox set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        p = (struct rset_prox_rfd *)(rfd->priv);
    else
    {
        p = (struct rset_prox_rfd *) nmem_malloc(ct->nmem,sizeof(*p));
        rfd->priv = p;
        p->more = nmem_malloc(ct->nmem,sizeof(*p->more) * ct->no_children);
        p->buf = nmem_malloc(ct->nmem,sizeof(*p->buf) * ct->no_children);
        p->terms = nmem_malloc(ct->nmem,sizeof(*p->terms) * ct->no_children);
        for (i = 0; i < ct->no_children; i++)
        {
            p->buf[i] = nmem_malloc(ct->nmem,ct->keycontrol->key_size);
            p->terms[i] = 0;
        }
        p->rfd = nmem_malloc(ct->nmem,sizeof(*p->rfd) * ct->no_children);
    }
    yaz_log(YLOG_DEBUG,"rsprox (%s) open [%p] n=%d",
            ct->control->desc, rfd, ct->no_children);

    for (i = 0; i < ct->no_children; i++)
    {
        p->rfd[i] = rset_open(ct->children[i], RSETF_READ);
        p->more[i] = rset_read(p->rfd[i], p->buf[i], &p->terms[i]);
    }
    p->hits = 0;
    return rfd;
}

static void r_close(RSFD rfd)
{
    RSET ct = rfd->rset;
    struct rset_prox_rfd *p = (struct rset_prox_rfd *)(rfd->priv);

    int i;
    for (i = 0; i < ct->no_children; i++)
        rset_close(p->rfd[i]);
}

static int r_forward(RSFD rfd, void *buf, TERMID *term, const void *untilbuf)
{
    RSET ct = rfd->rset;
    struct rset_prox_info *info = (struct rset_prox_info *)(ct->priv);
    struct rset_prox_rfd *p = (struct rset_prox_rfd *)(rfd->priv);
    const struct rset_key_control *kctrl = ct->keycontrol;
    int cmp = 0;
    int i;

    if (untilbuf)
    {
        /* it is enough to forward first one. Other will follow. */
        if (p->more[0] &&   /* was: cmp >=2 */
            ((kctrl->cmp)(untilbuf, p->buf[0]) >= rfd->rset->scope) )
            p->more[0] = rset_forward(p->rfd[0], p->buf[0],
                                      &p->terms[0], untilbuf);
    }
    if (info->ordered && info->relation <= 3 && info->exclusion == 0)
    {
        while (p->more[0])
        {
            for (i = 1; i < ct->no_children; i++)
            {
                if (!p->more[i])
                {
                    p->more[0] = 0; /* saves us a goto out of while loop. */
                    break;
                }
                cmp = (*kctrl->cmp)(p->buf[i], p->buf[i-1]);
                if (cmp >= rfd->rset->scope)  /* not same record */
                {
                    p->more[i-1] = rset_forward(p->rfd[i-1],
                                                p->buf[i-1],
                                                &p->terms[i-1],
                                                p->buf[i]);
                    break;
                }
                else if (cmp > 0) /* within record and ordered */
                {
                    zint diff = (*kctrl->getseq)(p->buf[i]) -
                        (*kctrl->getseq)(p->buf[i-1]);
                    if (info->relation == 3 && diff == info->distance)
                        continue;
                    else if (info->relation == 2 && diff <= info->distance)
                        continue;
                    else if (info->relation == 1 && diff < info->distance)
                        continue;

                    p->more[i-1] = rset_read(p->rfd[i-1], p->buf[i-1],
                                             &p->terms[i-1]);
                    break;
                }
                else  /* within record - wrong order */
                {
                    p->more[i] = rset_forward(p->rfd[i], p->buf[i],
                                              &p->terms[i], p->buf[i-1]);
                    break;
                }
            }
            if (i == ct->no_children)
            {
                i = ct->no_children-1;
                memcpy(buf, p->buf[i], kctrl->key_size);
                if (term)
                    *term = p->terms[i];
                p->more[i] = rset_read(p->rfd[i], p->buf[i], &p->terms[i]);
                p->hits++;
                return 1;
            }
        }
    }
    else if (ct->no_children == 2)
    {
        while (p->more[0] && p->more[1])
        {
            int cmp = (*kctrl->cmp)(p->buf[0], p->buf[1]);
            if ( cmp <= - rfd->rset->scope) /* cmp<-1*/
                p->more[0] = rset_forward(p->rfd[0], p->buf[0],
                                          &p->terms[0],p->buf[1]);
            else if ( cmp >= rfd->rset->scope ) /* cmp>1 */
                p->more[1] = rset_forward(p->rfd[1], p->buf[1],
                                          &p->terms[1],p->buf[0]);
            else
            {
                zint seqno[500]; /* FIXME - why 500 ?? */
                int n = 0;

                seqno[n++] = (*kctrl->getseq)(p->buf[0]);
                while ((p->more[0] = rset_read(p->rfd[0],
                                               p->buf[0], &p->terms[0])))
                {
                    cmp = (*kctrl->cmp)(p->buf[0], p->buf[1]);
                    if (cmp <= - rfd->rset->scope || cmp >= rfd->rset->scope)
                        break;
                    if (n < 500)
                        seqno[n++] = (*kctrl->getseq)(p->buf[0]);
                }
                /* set up return buffer.. (save buf[1]) */
                memcpy(buf, p->buf[1], kctrl->key_size);
                if (term)
                    *term = p->terms[1];
                while (1)
                {
                    for (i = 0; i < n; i++)
                    {
                        zint diff = (*kctrl->getseq)(p->buf[1]) - seqno[i];
                        int excl = info->exclusion;
                        if (!info->ordered && diff < 0)
                            diff = -diff;
                        switch (info->relation)
                        {
                        case 1:      /* < */
                            if (diff < info->distance && diff >= 0)
                                excl = !excl;
                            break;
                        case 2:      /* <= */
                            if (diff <= info->distance && diff >= 0)
                                excl = !excl;
                            break;
                        case 3:      /* == */
                            if (diff == info->distance && diff >= 0)
                                excl = !excl;
                            break;
                        case 4:      /* >= */
                            if (diff >= info->distance && diff >= 0)
                                excl = !excl;
                            break;
                        case 5:      /* > */
                            if (diff > info->distance && diff >= 0)
                                excl = !excl;
                            break;
                        case 6:      /* != */
                            if (diff != info->distance && diff >= 0)
                                excl = !excl;
                            break;
                        }
                        if (excl)
                        {
                            p->more[1] = rset_read( p->rfd[1], p->buf[1],
                                                    &p->terms[1]);
                            p->hits++;
                            return 1;
                        }
                    }
                    p->more[1] = rset_read(p->rfd[1], p->buf[1], &p->terms[1]);
                    if (!p->more[1])
                        break;
                    cmp = (*kctrl->cmp)(buf, p->buf[1]);
                    if (cmp <= - rfd->rset->scope || cmp >= rfd->rset->scope)
                        break;
                }
            }
        }
    }
    return 0;
}


static int r_read(RSFD rfd, void *buf, TERMID *term)
{
    return r_forward(rfd, buf, term, 0);
}

static void r_pos(RSFD rfd, double *current, double *total)
{
    RSET ct = rfd->rset;
    struct rset_prox_rfd *p = (struct rset_prox_rfd *)(rfd->priv);
    int i;
    double ratio = 0.0;

    for (i = 0; i < ct->no_children; i++)
    {
        double cur, tot;
        rset_pos(p->rfd[i], &cur, &tot);
        if (tot > 0.0)
        {
            double nratio = cur / tot;
            if (ratio < nratio)
                ratio = nratio;
        }
    }
    *current = (double) p->hits;
    if (ratio > 0.0)
        *total = *current/ratio;
    else
        *total = 0.0;

    yaz_log(YLOG_DEBUG, "prox_pos: [%d] %0.1f/%0.1f= %0.4f ",
            i, *current, *total, ratio);
}

static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm)
{
    int i;
    for (i = 0; i < ct->no_children; i++)
        rset_getterms(ct->children[i], terms, maxterms, curterm);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

