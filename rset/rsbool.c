/* $Id: rsbool.c,v 1.37 2004-08-06 14:09:02 heikki Exp $
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

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_forward(RSET ct, RSFD rfd, void *buf, int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);
static void r_pos (RSFD rfd, double *current, double *total); 
static int r_read_and (RSFD rfd, void *buf, int *term_index);
static int r_read_or (RSFD rfd, void *buf, int *term_index);
static int r_read_not (RSFD rfd, void *buf, int *term_index);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control_and = 
{
    "and",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_forward, 
    r_pos,    
    r_read_and,
    r_write,
};

static const struct rset_control control_or = 
{
    "or",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_forward, 
    r_pos,
    r_read_or,
    r_write,
};

static const struct rset_control control_not = 
{
    "not",
    r_create,
    r_open,
    r_close,
    r_delete,
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
    int term_index_s;
    int (*cmp)(const void *p1, const void *p2);
    void (*log_item)(int logmask, const void *p, const char *txt);
    struct rset_bool_rfd *rfd_list;
};

struct rset_bool_rfd {
    zint hits;
    RSFD rfd_l;
    RSFD rfd_r;
    int  more_l;
    int  more_r;
    int term_index_l;
    int term_index_r;
    void *buf_l;
    void *buf_r;
    int tail;
    struct rset_bool_rfd *next;
    struct rset_bool_info *info;
};    

static void *r_create (RSET ct, const struct rset_control *sel, void *parms)
{
    rset_bool_parms *bool_parms = (rset_bool_parms *) parms;
    struct rset_bool_info *info;

    info = (struct rset_bool_info *) xmalloc (sizeof(*info));
    info->key_size = bool_parms->key_size;
    info->rset_l = bool_parms->rset_l;
    info->rset_r = bool_parms->rset_r;
    if (rset_is_volatile(info->rset_l) || rset_is_volatile(info->rset_r))
        ct->flags |= RSET_FLAG_VOLATILE;
    info->cmp = bool_parms->cmp;
    info->log_item = bool_parms->log_item;
    info->rfd_list = NULL;
    
    info->term_index_s = info->rset_l->no_rset_terms;
    ct->no_rset_terms =
	info->rset_l->no_rset_terms + info->rset_r->no_rset_terms;
    ct->rset_terms = (RSET_TERM *)
	xmalloc (sizeof (*ct->rset_terms) * ct->no_rset_terms);

    memcpy (ct->rset_terms, info->rset_l->rset_terms,
	    info->rset_l->no_rset_terms * sizeof(*ct->rset_terms));
    memcpy (ct->rset_terms + info->rset_l->no_rset_terms,
	    info->rset_r->rset_terms,
	    info->rset_r->no_rset_terms * sizeof(*ct->rset_terms));
    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_bool_info *info = (struct rset_bool_info *) ct->buf;
    struct rset_bool_rfd *rfd;

    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "bool set type is read-only");
	return NULL;
    }
    rfd = (struct rset_bool_rfd *) xmalloc (sizeof(*rfd));
    logf(LOG_DEBUG,"rsbool (%s) open [%p]", ct->control->desc, rfd);
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;
    rfd->hits=0;

    rfd->buf_l = xmalloc (info->key_size);
    rfd->buf_r = xmalloc (info->key_size);
    rfd->rfd_l = rset_open (info->rset_l, RSETF_READ);
    rfd->rfd_r = rset_open (info->rset_r, RSETF_READ);
    rfd->more_l = rset_read (info->rset_l, rfd->rfd_l, rfd->buf_l,
			     &rfd->term_index_l);
    rfd->more_r = rset_read (info->rset_r, rfd->rfd_r, rfd->buf_r,
			     &rfd->term_index_r);
    rfd->tail = 0;
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_bool_info *info = ((struct rset_bool_rfd*)rfd)->info;
    struct rset_bool_rfd **rfdp;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            xfree ((*rfdp)->buf_l);
            xfree ((*rfdp)->buf_r);
            rset_close (info->rset_l, (*rfdp)->rfd_l);
            rset_close (info->rset_r, (*rfdp)->rfd_r);
            *rfdp = (*rfdp)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_bool_info *info = (struct rset_bool_info *) ct->buf;

    assert (info->rfd_list == NULL);
    xfree (ct->rset_terms);
    rset_delete (info->rset_l);
    rset_delete (info->rset_r);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{
    struct rset_bool_info *info = ((struct rset_bool_rfd*)rfd)->info;
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;

    logf (LOG_DEBUG, "rsbool_rewind");
    rset_rewind (info->rset_l, p->rfd_l);
    rset_rewind (info->rset_r, p->rfd_r);
    p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l, &p->term_index_l);
    p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r, &p->term_index_r);
    p->hits=0;
}

static int r_forward (RSET ct, RSFD rfd, void *buf, int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf)
{
    struct rset_bool_info *info = ((struct rset_bool_rfd*)rfd)->info;
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    int rc;

#if RSET_DEBUG
    logf (LOG_DEBUG, "rsbool_forward (L) [%p] '%s' (ct=%p rfd=%p m=%d,%d)",
                      rfd, ct->control->desc, ct, rfd, p->more_l, p->more_r);
#endif
    if ( p->more_l && ((cmpfunc)(untilbuf,p->buf_l)==2) )
        p->more_l = rset_forward(info->rset_l, p->rfd_l, p->buf_l,
                        &p->term_index_l, info->cmp, untilbuf);
#if RSET_DEBUG
    logf (LOG_DEBUG, "rsbool_forward (R) [%p] '%s' (ct=%p rfd=%p m=%d,%d)",
                      rfd, ct->control->desc, ct, rfd, p->more_l, p->more_r);
#endif
    if ( p->more_r && ((cmpfunc)(untilbuf,p->buf_r)==2))
        p->more_r = rset_forward(info->rset_r, p->rfd_r, p->buf_r,
                        &p->term_index_r, info->cmp, untilbuf);
#if RSET_DEBUG
    logf (LOG_DEBUG, "rsbool_forward [%p] calling read, m=%d,%d t=%d", 
                       rfd, p->more_l, p->more_r, p->tail);
#endif
    
    p->tail=0; 
    rc = rset_read(ct,rfd,buf,term_index); 
#if RSET_DEBUG
    logf (LOG_DEBUG, "rsbool_forward returning [%p] %d m=%d,%d", 
                       rfd, rc, p->more_l, p->more_r);
#endif
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

static int r_read_and (RSFD rfd, void *buf, int *term_index)
{
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;

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
	        *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            p->tail = 1;
        }
        else if (cmp == 1)
        {
            memcpy (buf, p->buf_r, info->key_size);
	        *term_index = p->term_index_r + info->term_index_s;
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
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
	        *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
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
            *term_index = p->term_index_r + info->term_index_s;
            
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
                                   &p->term_index_r);
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
                *term_index = p->term_index_r + info->term_index_s;
                p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
                                   &p->term_index_r);
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
                    p->more_r = rset_forward( 
                                    info->rset_r, p->rfd_r, 
                                    p->buf_r, &p->term_index_r, 
                                    (info->cmp), p->buf_l);
                else 
                    return 0; /* no point in reading further */
            }
