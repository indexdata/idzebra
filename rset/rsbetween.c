/* $Id: rsbetween.c,v 1.29 2004-11-01 15:53:57 heikki Exp $
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
 * example "Shakespeare" in between "<author>" and </author>. The thing is 
 * complicated by the inclusion of attributes (from their own rset). If attrs
 * specified, they must match the "left" rset (start tag). "Hamlet" between
 * "<title lang=eng>" and "</title>". (This assumes that the attributes are
 * indexed to the same seqno as the tags).
 *
 * Currently fails to return all hits from a record, which breaks the ranking.
 * This is bug #202. Seems like there is no decent way to get that working
 * with the current implementation. I have planned a new method for doing
 * this, see at the end of this file.
 *
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zebrautl.h>
#include <rset.h>

#define RSBETWEEN_DEBUG 0 

static RSFD r_open_between (RSET ct, int flag);
static void r_close_between (RSFD rfd);
static void r_delete_between (RSET ct);
static int r_forward_between(RSFD rfd, void *buf, 
                    TERMID *term, const void *untilbuf);
static int r_read_between (RSFD rfd, void *buf, TERMID *term );
static int r_write_between (RSFD rfd, const void *buf);
static void r_pos_between (RSFD rfd, double *current, double *total);
static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm);

static const struct rset_control control = 
{
    "between",
    r_delete_between,
    r_get_terms,
    r_open_between,
    r_close_between,
    r_forward_between, 
    r_pos_between,
    r_read_between,
    r_write_between,
};


const struct rset_control *rset_kind_between = &control;

struct rset_between_info {
    RSET rset_l;  /* left arg, start tag */
    RSET rset_m;  /* the thing itself */
    RSET rset_r;  /* right arg, end tag */
    RSET rset_attr; /* attributes , optional */
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
    void *buf_l;
    void *buf_m;
    void *buf_r;
    void *buf_attr;
    TERMID term_m; /* we only return terms for the mid argument */
    int level;  /* counting start/end tags */
    int attr_match; /* did we have a matching attr for L */
    zint hits;
};    

#if RSBETWEEN_DEBUG
static void log2 (RSFD rfd, char *msg, int cmp_l, int cmp_r)
{
    struct rset_between_rfd *p=(struct rset_between_rfd *)rfd->priv;
    RSET ct = rfd->rset;
    logf(LOG_LOG,"between: %s cmp_l=%d cmp_r=%d", msg, cmp_l, cmp_r);
    (*ct->keycontrol->key_logdump_txt)(LOG_LOG, p->buf_l, "between: L");
    (*ct->keycontrol->key_logdump_txt)(LOG_LOG, p->buf_m, "between: M");
    (*ct->keycontrol->key_logdump_txt)(LOG_LOG, p->buf_r, "between: R");
}
#endif

RSET rsbetween_create( NMEM nmem, const struct key_control *kcontrol,
            int scope,
            RSET rset_l, RSET rset_m, RSET rset_r, RSET rset_attr)
{
    RSET rnew=rset_create_base(&control, nmem, kcontrol, scope,0);
    struct rset_between_info *info=
        (struct rset_between_info *) nmem_malloc(rnew->nmem,sizeof(*info));
    info->rset_l = rset_l;
    info->rset_m = rset_m;
    info->rset_r = rset_r;
    info->rset_attr = rset_attr;
    rnew->priv=info;
    return rnew;
}


static void r_delete_between (RSET ct)
{
    struct rset_between_info *info = (struct rset_between_info *) ct->priv;

    rset_delete (info->rset_l);
    rset_delete (info->rset_m);
    rset_delete (info->rset_r);
    if (info->rset_attr)
        rset_delete (info->rset_attr);
}


