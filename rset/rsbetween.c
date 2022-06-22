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


/* rsbetween is (mostly) used for xml searches. It returns the hits of the
 * "middle" rset, that are in between the "left" and "right" rsets. For
 * example "Shakespeare" in between "<author>" and </author>. The thing is
 * complicated by the inclusion of attributes (from their own rset). If attrs
 * specified, they must match the "left" rset (start tag). "Hamlet" between
 * "<title lang = eng>" and "</title>". (This assumes that the attributes are
 * indexed to the same seqno as the tags).
 *
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

static RSFD r_open(RSET ct, int flag);
static void r_close(RSFD rfd);
static void r_delete(RSET ct);
static int r_forward(RSFD rfd, void *buf,
                    TERMID *term, const void *untilbuf);
static int r_read(RSFD rfd, void *buf, TERMID *term );
static void r_pos(RSFD rfd, double *current, double *total);
static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm);

static const struct rset_control control =
{
    "between",
    r_delete,
    r_get_terms,
    r_open,
    r_close,
    r_forward,
    r_pos,
    r_read,
    rset_no_write,
};

struct rset_between_info {
    TERMID startterm; /* pseudo terms for detecting which one we read from */
    TERMID stopterm;
    TERMID attrterm;
    TERMID *hit2_terms;
};

struct rset_between_rfd {
    RSFD andrfd;
    void *recbuf; /* a key that tells which record we are in */
    void *startbuf; /* the start tag */
    int startbufok; /* we have seen the first start tag */
    void *attrbuf;  /* the attr tag. If these two match, we have attr match */
    int attrbufok; /* we have seen the first attr tag, can compare */
    int depth; /* number of start-tags without end-tags */
    int attrdepth; /* on what depth the attr matched */
    int match_1;
    int match_2;
    zint hits;
};

static int log_level = 0;
static int log_level_initialized = 0;


/* make sure that the rset has a term attached. If not, create one */
/* we need these terms for the tags, to distinguish what we read */
static void checkterm(RSET rs, char *tag, NMEM nmem)
{
    if (!rs->term)
    {
        rs->term = rset_term_create(tag, -1, "", 0, nmem, 0, 0, 0, 0);
        rs->term->rset = rs;
    }
}


RSET rset_create_between(NMEM nmem, struct rset_key_control *kcontrol,
                         int scope, RSET rset_l, RSET rset_m1, RSET rset_m2,
                         RSET rset_r, RSET rset_attr)
{
    RSET rnew = rset_create_base(&control, nmem, kcontrol, scope, 0, 0, 0);
    struct rset_between_info *info =
        (struct rset_between_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    RSET rsetarray[5];
    int n = 0;

    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("rsbetween");
        log_level_initialized = 1;
    }
    rsetarray[n++] = rset_l;
    checkterm(rset_l, "", nmem);
    info->startterm = rset_l->term;

    rsetarray[n++] = rset_r;
    checkterm(rset_r, "", nmem);
    info->stopterm = rset_r->term;

    rsetarray[n++] = rset_m1;
    if (rset_m2)
    {
        rsetarray[n++] = rset_m2;
        /* hard to work do determine whether we get results from
           rset_m2 or rset_m1 */
        info->hit2_terms = (TERMID*)
            nmem_malloc(nmem, (2 + rset_m2->no_children) * sizeof(TERMID));
        int i;
        for (i = 0; i < rset_m2->no_children; i++) /* sub terms */
            info->hit2_terms[i] = rset_m2->children[i]->term;
        if (rset_m2->term) /* immediate term */
            info->hit2_terms[i++] = rset_m2->term;
        info->hit2_terms[i] = 0;
    }
    else
        info->hit2_terms = NULL;

    if (rset_attr)
    {
        rsetarray[n++] = rset_attr;
        checkterm(rset_attr, "(attr)", nmem);
        info->attrterm = rset_attr->term;
    }
    else
        info->attrterm = NULL;
    rnew->no_children = 1;
    rnew->children = nmem_malloc(rnew->nmem, sizeof(RSET *));
    rnew->children[0] = rset_create_and(nmem, kcontrol,
                                        scope, n, rsetarray);
    rnew->priv = info;
    yaz_log(log_level, "create rset at %p", rnew);
    return rnew;
}

static void r_delete(RSET ct)
{
}


