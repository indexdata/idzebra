/* $Id: rsprox.c,v 1.3 2004-06-16 21:27:37 adam Exp $
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

#include <rsprox.h>
#include <zebrautl.h>

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
static int r_count (RSET ct);
static int r_read (RSFD rfd, void *buf, int *term_index);
static int r_write (RSFD rfd, const void *buf);

static const struct rset_control control_prox = 
{
    "prox",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_forward,
    r_count,
    r_read,
    r_write,
};

const struct rset_control *rset_kind_prox = &control_prox;

struct rset_prox_info {
    struct rset_prox_parms p;

    struct rset_prox_rfd *rfd_list;
};

struct rset_prox_rfd {
    RSFD *rfd;
    char **buf;  /* lookahead key buffers */
    char *more;  /* more in each lookahead? */
    struct rset_prox_rfd *next;
    struct rset_prox_info *info;
};    

static void *r_create (RSET ct, const struct rset_control *sel, void *parms)
{
    rset_prox_parms *prox_parms = (rset_prox_parms *) parms;
    struct rset_prox_info *info;
    int i;
    char prox_term[512];
    int length_prox_term = 0;
    int min_nn = 10000000;
    const char *flags = NULL;
    int term_type = 0;


    info = (struct rset_prox_info *) xmalloc (sizeof(*info));
    memcpy(&info->p, prox_parms, sizeof(struct rset_prox_parms));
    assert(info->p.rset_no >= 2);
    info->p.rset = xmalloc(info->p.rset_no * sizeof(*info->p.rset));
    memcpy(info->p.rset, prox_parms->rset,
	   info->p.rset_no * sizeof(*info->p.rset));
    info->rfd_list = NULL;

    for (i = 0; i<info->p.rset_no; i++)
	if (rset_is_volatile(info->p.rset[i]))
	    ct->flags |= RSET_FLAG_VOLATILE;

    *prox_term = '\0';
    for (i = 0; i<info->p.rset_no; i++)
    {
	int j;
	for (j = 0; j < info->p.rset[i]->no_rset_terms; j++)
	{
	    const char *nflags = info->p.rset[i]->rset_terms[j]->flags;
	    char *term = info->p.rset[i]->rset_terms[j]->name;
	    int lterm = strlen(term);
	    if (lterm + length_prox_term < sizeof(prox_term)-1)
	    {
		if (length_prox_term)
		    prox_term[length_prox_term++] = ' ';
		strcpy (prox_term + length_prox_term, term);
		length_prox_term += lterm;
	    }
	    if (min_nn > info->p.rset[i]->rset_terms[j]->nn)
		min_nn = info->p.rset[i]->rset_terms[j]->nn;
	    flags = nflags;
            term_type = info->p.rset[i]->rset_terms[j]->type;
	}
    }

    ct->no_rset_terms = 1;
    ct->rset_terms = (RSET_TERM *)
	xmalloc (sizeof (*ct->rset_terms) * ct->no_rset_terms);

    ct->rset_terms[0] = rset_term_create (prox_term, length_prox_term,
					  flags, term_type);
    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_prox_info *info = (struct rset_prox_info *) ct->buf;
    struct rset_prox_rfd *rfd;
    int i, dummy;

    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "prox set type is read-only");
	return NULL;
    }
    rfd = (struct rset_prox_rfd *) xmalloc (sizeof(*rfd));
    logf(LOG_DEBUG,"rsprox (%s) open [%p]", ct->control->desc, rfd);
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;

    rfd->more = xmalloc (sizeof(*rfd->more) * info->p.rset_no);

    rfd->buf = xmalloc(sizeof(*rfd->buf) * info->p.rset_no);
    for (i = 0; i < info->p.rset_no; i++)
	rfd->buf[i] = xmalloc (info->p.key_size);

    rfd->rfd = xmalloc(sizeof(*rfd->rfd) * info->p.rset_no);
    for (i = 0; i < info->p.rset_no; i++)
	rfd->rfd[i] = rset_open (info->p.rset[i], RSETF_READ);

    for (i = 0; i < info->p.rset_no; i++)
	rfd->more[i] = rset_read (info->p.rset[i], rfd->rfd[i],
				  rfd->buf[i], &dummy);
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
	    for (i = 0; i<info->p.rset_no; i++)
		xfree ((*rfdp)->buf[i]);
	    xfree ((*rfdp)->buf);
            xfree ((*rfdp)->more);

	    for (i = 0; i<info->p.rset_no; i++)
		rset_close (info->p.rset[i], (*rfdp)->rfd[i]);
	    xfree ((*rfdp)->rfd);

            *rfdp = (*rfdp)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_prox_info *info = (struct rset_prox_info *) ct->buf;
    int i;

    assert (info->rfd_list == NULL);
    rset_term_destroy(ct->rset_terms[0]);
    xfree (ct->rset_terms);
    for (i = 0; i<info->p.rset_no; i++)
	rset_delete (info->p.rset[i]);
    xfree (info->p.rset);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{
    struct rset_prox_info *info = ((struct rset_prox_rfd*)rfd)->info;
    struct rset_prox_rfd *p = (struct rset_prox_rfd *) rfd;
    int dummy, i;

    logf (LOG_DEBUG, "rsprox_rewind");

    for (i = 0; i < info->p.rset_no; i++)
    {
	rset_rewind (info->p.rset[i], p->rfd[i]);
	p->more[i] = rset_read (info->p.rset[i], p->rfd[i], p->buf[i], &dummy);
    }
}

