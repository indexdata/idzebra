/* $Id: rsbool.c,v 1.45 2004-08-31 10:43:39 heikki Exp $
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
#include <rset.h>
#include <rsbool.h>

#ifndef RSET_DEBUG
#define RSET_DEBUG 0
#endif

static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_forward(RSFD rfd, void *buf,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);
static void r_pos (RSFD rfd, double *current, double *total); 
static int r_read_and (RSFD rfd, void *buf);
static int r_read_or (RSFD rfd, void *buf);
static int r_read_not (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control_and = 
{
    "and",
    r_delete,
    r_open,
    r_close,
    r_rewind,
    r_forward, 
    r_pos,    
    r_read_and,
    r_write,
};

static const struct rset_control control_or = 
{
    "or",
    r_delete,
    r_open,
    r_close,
    r_rewind,
    r_forward, 
    r_pos,
    r_read_or,
    r_write,
};

static const struct rset_control control_not = 
{
    "not",
    r_delete,
    r_open,
    r_close,
    r_rewind,
    r_forward, 
    r_pos,
    r_read_not,
    r_write,
};


const struct rset_control *rset_kind_and = &control_and;
const struct rset_control *rset_kind_or = &control_or;
const struct rset_control *rset_kind_not = &control_not;

struct rset_bool_info {
    int key_size;
    RSET rset_l;
    RSET rset_r;
    int (*cmp)(const void *p1, const void *p2);
    void (*log_item)(int logmask, const void *p, const char *txt);
};

struct rset_bool_rfd {
    zint hits;
    RSFD rfd_l;
    RSFD rfd_r;
    int  more_l;
    int  more_r;
    void *buf_l;
    void *buf_r;
    int tail;
    struct rset_bool_rfd *next;
    struct rset_bool_info *info;
};    

static RSET rsbool_create_base( const struct rset_control *ctrl,
            NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            RSET rset_l, RSET rset_r, 
            void (*log_item)(int logmask, const void *p, const char *txt) )
{
    RSET rnew=rset_create_base(ctrl, nmem);
    struct rset_bool_info *info;
    info = (struct rset_bool_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->key_size = key_size;
    info->rset_l = rset_l;
    info->rset_r = rset_r;
    info->cmp = cmp;
    info->log_item = log_item;
    
    rnew->priv=info;
    return rnew;
}


RSET rsbool_create_and( NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            RSET rset_l, RSET rset_r, 
            void (*log_item)(int logmask, const void *p, const char *txt) )
{
    return rsbool_create_base(rset_kind_and, nmem, key_size, cmp,
                              rset_l, rset_r, log_item);
}

RSET rsbool_create_or( NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            RSET rset_l, RSET rset_r, 
            void (*log_item)(int logmask, const void *p, const char *txt) )
{
    return rsbool_create_base(rset_kind_or, nmem, key_size, cmp,
                              rset_l, rset_r, log_item);
}

RSET rsbool_create_not( NMEM nmem, int key_size, 
            int (*cmp)(const void *p1, const void *p2),
            RSET rset_l, RSET rset_r, 
            void (*log_item)(int logmask, const void *p, const char *txt) )
{
    return rsbool_create_base(rset_kind_not, nmem, key_size, cmp,
                              rset_l, rset_r, log_item);
}

static void r_delete (RSET ct)
{
    struct rset_bool_info *info = (struct rset_bool_info *) ct->priv;
    rset_delete (info->rset_l);
    rset_delete (info->rset_r);
}


static RSFD r_open (RSET ct, int flag)
{
    struct rset_bool_info *info = (struct rset_bool_info *) ct->priv;
    RSFD rfd;
    struct rset_bool_rfd *p;
   

    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "bool set type is read-only");
        return NULL;
    }
    rfd = rfd_create_base(ct);
    if (rfd->priv)
        p=(struct rset_bool_rfd *)rfd->priv;
    else {
        p=nmem_malloc(ct->nmem,sizeof(*p));
        rfd->priv=p;
        p->buf_l = nmem_malloc(ct->nmem, info->key_size);
        p->buf_r = nmem_malloc(ct->nmem, info->key_size);
    }

    logf(LOG_DEBUG,"rsbool (%s) open [%p]", ct->control->desc, rfd);
    p->hits=0;

    p->rfd_l = rset_open (info->rset_l, RSETF_READ);
    p->rfd_r = rset_open (info->rset_r, RSETF_READ);
    p->more_l = rset_read (p->rfd_l, p->buf_l);
    p->more_r = rset_read (p->rfd_r, p->buf_r);
    p->tail = 0;
    return rfd;
}

static void r_close (RSFD rfd)
{
 /* struct rset_bool_info *info = (struct rset_bool_info*)(rfd->rset->priv); */
    struct rset_bool_rfd *prfd=(struct rset_bool_rfd *)rfd->priv;

    rset_close (prfd->rfd_l);
    rset_close (prfd->rfd_r);
    rfd_delete_base(rfd);
}