static RSFD r_open(RSET ct, int flag)
{
    RSFD rfd;
    struct rset_between_rfd *p;

    if (flag & RSETF_WRITE)
    {
        yaz_log(YLOG_FATAL, "between set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        p = (struct rset_between_rfd *) rfd->priv;
    else {
        p = (struct rset_between_rfd *) nmem_malloc(ct->nmem, sizeof(*p));
        rfd->priv = p;
        p->recbuf = nmem_malloc(ct->nmem, ct->keycontrol->key_size);
        p->startbuf = nmem_malloc(ct->nmem, ct->keycontrol->key_size);
        p->attrbuf = nmem_malloc(ct->nmem, ct->keycontrol->key_size);
    }
    p->andrfd = rset_open(ct->children[0], RSETF_READ);
    p->hits = -1;
    p->depth = 0;
    p->attrdepth = 0;
    p->attrbufok = 0;
    p->startbufok = 0;
    yaz_log(log_level, "open rset=%p rfd=%p", ct, rfd);
    return rfd;
}

static void r_close(RSFD rfd)
{
    struct rset_between_rfd *p = (struct rset_between_rfd *) rfd->priv;
    yaz_log(log_level, "close rfd=%p", rfd);
    rset_close(p->andrfd);
}

static int r_forward(RSFD rfd, void *buf,
                     TERMID *term, const void *untilbuf)
{
    struct rset_between_rfd *p = (struct rset_between_rfd *) rfd->priv;
    int rc;
    yaz_log(log_level, "forwarding ");
    rc = rset_forward(p->andrfd,buf,term,untilbuf);
    return rc;
}

static void checkattr(RSFD rfd)
{
    struct rset_between_info *info = (struct rset_between_info *)
        rfd->rset->priv;
    struct rset_between_rfd *p = (struct rset_between_rfd *)rfd->priv;
    const struct rset_key_control *kctrl = rfd->rset->keycontrol;
    int cmp;
    if (p->attrdepth)
        return; /* already found one */
    if (!info->attrterm)
    {
        p->attrdepth = -1; /* matches always */
        return;
    }
    if ( p->startbufok && p->attrbufok )
    { /* have buffers to compare */
        cmp = (kctrl->cmp)(p->startbuf, p->attrbuf);
        if (0 == cmp) /* and the keys match */
        {
            p->attrdepth = p->depth;
            yaz_log(log_level, "found attribute match at depth %d",
                    p->attrdepth);
        }
    }
}

static int r_read(RSFD rfd, void *buf, TERMID *term)
{
    struct rset_between_info *info =
        (struct rset_between_info *)rfd->rset->priv;
    struct rset_between_rfd *p = (struct rset_between_rfd *)rfd->priv;
    const struct rset_key_control *kctrl = rfd->rset->keycontrol;
    int cmp;
    TERMID dummyterm = 0;
    yaz_log(log_level, "== read: term=%p",term);
    if (!term)
        term = &dummyterm;
    while (rset_read(p->andrfd, buf, term))
    {
        yaz_log(log_level, "read loop term=%p d=%d ad=%d",
                *term, p->depth, p->attrdepth);
        if (p->hits < 0)
        {/* first time? */
            memcpy(p->recbuf, buf, kctrl->key_size);
            p->hits = 0;
            cmp = rfd->rset->scope; /* force newrecord */
        }
        else {
            cmp = (kctrl->cmp)(buf, p->recbuf);
            yaz_log(log_level, "cmp=%d", cmp);
        }

        if (cmp >= rfd->rset->scope)
        {
            yaz_log(log_level, "new record");
            p->depth = 0;
            p->attrdepth = 0;
            p->match_1 = p->match_2 = 0;
            memcpy(p->recbuf, buf, kctrl->key_size);
        }

        if (*term)
            yaz_log(log_level, "  term: '%s'", (*term)->name);
        if (*term == info->startterm)
        {
            p->depth++;
            yaz_log(log_level, "read start tag. d=%d", p->depth);
            memcpy(p->startbuf, buf, kctrl->key_size);
            p->startbufok = 1;
            checkattr(rfd); /* in case we already saw the attr here */
        }
        else if (*term == info->stopterm)
        {
            if (p->depth == p->attrdepth)
                p->attrdepth = 0; /* ending the tag with attr match */
            p->depth--;
            if (p->depth == 0)
                p->match_1 = p->match_2 = 0;
            yaz_log(log_level, "read end tag. d=%d ad=%d", p->depth,
                    p->attrdepth);
        }
        else if (*term == info->attrterm)
        {
            yaz_log(log_level, "read attr");
            memcpy(p->attrbuf, buf, kctrl->key_size);
            p->attrbufok = 1;
            checkattr(rfd); /* in case the start tag came first */
        }
        else
        { /* mut be a real hit */
            if (p->depth && p->attrdepth)
            {
                if (!info->hit2_terms)
                    p->match_1 = p->match_2 = 1;
                else
                {
                    int i;
                    for (i = 0; info->hit2_terms[i]; i++)
                        if (info->hit2_terms[i] == *term)
                            break;
                    if (info->hit2_terms[i])
                        p->match_2 = 1;
                    else
                        p->match_1 = 1;
                }
                if (p->match_1 && p->match_2)
                {
                    p->hits++;
                    yaz_log(log_level, "got a hit h="ZINT_FORMAT" d=%d ad=%d",
                            p->hits, p->depth, p->attrdepth);
                    return 1; /* we have everything in place already! */
                }
            } else
                yaz_log(log_level, "Ignoring hit. h="ZINT_FORMAT" d=%d ad=%d",
                        p->hits, p->depth, p->attrdepth);
        }
    } /* while read */
    return 0;
}  /* r_read */

static void r_pos(RSFD rfd, double *current, double *total)
{
    struct rset_between_rfd *p = (struct rset_between_rfd *) rfd->priv;
    rset_pos(p->andrfd, current, total);
    yaz_log(log_level, "pos: %0.1f/%0.1f ", *current, *total);
}

static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm)
{
    rset_getterms(ct->children[0], terms, maxterms, curterm);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