static int r_forward (RSET ct, RSFD rfd, void *buf, int *term_index,
		      int (*cmpfunc)(const void *p1, const void *p2),
		      const void *untilbuf)
{
    /* Note: CT is not used. We _can_ pass NULL for it */
    struct rset_prox_info *info = ((struct rset_prox_rfd*)rfd)->info;
    struct rset_prox_rfd *p = (struct rset_prox_rfd *) rfd;
    int cmp=0;
    int i;
    int dummy;

    if (untilbuf)
    {
	/* it's enough to forward first one. Other will follow
	   automatically */
	if ( p->more[0] && ((cmpfunc)(untilbuf, p->buf[0]) >= 2) )
	    p->more[0] = rset_forward(info->p.rset[0], p->rfd[0],
				      p->buf[0], &dummy, info->p.cmp,
				      untilbuf);
    }
    if (info->p.ordered && info->p.relation == 3 && info->p.exclusion == 0
	&& info->p.distance == 1)
    {
	while (p->more[0]) 
	{
	    for (i = 1; i < info->p.rset_no; i++)
	    {
		if (!p->more[i]) 
		{
		    p->more[0] = 0;    /* saves us a goto out of while loop. */
		    break;
		}
		cmp = (*info->p.cmp) (p->buf[i], p->buf[i-1]);
		if (cmp > 1)
		{
		    p->more[i-1] = rset_forward (info->p.rset[i-1],
						 p->rfd[i-1],
						 p->buf[i-1], &dummy,
						 info->p.cmp,
						 p->buf[i]);
		    break;
		}
		else if (cmp == 1)
		{
		    if ((*info->p.getseq)(p->buf[i-1]) +1 != 
			(*info->p.getseq)(p->buf[i]))
		    {
			p->more[i-1] = rset_read (
			    info->p.rset[i-1], p->rfd[i-1],
			    p->buf[i-1], &dummy);
			break;
		    }
		}
		else
		{
		    p->more[i] = rset_forward (info->p.rset[i], p->rfd[i],
					       p->buf[i], &dummy,
					       info->p.cmp,
					       p->buf[i-1]);
		    break;
		}
	    }
	    if (i == p->info->p.rset_no)
	    {
		memcpy (buf, p->buf[0], info->p.key_size);
		*term_index = 0;
		
		p->more[0] = rset_read (info->p.rset[0], p->rfd[0],
					p->buf[0], &dummy);
		return 1;
	    }
	}
    }
    else if (info->p.rset_no == 2)
    {
	while (p->more[0] && p->more[1]) 
	{
	    int cmp = (*info->p.cmp)(p->buf[0], p->buf[1]);
	    if (cmp < -1)
		p->more[0] = rset_forward (info->p.rset[0], p->rfd[0],
					   p->buf[0],
					   term_index, info->p.cmp, p->buf[0]);
	    else if (cmp > 1)
		p->more[1] = rset_forward (info->p.rset[1], p->rfd[1],
					   p->buf[1],
					   term_index, info->p.cmp, p->buf[1]);
	    else
	    {
		int seqno[500];
		int n = 0;
		
		seqno[n++] = (*info->p.getseq)(p->buf[0]);
		while ((p->more[0] = rset_read (info->p.rset[0], p->rfd[0],
						p->buf[0],
						term_index)) >= -1 &&
		       p->more[0] <= -1)
		    if (n < 500)
			seqno[n++] = (*info->p.getseq)(p->buf[0]);
		
		for (i = 0; i<n; i++)
		{
		    int diff = (*info->p.getseq)(p->buf[1]) - seqno[i];
		    int excl = info->p.exclusion;
		    if (!info->p.ordered && diff < 0)
			diff = -diff;
		    switch (info->p.relation)
		    {
		    case 1:      /* < */
			if (diff < info->p.distance && diff >= 0)
			    excl = !excl;
			break;
		    case 2:      /* <= */
			if (diff <= info->p.distance && diff >= 0)
			    excl = !excl;
			break;
		    case 3:      /* == */
			if (diff == info->p.distance && diff >= 0)
			    excl = !excl;
			break;
		    case 4:      /* >= */
			if (diff >= info->p.distance && diff >= 0)
			    excl = !excl;
			break;
		    case 5:      /* > */
			if (diff > info->p.distance && diff >= 0)
			    excl = !excl;
			break;
		    case 6:      /* != */
			if (diff != info->p.distance && diff >= 0)
			    excl = !excl;
			break;
		    }
		    if (excl)
		    {
			memcpy (buf, p->buf[1], info->p.key_size);
			*term_index = 0;
			
			p->more[1] = rset_read (info->p.rset[1],
						p->rfd[1], p->buf[1],
						term_index);
			return 1;
		    }
		}
		p->more[1] = rset_read (info->p.rset[1], p->rfd[1],
					p->buf[1],
					term_index);
	    }
	}
    }
    return 0;
}

static int r_count (RSET ct)
{
    return 0;
}

static int r_read (RSFD rfd, void *buf, int *term_index)
{
    return r_forward(0, rfd, buf, term_index, 0, 0);
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "prox set type is read-only");
    return -1;
}

