/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Heikki Levanto
 *
 * $Id: rsbetween.c,v 1.6 2002-08-01 08:53:35 adam Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <rsbetween.h>
#include <zebrautl.h>

static void *r_create_between(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open_between (RSET ct, int flag);
static void r_close_between (RSFD rfd);
static void r_delete_between (RSET ct);
static void r_rewind_between (RSFD rfd);
static int r_count_between (RSET ct);
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
    r_count_between,
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

    logf (LOG_DEBUG, "rsbetween_rewind");
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

static int r_count_between (RSET ct)
{
    return 0;
}

static void logit( struct rset_between_info *info, char *prefix, void *l, void *m, void *r)
{
    char buf_l[32];
    char buf_m[32];
    char buf_r[32];
    logf(LOG_DEBUG,"btw: %s l=%s m=%s r=%s",
      prefix, 
      (*info->printer)(l, buf_l),
      (*info->printer)(m, buf_m),
      (*info->printer)(r, buf_r) );
}

static int r_read_between (RSFD rfd, void *buf, int *term_index)
{
    struct rset_between_rfd *p = (struct rset_between_rfd *) rfd;
    struct rset_between_info *info = p->info;
    int cmp_l;
    int cmp_r;
    int attr_match;

    while (p->more_m)
    {
        logit( info, "start of loop", p->buf_l, p->buf_m, p->buf_r);

	/* forward L until past m, count levels, note rec boundaries */
	if (p->more_l)
	    cmp_l= (*info->cmp)(p->buf_l, p->buf_m);
	else
	    cmp_l=2; /* past this record */
        logf(LOG_DEBUG, "cmp_l=%d", cmp_l);

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
                        p->more_attr = rset_read (info->rset_attr, p->rfd_attr,
                                                  p->buf_attr, &dummy_term);
                    }
                }
            }
            if (p->more_l)
            {
                p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
		    		   &p->term_index_l);
	        cmp_l= (*info->cmp)(p->buf_l, p->buf_m);
                logit( info, "forwarded L", p->buf_l, p->buf_m, p->buf_r);
                logf(LOG_DEBUG, "  cmp_l=%d", cmp_l);
            }
            else
		cmp_l=2; 
        } /* forward L */

            
	/* forward R until past m, count levels */
        if (p->more_r)
	    cmp_r= (*info->cmp)(p->buf_r, p->buf_m);
	else
	    cmp_r=2; 
        logf(LOG_DEBUG, "cmp_r=%d", cmp_r);
        while (cmp_r < 0)   /* r before m */
	{
 	    /* -2, earlier record, doesn't matter */
            if (cmp_r == -1)
		p->level--; /* relevant end tag */
            if (p->more_r)
            {
                p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
		    		   &p->term_index_r);
	        cmp_r= (*info->cmp)(p->buf_r, p->buf_m);
                logit( info, "forwarded R", p->buf_l, p->buf_m, p->buf_r);
                logf(LOG_DEBUG, "  cmp_r=%d", cmp_r);
            }
            else
		cmp_r=2; 
        } /* forward R */
	
	if ( ( p->level <= 0 ) && ! p->more_l)
	    return 0; /* no more start tags, nothing more to find */
        
	if ( attr_match && p->level > 0)  /* within a tag pair (or deeper) */
	{
	    memcpy (buf, p->buf_m, info->key_size);
            *term_index = p->term_index_m;
            logit( info, "Returning a hit (m)", p->buf_l, p->buf_m, p->buf_r);
            p->more_m = rset_read (info->rset_m, p->rfd_m, p->buf_m,
                                   &p->term_index_m);
	    return 1;
	}
	else
	    if ( ! p->more_l )  /* not in data, no more starts */
		return 0;  /* ergo, nothing can be found. stop scanning */
        
        p->more_m = rset_read (info->rset_m, p->rfd_m, p->buf_m,
                               &p->term_index_m);
    } /* while more_m */
      
    logf(LOG_DEBUG,"Exiting, no more stuff in m");
    return 0;  /* no more data possible */


}  /* r_read */


static int r_write_between (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "between set type is read-only");
    return -1;
}

