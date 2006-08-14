/* $Id: rsbetween.c,v 1.15.2.5 2006-08-14 10:39:20 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

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
#if RSBETWEEN_DEBUG
#define log_level LOG_DEBUG
#endif

static void *r_create_between(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open_between (RSET ct, int flag);
static void r_close_between (RSFD rfd);
static void r_delete_between (RSET ct);
static void r_rewind_between (RSFD rfd);
static int r_forward_between(RSET ct, RSFD rfd, void *buf, int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2),
                     const void *untilbuf);
static int r_read (RSFD rfd, void *buf, int *term_index);
static int r_write_between (RSFD rfd, const void *buf);

static const struct rset_control control_between = 
{
    "between",
    r_create_between,
    r_open_between,
    r_close_between,
    r_delete_between,
    r_rewind_between,
    r_forward_between,  /* rset_default_forward,  */
    rset_default_pos,
    r_read,
    r_write_between,
};


const struct rset_control *rset_kind_between = &control_between;

#define WHICH_L 0
#define WHICH_M 1
#define WHICH_R 2
#define WHICH_A 3

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
    /* 1.4 backport */
    void *recbuf; /* a key that tells which record we are in */
    void *startbuf; /* the start tag */
    int startbufok; /* we have seen the first start tag */
    void *attrbuf;  /* the attr tag. If these two match, we have attr match */
    int attrbufok; /* we have seen the first attr tag, can compare */
    int depth; /* number of start-tags without end-tags */
    int attrdepth; /* on what depth the attr matched */
    int hits; /* number of hits returned so far */
    /* Stuff for the multi-and read, also backported */
    /* uses more_l/m/r/a etc from above */
    int tailcount;
    int tailbits[4]; 
    int and_eof; 
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
    rfd->recbuf=xmalloc (info->key_size); 
    rfd->startbuf=xmalloc (info->key_size);
    rfd->attrbuf=xmalloc (info->key_size); 
    rfd->startbufok=0; 
    rfd->attrbufok=0; 
    rfd->depth=0;
    rfd->attrdepth=0;
    rfd->hits=-1; /* signals uninitialized */
    rfd->tailcount=0;
    rfd->tailbits[WHICH_L]=0; 
    rfd->tailbits[WHICH_M]=0; 
    rfd->tailbits[WHICH_R]=0; 
    rfd->tailbits[WHICH_A]=0; 
    rfd->and_eof=0; 
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
            xfree ((*rfdp)->recbuf);
            xfree ((*rfdp)->startbuf);
            xfree ((*rfdp)->attrbuf);
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
    /* rc = r_read_between(rfd, buf, term_index);*/
    rc = r_read(rfd, buf, term_index);
#if RSBETWEEN_DEBUG
    log2( p, "fwd: after forward", 0,0);
#endif
    return rc;
}



static int r_write_between (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "between set type is read-only");
    return -1;
}



/* Heikki's back-port of the cleaner 1.4 algorithm */

static void checkattr(struct rset_between_rfd *p)
{
    struct rset_between_info *info =p->info;
    int cmp;
    if (p->attrdepth)
        return; /* already found one */
    if (!info->rset_attr) 
    {
        p->attrdepth=-1; /* matches always */
        return;
    }
    if ( p->startbufok && p->attrbufok )
    { /* have buffers to compare */
        cmp=(*info->cmp)(p->startbuf,p->attrbuf);
        if (0==cmp) /* and the keys match */
        {
            p->attrdepth=p->depth;
#if RSBETWEEN_DEBUG
            yaz_log(log_level, "found attribute match at depth %d",p->attrdepth);
#endif
        }
    }
}


