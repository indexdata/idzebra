/* $Id: rsbetween.c,v 1.14 2004-08-03 12:15:45 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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


/* rsbetween is (mostly) used for xml searches. It returns the hits of the
 * "middle" rset, that are in between the "left" and "right" rsets. For
 * example "Shakespeare" in between "<title>" and </title>. The thing is 
 * complicated by the inclusion of attributes (from their own rset). If attrs
 * specified, they must match the "left" rset (start tag). "Hamlet" between
 * "<title lang=eng>" and "</title>". (This assumes that the attributes are
 * indexed to the same seqno as the tags).
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <rsbetween.h>
#include <zebrautl.h>

#define RSBETWEEN_DEBUG 0

static void *r_create_between(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open_between (RSET ct, int flag);
static void r_close_between (RSFD rfd);
static void r_delete_between (RSET ct);
static void r_rewind_between (RSFD rfd);
static int r_forward_between(RSET ct, RSFD rfd, void *buf, int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);
/* static int r_count_between (RSET ct); */
static int r_read_between (RSFD rfd, void *buf, int *term_index);
static int r_write_between (RSFD rfd, const void *buf);

static const struct rset_control control_between = 
{
    "between",
    r_create_between,
    r_open_between,
    r_close_between,
    r_delete_between,
    r_rewind_between,
    r_forward_between, /* rset_default_forward, */
    /* r_count_between, */
    r_read_between,
    r_write_between,
};


const struct rset_control *rset_kind_between = &control_between;

struct rset_between_info {
    int key_size;
    RSET rset_l;
    RSET rset_m;
    RSET rset_r;
    RSET rset_attr;
    int term_index_s;
    int (*cmp)(const void *p1, const void *p2);
    char *(*printer)(const void *p1, char *buf);
    struct rset_between_rfd *rfd_list;
};

struct rset_between_rfd {
    RSFD rfd_l;
    RSFD rfd_m;
    RSFD rfd_r;
    RSFD rfd_attr;
    int  more_l;
    int  more_m;
    int  more_r;
    int  more_attr;
    int term_index_l;
    int term_index_m;
    int term_index_r;
    void *buf_l;
    void *buf_m;
    void *buf_r;
    void *buf_attr;
    int level;
    struct rset_between_rfd *next;
    struct rset_between_info *info;
};    

#if RSBETWEEN_DEBUG
static void log2 (struct rset_between_rfd *p, char *msg, int cmp_l, int cmp_r)
{
    char buf_l[32];
    char buf_m[32];
    char buf_r[32];
    logf(LOG_DEBUG,"btw: %s l=%s(%d/%d) m=%s(%d) r=%s(%d/%d), lev=%d",
      msg, 
      (*p->info->printer)(p->buf_l, buf_l), p->more_l, cmp_l,
      (*p->info->printer)(p->buf_m, buf_m), p->more_m,
      (*p->info->printer)(p->buf_r, buf_r), p->more_r, cmp_r,
      p->level);
}
#endif