static void r_rewind (RSFD rfd)
{
 /* struct rset_bool_info *info = (struct rset_bool_info*)(rfd->rset->priv); */
    struct rset_bool_rfd *p=(struct rset_bool_rfd *)rfd->priv;

    logf (LOG_DEBUG, "rsbool_rewind");
    rset_rewind (p->rfd_l);
    rset_rewind (p->rfd_r);
    p->more_l = rset_read (p->rfd_l, p->buf_l);
    p->more_r = rset_read (p->rfd_r, p->buf_r);
    p->hits=0;
}

static int r_forward (RSFD rfd, void *buf,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf)
{
    struct rset_bool_info *info = (struct rset_bool_info*)(rfd->rset->priv);
    struct rset_bool_rfd *p=(struct rset_bool_rfd *)rfd->priv;
    int rc;

    if ( p->more_l && ((cmpfunc)(untilbuf,p->buf_l)==2) )
        p->more_l = rset_forward(p->rfd_l, p->buf_l,
                        info->cmp, untilbuf);
    if ( p->more_r && ((cmpfunc)(untilbuf,p->buf_r)==2))
        p->more_r = rset_forward(p->rfd_r, p->buf_r,
                        info->cmp, untilbuf);
    p->tail=0; 
    rc = rset_read(rfd,buf); 
    return rc;
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

static int r_read_and (RSFD rfd, void *buf)
{
    struct rset_bool_info *info = (struct rset_bool_info*)(rfd->rset->priv);
    struct rset_bool_rfd *p=(struct rset_bool_rfd *)rfd->priv;

    while (p->more_l || p->more_r)
    {
        int cmp;

        if (p->more_l && p->more_r)
            cmp = (*info->cmp)(p->buf_l, p->buf_r);
        else if (p->more_l)
            cmp = -2;
        else
            cmp = 2;
#if RSET_DEBUG
        logf (LOG_DEBUG, "r_read_and [%p] looping: m=%d/%d c=%d t=%d",
                        rfd, p->more_l, p->more_r, cmp, p->tail);
        (*info->log_item)(LOG_DEBUG, p->buf_l, "left ");
        (*info->log_item)(LOG_DEBUG, p->buf_r, "right ");
#endif
        if (!cmp)
        {
            memcpy (buf, p->buf_l, info->key_size);
            p->more_l = rset_read (p->rfd_l, p->buf_l);
            p->tail = 1;
        }
        else if (cmp == 1)
        {
            memcpy (buf, p->buf_r, info->key_size);
            p->more_r = rset_read (p->rfd_r, p->buf_r);
            p->tail = 1;
#if RSET_DEBUG
            logf (LOG_DEBUG, "r_read_and [%p] returning R m=%d/%d c=%d",
                    rfd, p->more_l, p->more_r, cmp);
            key_logdump(LOG_DEBUG,buf);
            (*info->log_item)(LOG_DEBUG, buf, "");
#endif
            p->hits++;
            return 1;
        }
        else if (cmp == -1)
        {
            memcpy (buf, p->buf_l, info->key_size);
            p->more_l = rset_read (p->rfd_l, p->buf_l);
            p->tail = 1;
#if RSET_DEBUG
            logf (LOG_DEBUG, "r_read_and [%p] returning L m=%d/%d c=%d",
                    rfd, p->more_l, p->more_r, cmp);
            (*info->log_item)(LOG_DEBUG, buf, "");
#endif
            p->hits++;
            return 1;
        }
        else if (cmp > 1)  /* cmp == 2 */
        {
#define OLDCODE 0
#if OLDCODE
            memcpy (buf, p->buf_r, info->key_size);
            
            p->more_r = rset_read (p->rfd_r, p->buf_r);
            if (p->tail)
            {
                if (!p->more_r || (*info->cmp)(p->buf_r, buf) > 1)
                    p->tail = 0;
#if RSET_DEBUG
                logf (LOG_DEBUG, "r_read_and returning C m=%d/%d c=%d",
                        p->more_l, p->more_r, cmp);
                (*info->log_item)(LOG_DEBUG, buf, "");
#endif
                p->hits++;
                return 1;
            }
#else
            
            if (p->tail)
            {
                memcpy (buf, p->buf_r, info->key_size);
                p->more_r = rset_read (p->rfd_r, p->buf_r);
                if (!p->more_r || (*info->cmp)(p->buf_r, buf) > 1)
                    p->tail = 0;
#if RSET_DEBUG
                logf (LOG_DEBUG, "r_read_and [%p] returning R tail m=%d/%d c=%d",
                        rfd, p->more_l, p->more_r, cmp);
                (*info->log_item)(LOG_DEBUG, buf, "");
#endif
                p->hits++;
                return 1;
            }
            else
            {
#if RSET_DEBUG
                logf (LOG_DEBUG, "r_read_and [%p] about to forward R m=%d/%d c=%d",
                        rfd, p->more_l, p->more_r, cmp);
#endif
                if (p->more_r && p->more_l)
                    p->more_r = rset_forward( p->rfd_r, 
                                    p->buf_r, (info->cmp), p->buf_l);
                else 
                    return 0; /* no point in reading further */
            }
#endif
        }
        else  /* cmp == -2 */
        {
#if OLDCODE
             memcpy (buf, p->buf_l, info->key_size);
             p->more_l = rset_read (p->rfd_l, p->buf_l);
             if (p->tail)
             {
                 if (!p->more_l || (*info->cmp)(p->buf_l, buf) > 1)
                     p->tail = 0;
#if RSET_DEBUG
                 logf (LOG_DEBUG, "r_read_and [%p] returning R tail m=%d/%d c=%d",
                        rfd, p->more_l, p->more_r, cmp);
                 (*info->log_item)(LOG_DEBUG, buf, "");
#endif
                 p->hits++;
                 return 1;
             }
#else
            if (p->tail)
            {
                memcpy (buf, p->buf_l, info->key_size);
                p->more_l = rset_read (p->rfd_l, p->buf_l);
                if (!p->more_l || (*info->cmp)(p->buf_l, buf) > 1)
                    p->tail = 0;
#if RSET_DEBUG
                logf (LOG_DEBUG, "r_read_and [%p] returning L tail m=%d/%d c=%d",
                        rfd, p->more_l, p->more_r, cmp);
                (*info->log_item)(LOG_DEBUG, buf, "");
#endif
                p->hits++;
                return 1;
            }
            else
            {
#if RSET_DEBUG
                logf (LOG_DEBUG, "r_read_and [%p] about to forward L m=%d/%d c=%d",
                        rfd, p->more_l, p->more_r, cmp);
#endif
                if (p->more_r && p->more_l)
                    p->more_l = rset_forward(p->rfd_l, 
                                    p->buf_l, (info->cmp), p->buf_r);
                else 
                    return 0; /* no point in reading further */
            }
#endif
        }
    }
#if RSET_DEBUG
    logf (LOG_DEBUG, "r_read_and [%p] reached its end",rfd);
#endif
    return 0;
}

static int r_read_or (RSFD rfd, void *buf)
{
    struct rset_bool_info *info = (struct rset_bool_info*)(rfd->rset->priv);
    struct rset_bool_rfd *p=(struct rset_bool_rfd *)rfd->priv;

    while (p->more_l || p->more_r)
    {
        int cmp;

        if (p->more_l && p->more_r)
            cmp = (*info->cmp)(p->buf_l, p->buf_r);
        else if (p->more_r)
            cmp = 2;
        else
            cmp = -2;
        if (!cmp)
        {
            memcpy (buf, p->buf_l, info->key_size);
            p->more_l = rset_read (p->rfd_l, p->buf_l);
            p->more_r = rset_read (p->rfd_r, p->buf_r);
#if RSET_DEBUG
            logf (LOG_DEBUG, "r_read_or returning A m=%d/%d c=%d",
                    p->more_l, p->more_r, cmp);
            (*info->log_item)(LOG_DEBUG, buf, "");
#endif
            p->hits++;
            return 1;
        }
        else if (cmp > 0)
        {
            memcpy (buf, p->buf_r, info->key_size);
            p->more_r = rset_read (p->rfd_r, p->buf_r);
#if RSET_DEBUG
            logf (LOG_DEBUG, "r_read_or returning B m=%d/%d c=%d",
                    p->more_l, p->more_r, cmp);
            (*info->log_item)(LOG_DEBUG, buf, "");
#endif
            p->hits++;
            return 1;
        }
        else
        {
            memcpy (buf, p->buf_l, info->key_size);
            p->more_l = rset_read ( p->rfd_l, p->buf_l);
#if RSET_DEBUG
            logf (LOG_DEBUG, "r_read_or returning C m=%d/%d c=%d",
                    p->more_l, p->more_r, cmp);
            (*info->log_item)(LOG_DEBUG, buf, "");
#endif
            p->hits++;
            return 1;
        }
    }
    return 0;
}

static int r_read_not (RSFD rfd, void *buf)
{
    struct rset_bool_info *info = (struct rset_bool_info*)(rfd->rset->priv);
    struct rset_bool_rfd *p=(struct rset_bool_rfd *)rfd->priv;

    while (p->more_l || p->more_r)
    {
        int cmp;

        if (p->more_l && p->more_r)
            cmp = (*info->cmp)(p->buf_l, p->buf_r);
        else if (p->more_r)
            cmp = 2;
        else
            cmp = -2;
        if (cmp < -1)
        {
            memcpy (buf, p->buf_l, info->key_size);
            p->more_l = rset_read (p->rfd_l, p->buf_l);
            p->hits++;
            return 1;
        }
        else if (cmp > 1)
        {
                p->more_r = rset_forward( p->rfd_r, 
                    p->buf_r, (info->cmp), p->buf_l);
        }
        else
        {
            memcpy (buf, p->buf_l, info->key_size);
            do
            { 
                p->more_l = rset_read (p->rfd_l, p->buf_l);
                if (!p->more_l)
                    break;
                cmp = (*info->cmp)(p->buf_l, buf);
            } while (cmp >= -1 && cmp <= 1);
            do
            {
                p->more_r = rset_read (p->rfd_r, p->buf_r);
                if (!p->more_r)
                    break;
                cmp = (*info->cmp)(p->buf_r, buf);
            } while (cmp >= -1 && cmp <= 1);
        }
    }
    return 0;
}


static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "bool set type is read-only");
    return -1;
}

static void r_pos (RSFD rfd, double *current, double *total)
{
    struct rset_bool_rfd *p=(struct rset_bool_rfd *)rfd->priv;
    double lcur,ltot;
    double rcur,rtot;
    double r;
    ltot=-1; rtot=-1;
    rset_pos(p->rfd_l,  &lcur, &ltot);
    rset_pos(p->rfd_r,  &rcur, &rtot);
    if ( (rtot<0) && (ltot<0)) { /*no position */
        *current=rcur;  /* return same as you got */
        *total=rtot;    /* probably -1 for not available */
    }
    if ( rtot<0) { rtot=0; rcur=0;} /* if only one useful, use it */
    if ( ltot<0) { ltot=0; lcur=0;}
    if ( rtot+ltot < 1 ) { /* empty rset */
        *current=0;
        *total=0;
        return;
    }
    r=1.0*(lcur+rcur)/(ltot+rtot); /* weighed average of l and r */
    *current=(double) (p->hits);
    *total=*current/r ; 
#if RSET_DEBUG
    yaz_log(LOG_DEBUG,"bool_pos: (%s/%s) %0.1f/%0.1f= %0.4f ",
                    info->rset_l->control->desc, info->rset_r->control->desc,
                    *current, *total, r);
#endif
}