/* Implements a multi-and between the l,r, and m rfds. Returns all */
/* hits from those, provided that all three point to the same record */
/* See rsmultiandor.c in zebra 1.4 for details */
static int read_anded(struct rset_between_rfd *p,void *buf,
                      int *term, int *which) {
    RSFD rfds[4];
    RSET rsets[4];
    void *bufs[4];
    int *terms[4];
    int dummyterm;
    struct rset_between_info *info =p->info;
    int i, mintail;
    int n;
    int cmp;

    /* make the individual args into arrays, to match rsmultiandor */
    rfds[WHICH_L]=p->rfd_l;
    rfds[WHICH_M]=p->rfd_m;
    rfds[WHICH_R]=p->rfd_r;
    rfds[WHICH_A]=p->rfd_attr;
    rsets[WHICH_L]=info->rset_l;
    rsets[WHICH_M]=info->rset_m;
    rsets[WHICH_R]=info->rset_r;
    rsets[WHICH_A]=info->rset_attr;
    bufs[WHICH_L]=p->buf_l;
    bufs[WHICH_M]=p->buf_m;
    bufs[WHICH_R]=p->buf_r;
    bufs[WHICH_A]=p->buf_attr;
    terms[WHICH_L]=&(p->term_index_l);
    terms[WHICH_M]=&(p->term_index_m);
    terms[WHICH_R]=&(p->term_index_r);
    terms[WHICH_A]=&dummyterm;
    dummyterm=0;
    if (info->rset_attr)
        n=4;
    else
        n=3;
    while (1) {
        if (p->tailcount)
        { /* we are tailing */
            mintail=0;
            while ((mintail<n) && !p->tailbits[mintail])
                mintail++; /* first tail */
            for (i=mintail+1;i<n;i++)
            {
                if (p->tailbits[i])
                {
                    cmp=(*info->cmp)(bufs[i],bufs[mintail]);
                    if (cmp<0)
                        mintail=i;
                }
            }
            /* return the lowest tail */
            memcpy(buf, bufs[mintail], info->key_size);
            *which=mintail;
            *term=*terms[mintail];
            /* and read that one further */
            if (!rset_read(rsets[mintail],rfds[mintail], 
                           bufs[mintail],terms[mintail]))
            {
                p->and_eof=1; 
                p->tailbits[mintail]=0;
                (p->tailcount)--;
                return 1;
            }
            /* still a tail? */
            cmp=(info->cmp)(bufs[mintail],buf);
            if (cmp>=2) 
            {
                p->tailbits[mintail]=0;
                (p->tailcount)--;
            }
            return 1;
        }
        else
        { /* not tailing */
            if (p->and_eof)
                return 0; /* game over */
            i=1; /* assume items[0] is highest up */
            while (i<n) 
            {
                cmp=(*info->cmp)(bufs[0],bufs[i]);
                if (cmp<=-2)
                { /* [0] was behid, forward it and start anew */
                    if (!rset_forward(rsets[0],rfds[0], bufs[0], 
                                      terms[0], info->cmp, bufs[i])) 
                    {
                        p->and_eof=1;
                        return 0;
                    }
                    i=0; /* all over again */
                }
                else if (cmp>=2)
                {/* [0] was ahead, forward i */
                    if (!rset_forward(rsets[i],rfds[i], bufs[i], 
                                      terms[i], info->cmp, bufs[0])) 
                    {
                        p->and_eof=1;
                        return 0;
                    }

                } else
                    i++; 
            } /* while i */
            /* now all are within the same records, set tails and let upper 
             * if return hits */
            for (i=0; i<n;i++)
                p->tailbits[i]=1;
            p->tailcount=n;
        }
    } 
} /* read_anded */

static int r_read (RSFD rfd, void *buf, int *term)
{
    struct rset_between_rfd *p=(struct rset_between_rfd *)rfd;
    struct rset_between_info *info =p->info;
    int cmp;
    int thisterm=0;
    int which; 
    *term=0; /* just in case, should not be necessary */
#if RSBETWEEN_DEBUG
    yaz_log(log_level,"btw: == read: term=%p",term);
#endif
    while ( read_anded(p,buf,&thisterm,&which) )
    {
#if RSBETWEEN_DEBUG
        yaz_log(log_level,"btw: read loop term=%p d=%d ad=%d",
                term, p->depth, p->attrdepth);
#endif
        if (p->hits<0) 
        {/* first time? */
            memcpy(p->recbuf,buf,info->key_size);
            p->hits=0;
            cmp=2;
        }
        else {
            cmp=(*info->cmp)(buf,p->recbuf);
#if RSBETWEEN_DEBUG
            yaz_log(log_level, "btw: cmp=%d",cmp);
#endif
        }

        if (cmp>=2)
        { 
#if RSBETWEEN_DEBUG
            yaz_log(log_level,"btw: new record");
#endif
            p->depth=0;
            p->attrdepth=0;
            memcpy(p->recbuf,buf,info->key_size);
        }

#if RSBETWEEN_DEBUG
        yaz_log(log_level,"btw:  which: %d", which); 
#endif
        if (which==WHICH_L)
        {
            p->depth++;
#if RSBETWEEN_DEBUG
            yaz_log(log_level,"btw: read start tag. d=%d",p->depth);
#endif
            memcpy(p->startbuf,buf,info->key_size);
            p->startbufok=1;
            checkattr(rfd);  /* in case we already saw the attr here */
        }
        else if (which==WHICH_R)  
        {
            if (p->depth == p->attrdepth)
                p->attrdepth=0; /* ending the tag with attr match */
            p->depth--;
#if RSBETWEEN_DEBUG
            yaz_log(log_level,"btw: read end tag. d=%d ad=%d",
                    p->depth, p->attrdepth);
#endif
        }
        else if (which==WHICH_A)
        {
#if RSBETWEEN_DEBUG
            yaz_log(log_level,"btw: read attr");
#endif
            memcpy(p->attrbuf,buf,info->key_size);
            p->attrbufok=1;
            checkattr(rfd); /* in case the start tag came first */
        }
        else 
        { /* mut be a real hit */
            if (p->depth && p->attrdepth)
            {
                p->hits++;
#if RSBETWEEN_DEBUG
                yaz_log(log_level,"btw: got a hit h=%d d=%d ad=%d t=%d+%d", 
                        p->hits,p->depth,p->attrdepth,
                        info->rset_m->no_rset_terms,thisterm);
#endif
                *term= info->rset_m->no_rset_terms + thisterm;
                return 1; /* everything else is in place already */
            } else
            {
#if RSBETWEEN_DEBUG
                yaz_log(log_level, "btw: Ignoring hit. h=%d d=%d ad=%d",
                        p->hits,p->depth,p->attrdepth);
#endif
            }
        }
    } /* while read */

    return 0;

}  /* r_read */