static void *r_create_between (RSET ct, const struct rset_control *sel,
                               void *parms)
{
    rset_between_parms *between_parms = (rset_between_parms *) parms;
    struct rset_between_info *info;

    info = (struct rset_between_info *) xmalloc (sizeof(*info));
    info->key_size = between_parms->key_size;
    info->rset_l = between_parms->rset_l;
    info->rset_m = between_parms->rset_m;
    info->rset_r = between_parms->rset_r;
    info->rset_attr = between_parms->rset_attr;
    if (rset_is_volatile(info->rset_l) || 
        rset_is_volatile(info->rset_m) ||
        rset_is_volatile(info->rset_r))
        ct->flags |= RSET_FLAG_VOLATILE;
    info->cmp = between_parms->cmp;
    info->printer = between_parms->printer;
    info->rfd_list = NULL;
    
    info->term_index_s = info->rset_l->no_rset_terms;
    if (info->rset_m)
    {
        ct->no_rset_terms =
            info->rset_l->no_rset_terms + 
            info->rset_m->no_rset_terms + 
            info->rset_r->no_rset_terms;
        ct->rset_terms = (RSET_TERM *)
            xmalloc (sizeof (*ct->rset_terms) * ct->no_rset_terms);
        memcpy (ct->rset_terms, info->rset_l->rset_terms,
                info->rset_l->no_rset_terms * sizeof(*ct->rset_terms));
        memcpy (ct->rset_terms + info->rset_l->no_rset_terms,
                info->rset_m->rset_terms,
                info->rset_m->no_rset_terms * sizeof(*ct->rset_terms));
        memcpy (ct->rset_terms + info->rset_l->no_rset_terms + 
                info->rset_m->no_rset_terms,
                info->rset_r->rset_terms,
                info->rset_r->no_rset_terms * sizeof(*ct->rset_terms));
    }
    else
    {
        ct->no_rset_terms =
            info->rset_l->no_rset_terms + 
            info->rset_r->no_rset_terms;
        ct->rset_terms = (RSET_TERM *)
            xmalloc (sizeof (*ct->rset_terms) * ct->no_rset_terms);
        memcpy (ct->rset_terms, info->rset_l->rset_terms,
                info->rset_l->no_rset_terms * sizeof(*ct->rset_terms));
        memcpy (ct->rset_terms + info->rset_l->no_rset_terms,
                info->rset_r->rset_terms,
                info->rset_r->no_rset_terms * sizeof(*ct->rset_terms));
    }

    return info;
}

static RSFD r_open_between (RSET ct, int flag)
{
    struct rset_between_info *info = (struct rset_between_info *) ct->buf;
    struct rset_between_rfd *rfd;

    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "between set type is read-only");
        return NULL;
    }
    rfd = (struct rset_between_rfd *) xmalloc (sizeof(*rfd));
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;

    rfd->buf_l = xmalloc (info->key_size);
    rfd->buf_m = xmalloc (info->key_size);
    rfd->buf_r = xmalloc (info->key_size);
    rfd->buf_attr = xmalloc (info->key_size);

    rfd->rfd_l = rset_open (info->rset_l, RSETF_READ);
    rfd->rfd_m = rset_open (info->rset_m, RSETF_READ);
    rfd->rfd_r = rset_open (info->rset_r, RSETF_READ);
    
    rfd->more_l = rset_read (info->rset_l, rfd->rfd_l, rfd->buf_l,
                             &rfd->term_index_l);
    rfd->more_m = rset_read (info->rset_m, rfd->rfd_m, rfd->buf_m,
                             &rfd->term_index_m);
    rfd->more_r = rset_read (info->rset_r, rfd->rfd_r, rfd->buf_r,
                             &rfd->term_index_r);
    if (info->rset_attr)
    {
        int dummy;
        rfd->rfd_attr = rset_open (info->rset_attr, RSETF_READ);
        rfd->more_attr = rset_read (info->rset_attr, rfd->rfd_attr,
                                    rfd->buf_attr, &dummy);
    }
    rfd->level=0;
    return rfd;
}

static void r_close_between (RSFD rfd)
{
    struct rset_between_info *info = ((struct rset_between_rfd*)rfd)->info;
    struct rset_between_rfd **rfdp;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            xfree ((*rfdp)->buf_l);
            xfree ((*rfdp)->buf_m);
            xfree ((*rfdp)->buf_r);
            xfree ((*rfdp)->buf_attr);
            rset_close (info->rset_l, (*rfdp)->rfd_l);
            rset_close (info->rset_m, (*rfdp)->rfd_m);
            rset_close (info->rset_r, (*rfdp)->rfd_r);
            if (info->rset_attr)
                rset_close (info->rset_attr, (*rfdp)->rfd_attr);
            
            *rfdp = (*rfdp)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close_between but no rfd match!");
    assert (0);
}

static void r_delete_between (RSET ct)
{
    struct rset_between_info *info = (struct rset_between_info *) ct->buf;

    assert (info->rfd_list == NULL);
    xfree (ct->rset_terms);
    rset_delete (info->rset_l);
    rset_delete (info->rset_m);
    rset_delete (info->rset_r);
    if (info->rset_attr)
        rset_delete (info->rset_attr);
    xfree (info);
}

