/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsbool.c,v $
 * Revision 1.16  1999-05-26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.15  1999/02/02 14:51:32  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.14  1998/03/05 08:36:27  adam
 * New result set model.
 *
 * Revision 1.13  1997/12/18 10:54:24  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.12  1997/10/31 12:37:01  adam
 * Code calls xfree() instead of free().
 *
 * Revision 1.11  1997/09/09 13:38:15  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.10  1996/10/29 13:55:20  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.9  1995/12/11 09:15:22  adam
 * New set types: sand/sor/snot - ranked versions of and/or/not in
 * ranked/semi-ranked result sets.
 * Note: the snot not finished yet.
 * New rset member: flag.
 * Bug fix: r_delete in rsrel.c did free bad memory block.
 *
 * Revision 1.8  1995/10/12  12:41:55  adam
 * Private info (buf) moved from struct rset_control to struct rset.
 * Bug fixes in relevance.
 *
 * Revision 1.7  1995/10/10  14:00:03  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.6  1995/10/06  14:38:05  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.5  1995/09/08  14:52:41  adam
 * Work on relevance feedback.
 *
 * Revision 1.4  1995/09/08  08:54:04  adam
 * More efficient and operation.
 *
 * Revision 1.3  1995/09/07  13:58:43  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.2  1995/09/06  16:11:55  adam
 * More work on boolean sets.
 *
 * Revision 1.1  1995/09/06  13:27:15  adam
 * New set type: bool. Not finished yet.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <rsbool.h>
#include <zebrautl.h>

static void *r_create(RSET ct, const struct rset_control *sel, void *parms);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_count (RSET ct);
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
    r_count,
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
    r_count,
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
    r_count,
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
    struct rset_bool_rfd *rfd_list;
};

struct rset_bool_rfd {
    RSFD rfd_l;
    RSFD rfd_r;
    int  more_l;
    int  more_r;
    int term_index_l;
    int term_index_r;
    void *buf_l;
    void *buf_r;
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
    rfd->next = info->rfd_list;
    info->rfd_list = rfd;
    rfd->info = info;

    rfd->buf_l = xmalloc (info->key_size);
    rfd->buf_r = xmalloc (info->key_size);
    rfd->rfd_l = rset_open (info->rset_l, RSETF_READ);
    rfd->rfd_r = rset_open (info->rset_r, RSETF_READ);
    rfd->more_l = rset_read (info->rset_l, rfd->rfd_l, rfd->buf_l,
			     &rfd->term_index_l);
    rfd->more_r = rset_read (info->rset_r, rfd->rfd_r, rfd->buf_r,
			     &rfd->term_index_r);
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
}

static int r_count (RSET ct)
{
    return 0;
}

static int r_read_and (RSFD rfd, void *buf, int *term_index)
{
    struct rset_bool_rfd *p = (struct rset_bool_rfd *) rfd;
    struct rset_bool_info *info = p->info;

    while (p->more_l && p->more_r)
    {
        int cmp;

        cmp = (*info->cmp)(p->buf_l, p->buf_r);
        if (!cmp)
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
            return 1;
        }
        else if (cmp == 1)
        {
            memcpy (buf, p->buf_r, info->key_size);

	    *term_index = p->term_index_r + info->term_index_s;
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
            return 1;
        }
        else if (cmp == -1)
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
            return 1;
        }
        else if (cmp > 1)
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
        else
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
    }
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
            return 1;
        }
        else if (cmp > 0)
        {
            memcpy (buf, p->buf_r, info->key_size);
	    *term_index = p->term_index_r + info->term_index_s;
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
            return 1;
        }
        else
        {
            memcpy (buf, p->buf_l, info->key_size);
	    *term_index = p->term_index_l;
            p->more_l = rset_read (info->rset_l, p->rfd_l, p->buf_l,
				   &p->term_index_l);
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
            return 1;
        }
        else if (cmp > 1)
            p->more_r = rset_read (info->rset_r, p->rfd_r, p->buf_r,
				   &p->term_index_r);
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