#endif
        }
        else  /* cmp == -2 */
        {
#if OLDCODE
             memcpy (buf, p->buf_l, info->key_size);
             *term_index = p->term_index_l;
             p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
                                    &p->term_index_l);
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
	            *term_index = p->term_index_l;
                p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
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
                    p->more_l = rset_forward( 
                    /* p->more_l = rset_default_forward( */
                                    info->rset_l, p->rfd_l, 
                                    p->buf_l, &p->term_index_l, 
                                    (info->cmp), p->buf_r);
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

static int r_read_or (RSFD rfd, void *buf, int *term_index)
{
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;

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
	        *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
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
	        *term_index = p->term_index_r + info->term_index_s;
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
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
	        *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
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

static int r_read_not (RSFD rfd, void *buf, int *term_index)
{
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;

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
	        *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            p->hits++;
            return 1;
        }
        else if (cmp > 1)
        {
	        p->more_r = rset_forward( 
	            info->rset_r, p->rfd_r, 
	            p->buf_r, &p->term_index_r, 
	            (info->cmp), p->buf_l);
        }
        else
        {
            memcpy (buf, p->buf_l, info->key_size);
            do
            { 
                p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				       &p->term_index_l);
                if (!p->more_l)
                    break;
                cmp = (*info->cmp)(p->buf_l, buf);
            } while (cmp >= -1 && cmp <= 1);
            do
            {
                p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				       &p->term_index_r);
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
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;
    double lcur,ltot;
    double rcur,rtot;
    double r;
    ltot=-1; rtot=-1;
    rset_pos(info->rset_l, p->rfd_l,  &lcur, &ltot);
    rset_pos(info->rset_r, p->rfd_r,  &rcur, &rtot);
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