static void r_rewind_between (RSFD rfd)
{
    struct rset_between_info *info = ((struct rset_between_rfd*)rfd)->info;
    struct rset_between_rfd *p = (struct rset_between_rfd *) rfd;

#if RSBETWEEN_DEBUG
    logf (LOG_DEBUG, "rsbetween_rewind");
#endif
    rset_rewind (info->rset_l, p->rfd_l);
    rset_rewind (info->rset_m, p->rfd_m);
    rset_rewind (info->rset_r, p->rfd_r);
    p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l, &p->term_index_l);
    p->more_m = rset_read (info->rset_m, p->rfd_m, p->buf_m, &p->term_index_m);
    p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r, &p->term_index_r);
    if (info->rset_attr)
    {
        int dummy;
        rset_rewind (info->rset_attr, p->rfd_attr);
        p->more_attr = rset_read (info->rset_attr, p->rfd_attr, p->buf_attr,
                                  &dummy);
    }
    p->level=0;
}



static int r_forward_between(RSET ct, RSFD rfd, void *buf, int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf)
{
    struct rset_between_info *info = ((struct rset_between_rfd*)rfd)->info;
    struct rset_between_rfd *p = (struct rset_between_rfd *) rfd;
    int rc;
#if RSBETWEEN_DEBUG
    log2( p, "fwd: before forward", 0,0);
#endif
    /* It is enough to forward the m pointer here, the read will */
    /* naturally forward the l, m, and attr pointers */
    if (p->more_m)
        p->more_m=rset_forward(info->rset_m,p->rfd_m, p->buf_m,
                        &p->term_index_m, info->cmp,untilbuf);
#if RSBETWEEN_DEBUG
    log2( p, "fwd: after forward M", 0,0);
#endif
    rc = r_read_between(rfd, buf, term_index);
#if RSBETWEEN_DEBUG
    log2( p, "fwd: after forward", 0,0);
#endif
    return rc;
}

/*
static int r_count_between (RSET ct)
{
    return 0;
}
*/



static int r_read_between (RSFD rfd, void *buf, int *term_index)
{
    struct rset_between_rfd *p = (struct rset_between_rfd *) rfd;
    struct rset_between_info *info = p->info;
    int cmp_l=0;
    int cmp_r=0;
    int attr_match = 0;

    while (p->more_m)
    {
#if RSBETWEEN_DEBUG
        log2( p, "start of loop", cmp_l, cmp_r);
#endif

        /* forward L until past m, count levels, note rec boundaries */
        if (p->more_l)
            cmp_l= (*info->cmp)(p->buf_l, p->buf_m);
        else
        {
            p->level = 0;
            cmp_l=2; /* past this record */
        }
#if RSBETWEEN_DEBUG
        log2( p, "after first L", cmp_l, cmp_r);
#endif

        while (cmp_l < 0)   /* l before m */
        {
            if (cmp_l == -2)
                p->level=0; /* earlier record */
            if (cmp_l == -1)
            {
                p->level++; /* relevant start tag */

                if (!info->rset_attr)
                    attr_match = 1;
                else
                {
                    int cmp_attr;
                    int dummy_term;
                    attr_match = 0;
                    while (p->more_attr)
                    {
                        cmp_attr = (*info->cmp)(p->buf_attr, p->buf_l);
                        if (cmp_attr == 0)
                        {
                            attr_match = 1;
                            break;
                        }
                        else if (cmp_attr > 0)
                            break;
                        else if (cmp_attr==-1) 
                            p->more_attr = rset_read (info->rset_attr, p->rfd_attr,
                                                  p->buf_attr, &dummy_term);
                            /* if we had a forward that went all the way to
                             * the seqno, we could use that. But fwd only goes
                             * to the sysno */
                        else if (cmp_attr==-2) 
                        {
                            p->more_attr = rset_forward(
                                      info->rset_attr, p->rfd_attr,
                                      p->buf_attr, &dummy_term,
                                      info->cmp, p->buf_l);
#if RSBETWEEN_DEBUG
                            logf(LOG_DEBUG, "btw: after frowarding attr m=%d",p->more_attr);
#endif
                        }
                    } /* while more_attr */
                }
            }
#define NEWCODE 1 
#if NEWCODE                
            if (cmp_l==-2)
            {
                if (p->more_l) 
                {
                    p->more_l=rset_forward(
                                      info->rset_l, p->rfd_l,
                                      p->buf_l, &p->term_index_l,
                                      info->cmp, p->buf_m);
                    if (p->more_l)
                        cmp_l= (*info->cmp)(p->buf_l, p->buf_m);
                    else
                        cmp_l=2;
#if RSBETWEEN_DEBUG
                    log2( p, "after forwarding L", cmp_l, cmp_r);
#endif
                }
            } else
            {
                p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
                              &p->term_index_l);
            }
#else
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
                              &p->term_index_l);
#endif
            if (p->more_l)
            {
                cmp_l= (*info->cmp)(p->buf_l, p->buf_m);
            }
            else
                cmp_l=2; 
#if RSBETWEEN_DEBUG
            log2( p, "end of L loop", cmp_l, cmp_r);
#endif
        } /* forward L */

            
        /* forward R until past m, count levels */
