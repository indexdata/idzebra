/* $Id: rsprox.c,v 1.12 2004-08-26 11:11:59 heikki Exp $
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zebrautl.h>
#include <rsprox.h>

#ifndef RSET_DEBUG
#define RSET_DEBUG 0
#endif

static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_forward(RSET ct, RSFD rfd, void *buf, 
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);
static void r_pos (RSFD rfd, double *current, double *total);

static const struct rset_control control = 
{
    "prox",
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_forward,
    r_pos,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_prox = &control;

struct rset_prox_info {
/*    struct rset_prox_parms p; */
    RSET *rset;  
    int rset_no;
    int ordered;
    int exclusion;
    int relation;
    int distance;
    int key_size;
    int (*cmp)(const void *p1, const void *p2);
    int (*getseq)(const void *p);
    struct rset_prox_rfd *rfd_list;
    struct rset_prox_rfd *free_list;
};

struct rset_prox_rfd {
    RSFD *rfd;
    char **buf;  /* lookahead key buffers */
    char *more;  /* more in each lookahead? */
    struct rset_prox_rfd *next;
    struct rset_prox_info *info;
    zint hits;
};    


RSET rsprox_create( NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            int (*getseq)(const void *p),
            int rset_no, RSET *rset,
            int ordered, int exclusion,
            int relation, int distance)
{
    RSET rnew=rset_create_base(&control, nmem);
    struct rset_prox_info *info;
    info = (struct rset_prox_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->key_size = key_size;
    info->cmp = cmp;
    info->getseq=getseq; /* FIXME - what about multi-level stuff ?? */
    info->rset = nmem_malloc(rnew->nmem,rset_no * sizeof(*info->rset));
    memcpy(info->rset, rset,
           rset_no * sizeof(*info->rset));
    info->rset_no=rset_no;
    info->ordered=ordered;
    info->exclusion=exclusion;
    info->relation=relation;
    info->distance=distance;
    info->rfd_list = NULL;
    info->free_list = NULL;
    rnew->priv=info;
    return rnew;
}


static void r_delete (RSET ct)
{
    struct rset_prox_info *info = (struct rset_prox_info *) ct->priv;
    int i;

    assert (info->rfd_list == NULL);
    for (i = 0; i<info->rset_no; i++)
        rset_delete (info->rset[i]);
}


static RSFD r_open (RSET ct, int flag)
{
    struct rset_prox_info *info = (struct rset_prox_info *) ct->priv;
    struct rset_prox_rfd *rfd;
    int i;

    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "prox set type is read-only");
        return NULL;
    }
    rfd = info->free_list;
    if (rfd)
        info->free_list=rfd->next;
    else {
        rfd = (struct rset_prox_rfd *) xmalloc (sizeof(*rfd));
        rfd->more = nmem_malloc (ct->nmem,sizeof(*rfd->more) * info->rset_no);
        rfd->buf = nmem_malloc(ct->nmem,sizeof(*rfd->buf) * info->rset_no);
        for (i = 0; i < info->rset_no; i++)
            rfd->buf[i] = nmem_malloc(ct->nmem,info->key_size);
        rfd->rfd = nmem_malloc(ct->nmem,sizeof(*rfd->rfd) * info->rset_no);
    }
    logf(LOG_DEBUG,"rsprox (%s) open [%p]", ct->control->desc, rfd);
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;


    for (i = 0; i < info->rset_no; i++) {
        rfd->rfd[i] = rset_open (info->rset[i], RSETF_READ);
        rfd->more[i] = rset_read (info->rset[i], rfd->rfd[i],
                                  rfd->buf[i]);
    }
    rfd->hits=0;
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_prox_info *info = ((struct rset_prox_rfd*)rfd)->info;
    struct rset_prox_rfd **rfdp;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            int i;
            struct rset_prox_rfd *rfd_tmp=*rfdp;
            for (i = 0; i<info->rset_no; i++)
                rset_close (info->rset[i], (*rfdp)->rfd[i]);
            *rfdp = (*rfdp)->next;
            rfd_tmp->next=info->free_list;
            info->free_list=rfd_tmp;
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_rewind (RSFD rfd)
{
    struct rset_prox_info *info = ((struct rset_prox_rfd*)rfd)->info;
    struct rset_prox_rfd *p = (struct rset_prox_rfd *) rfd;
    int i;

    logf (LOG_DEBUG, "rsprox_rewind");

    for (i = 0; i < info->rset_no; i++)
    {
        rset_rewind (info->rset[i], p->rfd[i]);
        p->more[i] = rset_read (info->rset[i], p->rfd[i], p->buf[i]);
    }
    p->hits=0;
}