static RSFD r_open_between (RSET ct, int flag)
{
    struct rset_between_info *info = (struct rset_between_info *) ct->priv;
    RSFD rfd;
    struct rset_between_rfd *p;

    if (flag & RSETF_WRITE)
    {
        logf (LOG_FATAL, "between set type is read-only");
        return NULL;
    }
    rfd=rfd_create_base(ct);
    if (rfd->priv)  
        p=(struct rset_between_rfd *)rfd->priv;
    else {
        p = (struct rset_between_rfd *) nmem_malloc(ct->nmem, (sizeof(*p)));
        rfd->priv=p;
        p->buf_l = nmem_malloc(ct->nmem, (ct->keycontrol->key_size));
        p->buf_m = nmem_malloc(ct->nmem, (ct->keycontrol->key_size));
        p->buf_r = nmem_malloc(ct->nmem, (ct->keycontrol->key_size));
        p->buf_attr = nmem_malloc(ct->nmem, (ct->keycontrol->key_size));
    }

    p->rfd_l = rset_open (info->rset_l, RSETF_READ);
    p->rfd_m = rset_open (info->rset_m, RSETF_READ);
    p->rfd_r = rset_open (info->rset_r, RSETF_READ);
    
    p->more_l = rset_read (p->rfd_l, p->buf_l,NULL);
    p->more_m = rset_read (p->rfd_m, p->buf_m, &p->term_m);
    p->more_r = rset_read (p->rfd_r, p->buf_r,NULL);
    if (info->rset_attr)
    {
        p->rfd_attr = rset_open (info->rset_attr, RSETF_READ);
        p->more_attr = rset_read (p->rfd_attr, p->buf_attr, NULL);
        p->attr_match=0;
    } else
        p->attr_match=1; 
    p->level=0;
    p->hits=0;
    return rfd;
}

static void r_close_between (RSFD rfd)
{
    struct rset_between_info *info =(struct rset_between_info *)rfd->rset->priv;
    struct rset_between_rfd *p=(struct rset_between_rfd *)rfd->priv;

#if RSBETWEEN_DEBUG
    log2( rfd, "fwd: close. hits:", p->hits,0);
#endif
    rset_close (p->rfd_l);
    rset_close (p->rfd_m);
    rset_close (p->rfd_r);
    if (info->rset_attr)
        rset_close (p->rfd_attr);
    rfd_delete_base(rfd);
}



static int r_forward_between(RSFD rfd, void *buf, 
                             TERMID *term, const void *untilbuf)
{
    struct rset_between_rfd *p=(struct rset_between_rfd *)rfd->priv;
    int rc;
#if RSBETWEEN_DEBUG
    log2( rfd, "fwd: before forward", 0,0);
#endif
    /* It is enough to forward the m pointer here, the read will */
    /* naturally forward the l, m, and attr pointers */
    if (p->more_m)
        p->more_m=rset_forward(p->rfd_m, p->buf_m, term, untilbuf);
#if RSBETWEEN_DEBUG
    log2( rfd, "fwd: after forward M", 0,0);
#endif
    rc = r_read_between(rfd, buf, term);
#if RSBETWEEN_DEBUG
    log2( rfd, "fwd: after forward", 0,0);
#endif
    return rc;
}




static int r_read_between (RSFD rfd, void *buf, TERMID *term)
{
    struct rset_between_info *info =(struct rset_between_info *)rfd->rset->priv;
    struct rset_between_rfd *p=(struct rset_between_rfd *)rfd->priv;
    const struct key_control *kctrl=rfd->rset->keycontrol;
    int cmp_l=0;
    int cmp_r=0;
/*    int attr_match = 0; */

    while (p->more_m)
    {
#if RSBETWEEN_DEBUG
        log2( rfd, "start of loop", cmp_l, cmp_r);
        logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                p->level,p->more_l,p->more_m, p->more_r);
#endif

        /* forward L until past m, count levels, note rec boundaries */
        if (p->more_l)
            cmp_l= (*kctrl->cmp)(p->buf_l, p->buf_m);
        else
        {
            p->level = 0;
            cmp_l=rfd->rset->scope; /* past this record */
        }
#if RSBETWEEN_DEBUG
        log2( rfd, "after first L", cmp_l, cmp_r);
        logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                p->level,p->more_l,p->more_m, p->more_r);
#endif

        while (cmp_l < 0)   /* l before m */
        {
            if (cmp_l <= - rfd->rset->scope)  /* ==-2 */
                p->level=0; /* earlier record */
            if (cmp_l > - rfd->rset->scope)  /* == -1 */
            {
                p->level++; /* relevant start tag */

                if (!info->rset_attr)
                    p->attr_match = 1;
                else
                {
                    int cmp_attr;
                    p->attr_match = 0;
                    while (p->more_attr)
                    {
                        cmp_attr = (*kctrl->cmp)(p->buf_attr, p->buf_l);
                        if (cmp_attr == 0)
                        {
                            p->attr_match = 1;
                            break;
                        }
                        else if (cmp_attr > 0)
                            break;
                        else if (cmp_attr > - rfd->rset->scope)  /* == -1 */
                            p->more_attr = rset_read (p->rfd_attr, 
                                                      p->buf_attr,NULL);
                            /* if we had a forward that went all the way to
                             * the seqno, we could use that. But fwd only goes
                             * to the sysno */
                        else if (cmp_attr <= - rfd->rset->scope) /* ==-2 */
                        {
                            p->more_attr = rset_forward( p->rfd_attr,
                                             p->buf_attr, NULL, p->buf_l);
#if RSBETWEEN_DEBUG
                            logf(LOG_LOG, "btw: after frowarding attr m=%d",
                                      p->more_attr);
#endif
                        }
                    } /* while more_attr */
                }
            }
#define NEWCODE 1 
#if NEWCODE                
            if (cmp_l <= - rfd->rset->scope )/* ==-2 */
            {
                if (p->more_l) 
                {
                    p->more_l=rset_forward(p->rfd_l, p->buf_l, NULL, p->buf_m);
                    if (p->more_l)
                        cmp_l= (*kctrl->cmp)(p->buf_l, p->buf_m);
                    else
                        cmp_l=rfd->rset->scope;  /*2*/
#if RSBETWEEN_DEBUG
                    log2( rfd, "after forwarding L", cmp_l, cmp_r);
                    logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                            p->level,p->more_l,p->more_m, p->more_r);
#endif
                }
            } else
            {
                p->more_l = rset_read (p->rfd_l, p->buf_l, NULL);
            }