#if RSBETWEEN_DEBUG
        log2( p, "Before moving R", cmp_l, cmp_r);
#endif
        if (p->more_r)
            cmp_r= (*info->cmp)(p->buf_r, p->buf_m);
        else
            cmp_r=2; 
#if RSBETWEEN_DEBUG
        log2( p, "after first R", cmp_l, cmp_r);
#endif
        while (cmp_r < 0)   /* r before m */
        {
             /* -2, earlier record, don't count level */
            if (cmp_r == -1)
                p->level--; /* relevant end tag */
            if (p->more_r)
            {
#if NEWCODE                
                if (cmp_r==-2)
                {
                    p->more_r=rset_forward(
                                      info->rset_r, p->rfd_r,
                                      p->buf_r, &p->term_index_r,
                                      info->cmp, p->buf_m);
                } else
                {
                    p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
                                       &p->term_index_r);
                }
                if (p->more_r)
                    cmp_r= (*info->cmp)(p->buf_r, p->buf_m);

#else
                p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
                                       &p->term_index_r);
                cmp_r= (*info->cmp)(p->buf_r, p->buf_m);
#endif
            }
            else
                cmp_r=2; 
#if RSBETWEEN_DEBUG
        log2( p, "End of R loop", cmp_l, cmp_r);
#endif
        } /* forward R */
        
        if ( ( p->level <= 0 ) && ! p->more_l)
            return 0; /* no more start tags, nothing more to find */
        
        if ( attr_match && p->level > 0)  /* within a tag pair (or deeper) */
        {
            memcpy (buf, p->buf_m, info->key_size);
            *term_index = p->term_index_m;
#if RSBETWEEN_DEBUG
            log2( p, "Returning a hit (and forwarding m)", cmp_l, cmp_r);
#endif
            p->more_m = rset_read (info->rset_m, p->rfd_m, p->buf_m,
                                   &p->term_index_m);
            if (cmp_l == 2)
                p->level = 0;
            return 1;
        }
        else if ( ! p->more_l )  /* not in data, no more starts */
        {
#if RSBETWEEN_DEBUG
            log2( p, "no more starts, exiting without a hit", cmp_l, cmp_r);
#endif
            return 0;  /* ergo, nothing can be found. stop scanning */
        }
#if NEWCODE                
        if (cmp_l == 2)
        {
            p->level = 0;
            p->more_m=rset_forward(
                              info->rset_m, p->rfd_m,
                              p->buf_m, &p->term_index_m,
                              info->cmp, p->buf_l);
        } else
        {
            p->more_m = rset_read (info->rset_m, p->rfd_m, p->buf_m,
                               &p->term_index_m);
        }
#else
        if (cmp_l == 2)
            p->level = 0;
        p->more_m = rset_read (info->rset_m, p->rfd_m, p->buf_m,
                               &p->term_index_m);
#endif
#if RSBETWEEN_DEBUG
        log2( p, "End of M loop", cmp_l, cmp_r);
#endif
    } /* while more_m */
    
#if RSBETWEEN_DEBUG
    log2( p, "Exiting, nothing more in m", cmp_l, cmp_r);
#endif
    return 0;  /* no more data possible */


}  /* r_read */


static int r_write_between (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "between set type is read-only");
    return -1;
}