static int r_forward (RSET ct, RSFD rfd, void *buf, 
                      int (*cmpfunc)(const void *p1, const void *p2),
                      const void *untilbuf)
{
    /* Note: CT is not used. We _can_ pass NULL for it */
    struct rset_prox_info *info = ((struct rset_prox_rfd*)rfd)->info;
    struct rset_prox_rfd *p = (struct rset_prox_rfd *) rfd;
    int cmp=0;
    int i;

    if (untilbuf)
    {
        /* it's enough to forward first one. Other will follow
           automatically */
        if ( p->more[0] && ((cmpfunc)(untilbuf, p->buf[0]) >= 2) )
            p->more[0] = rset_forward(info->rset[0], p->rfd[0],
                                      p->buf[0], info->cmp,
                                      untilbuf);
    }
    if (info->ordered && info->relation == 3 && info->exclusion == 0
        && info->distance == 1)
    {
        while (p->more[0]) 
        {
            for (i = 1; i < info->rset_no; i++)
            {
                if (!p->more[i]) 
                {
                    p->more[0] = 0;    /* saves us a goto out of while loop. */
                    break;
                }
                cmp = (*info->cmp) (p->buf[i], p->buf[i-1]);
                if (cmp > 1)
                {
                    p->more[i-1] = rset_forward (info->rset[i-1],
                                                 p->rfd[i-1],
                                                 p->buf[i-1],
                                                 info->cmp,
                                                 p->buf[i]);
                    break;
                }
                else if (cmp == 1)
                {
                    if ((*info->getseq)(p->buf[i-1]) +1 != 
                        (*info->getseq)(p->buf[i]))
                    {
                        p->more[i-1] = rset_read ( info->rset[i-1], 
                                             p->rfd[i-1], p->buf[i-1]);
                        break;
                    }
                }
                else
                {
                    p->more[i] = rset_forward (info->rset[i], p->rfd[i],
                                               p->buf[i], info->cmp,
                                               p->buf[i-1]);
                    break;
                }
            }
            if (i == p->info->rset_no)
            {
                memcpy (buf, p->buf[0], info->key_size);
                p->more[0] = rset_read (info->rset[0], p->rfd[0], p->buf[0]);
                p->hits++;
                return 1;
            }
        }
    }
    else if (info->rset_no == 2)
    {
        while (p->more[0] && p->more[1]) 
        {
            int cmp = (*info->cmp)(p->buf[0], p->buf[1]);
            if (cmp < -1)
                p->more[0] = rset_forward (info->rset[0], p->rfd[0],
                                           p->buf[0], info->cmp, p->buf[0]);
            else if (cmp > 1)
                p->more[1] = rset_forward (info->rset[1], p->rfd[1],
                                           p->buf[1], info->cmp, p->buf[1]);
            else
            {
                int seqno[500];
                int n = 0;
                
                seqno[n++] = (*info->getseq)(p->buf[0]);
                while ((p->more[0] = rset_read (info->rset[0], p->rfd[0],
                                                p->buf[0])) >= -1 &&
                       p->more[0] <= -1)
                    if (n < 500)
                        seqno[n++] = (*info->getseq)(p->buf[0]);
                
                for (i = 0; i<n; i++)
                {
                    int diff = (*info->getseq)(p->buf[1]) - seqno[i];
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
                        memcpy (buf, p->buf[1], info->key_size);
                        
                        p->more[1] = rset_read (info->rset[1],
                                                p->rfd[1], p->buf[1]);
                        p->hits++;
                        return 1;
                    }
                }
                p->more[1] = rset_read (info->rset[1], p->rfd[1],
                                        p->buf[1]);
            }
        }
    }
    return 0;
}


static int r_read (RSFD rfd, void *buf)
{
    { double cur,tot; r_pos(rfd,&cur,&tot); } /*!*/
    return r_forward(0, rfd, buf, 0, 0);
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "prox set type is read-only");
    return -1;
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    struct rset_prox_info *info = ((struct rset_prox_rfd*)rfd)->info;
    struct rset_prox_rfd *p = (struct rset_prox_rfd *) rfd;
    int i;
    double cur,tot=-1;
    double scur=0,stot=0;
    double r;

    logf (LOG_DEBUG, "rsprox_pos");

    for (i = 0; i < info->rset_no; i++)
    {
        rset_pos(info->rset[i], p->rfd[i],  &cur, &tot);
        if (tot>0) {
            scur += cur;
            stot += tot;
        }
    }
    if (tot <0) {  /* nothing found */
        *current=-1;
        *total=-1;
    } else if (tot <1) { /* most likely tot==0 */
        *current=0;
        *total=0;
    } else {
        r=scur/stot; 
        *current=p->hits;
        *total=*current/r ; 
    }
    logf(LOG_DEBUG,"prox_pos: [%d] %0.1f/%0.1f= %0.4f ",
                    i,*current, *total, r);
}