#else
            p->more_l = rset_read (p->rfd_l, p->buf_l, NULL);
#endif
            if (p->more_l)
            {
                cmp_l= (*kctrl->cmp)(p->buf_l, p->buf_m);
            }
            else
                cmp_l=rfd->rset->scope;   /*2*/
#if RSBETWEEN_DEBUG
            log2( rfd, "end of L loop", cmp_l, cmp_r);
            logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                p->level,p->more_l,p->more_m, p->more_r);
#endif
        } /* forward L */

            
        /* forward R until past m, count levels */
#if RSBETWEEN_DEBUG
        log2( rfd, "Before moving R", cmp_l, cmp_r);
        logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                p->level,p->more_l,p->more_m, p->more_r);
#endif
        if (p->more_r)
            cmp_r= (*kctrl->cmp)(p->buf_r, p->buf_m);
        else
            cmp_r=rfd->rset->scope;  /*2*/
#if RSBETWEEN_DEBUG
        log2( rfd, "after first R", cmp_l, cmp_r);
        logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                p->level,p->more_l,p->more_m, p->more_r);
#endif
        while (cmp_r < 0)   /* r before m */
        {
             /* -2, earlier record, don't count level */
            if (cmp_r > -rfd->rset->scope) /* == -1 */
                p->level--; /* relevant end tag */
            if (p->more_r)
            {
#if NEWCODE                
                if (cmp_r <= - rfd->rset->scope) /* == -2 */
                {
                    p->more_r=rset_forward(p->rfd_r, p->buf_r, NULL, p->buf_m);
                } else
                {
                    p->more_r = rset_read (p->rfd_r, p->buf_r, NULL);
                }
                if (p->more_r)
                    cmp_r= (*kctrl->cmp)(p->buf_r, p->buf_m);

#else
                p->more_r = rset_read (p->rfd_r, p->buf_r, NULL);
                cmp_r= (*kctrl->cmp)(p->buf_r, p->buf_m);
#endif
            }
            else
                cmp_r=rfd->rset->scope;  /*2*/
#if RSBETWEEN_DEBUG
        log2( rfd, "End of R loop", cmp_l, cmp_r);
        logf(LOG_LOG,"level=%d. more=%d/%d/%d am=%d",
                p->level,p->more_l,p->more_m, p->more_r, p->attr_match);
#endif
        } /* forward R */
        
        if ( ( p->level <= 0 ) && ! p->more_l)
        {
#if RSBETWEEN_DEBUG
            logf(LOG_LOG,"no more_l, returning zero");
#endif
            return 0; /* no more start tags, nothing more to find */
        }
        
#if RSBETWEEN_DEBUG
        log2( rfd, "Considering M", cmp_l, cmp_r);
        logf(LOG_LOG,"level=%d. more=%d/%d/%d am=%d",
                p->level,p->more_l,p->more_m, p->more_r, p->attr_match);
#endif
        if ( p->attr_match && p->level > 0)  /* within a tag pair (or deeper) */
        {
            memcpy (buf, p->buf_m, kctrl->key_size);
            if (term)
                *term=p->term_m;
#if RSBETWEEN_DEBUG
            log2( rfd, "Returning a hit (and forwarding m)", cmp_l, cmp_r);
#endif
            p->more_m = rset_read (p->rfd_m, p->buf_m, NULL);
#if RSBETWEEN_DEBUG
            logf(LOG_LOG,"read m. more=%d level=%d "
                    "cmp_l=%d scope=%d hits="ZINT_FORMAT, 
                    p->more_m, p->level,
                    cmp_l, rfd->rset->scope, p->hits);
#endif
            if (cmp_l >= rfd->rset->scope)  /* == 2 */
                p->level = 0;
            p->hits++;
            return 1;
        }
        else if ( ! p->more_l )  /* not in data, no more starts */
        {
#if RSBETWEEN_DEBUG
            log2( rfd, "no more starts, exiting without a hit", cmp_l, cmp_r);
#endif
            return 0;  /* ergo, nothing can be found. stop scanning */
        }
#if NEWCODE                
        if (cmp_l >= rfd->rset->scope) /* == 2 */
        {
            p->level = 0;
            p->more_m=rset_forward(p->rfd_m, p->buf_m, &p->term_m, p->buf_l);
        } else
        {
            p->more_m = rset_read (p->rfd_m, p->buf_m, &p->term_m);
        }
#else
        if (cmp_l >= rfd->rset->scope )  /* == 2 */
            p->level = 0;
        p->more_m = rset_read (p->rfd_m, p->buf_m, &p->term_m);
#endif
#if RSBETWEEN_DEBUG
        log2( rfd, "End of M loop", cmp_l, cmp_r);
        logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                p->level,p->more_l,p->more_m, p->more_r);
#endif
    } /* while more_m */
    
#if RSBETWEEN_DEBUG
    log2( rfd, "Exiting, nothing more in m", cmp_l, cmp_r);
    logf(LOG_LOG,"level=%d. more=%d/%d/%d",
                p->level,p->more_l,p->more_m, p->more_r);
#endif
    return 0;  /* no more data possible */


}  /* r_read */


static int r_write_between (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "between set type is read-only");
    return -1;
}


static void r_pos_between (RSFD rfd, double *current, double *total)
{
    struct rset_between_rfd *p=(struct rset_between_rfd *)rfd->priv;
    double lcur,ltot;
    double mcur,mtot;
    double rcur,rtot;
    double r;
    ltot=-1; rtot=-1;
    rset_pos(p->rfd_l,  &lcur, &ltot);
    rset_pos(p->rfd_m,  &mcur, &mtot);
    rset_pos(p->rfd_r,  &rcur, &rtot);
    if ( (ltot<0) && (mtot<0) && (rtot<0) ) { /*no position */
        *current=mcur;  /* return same as you got */
        *total=mtot;    /* probably -1 for not available */
    }
    if ( ltot<0) { ltot=0; lcur=0;} /* if only one useful, use it */
    if ( mtot<0) { mtot=0; mcur=0;}
    if ( rtot<0) { rtot=0; rcur=0;}
    if ( ltot+mtot+rtot < 1 ) { /* empty rset */
        *current=0;
        *total=0;
        return;
    }
    r=1.0*(lcur+mcur+rcur)/(ltot+mtot+rtot); /* weighed average of l and r */
    *current=p->hits;
    *total=*current/r ; 
#if RSBETWEEN_DEBUG
    {
    struct rset_between_info *info =(struct rset_between_info *)rfd->rset->priv;
    yaz_log(LOG_LOG,"betw_pos: (%s/%s) %0.1f/%0.1f= %0.4f ",
                    info->rset_l->control->desc, info->rset_r->control->desc,
                    *current, *total, r);
    }
#endif
}

static void r_get_terms(RSET ct, TERMID *terms, int maxterms, int *curterm)
{
    struct rset_between_info *info = (struct rset_between_info *) ct->priv;
    rset_getterms(info->rset_m, terms, maxterms, curterm);
}

/* Better algorithm
 * One of the major problems with rsbetween is the complexity of keeping track
 * of start tags, stop tags, hits, and attributes, together with record
 * boundaries etc. 
 *
 * Things can be divided into finding the right records, and then processing
 * hits inside the record.
 *
 * Finding the record is mostly a matter of forwarding until we have a start
 * tag and a hit in the same record.
 *
 * Handling stuff inside a record, we can simplify things by implementing a
 * glorified OR operator that returns all the occurrences in proper order,
 * together with info on what type it was. Then the main logic can just keep
 * reading, and consider each type separately:
 *   - if a start tag, increment level (some trickery with attributes!)
 *   - if a stop tag, decrement level
 *   - if a hit, and we have a level, return it
 *   - if a hit, but no level, ignore it
 *
 * The attributes can be detected when ever reading start tags. The main
 * routine needs to keep a stack of attribute match bits, so when ever we read
 * a start tag, we must report back if we have a matching attribute or not.
 *
 */
 
