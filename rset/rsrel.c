/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsrel.c,v $
 * Revision 1.22  1997-12-18 10:54:25  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.21  1997/11/18 10:05:08  adam
 * Changed character map facility so that admin can specify character
 * mapping files for each register type, w, p, etc.
 *
 * Revision 1.20  1997/10/31 12:37:55  adam
 * Code calls xfree() instead of free().
 *
 * Revision 1.19  1997/10/01 11:44:06  adam
 * Small improvement of new ranking.
 *
 * Revision 1.18  1997/09/24 13:36:41  adam
 * More work on new ranking algorithm.
 *
 * Revision 1.17  1997/09/22 12:39:07  adam
 * Added get_pos method for the ranked result sets.
 *
 * Revision 1.16  1997/09/17 12:19:23  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.15  1997/09/09 13:38:16  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.14  1996/11/08 11:15:58  adam
 * Compressed isam fully supported.
 *
 * Revision 1.13  1996/10/29 13:55:26  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.12  1996/10/08 13:00:40  adam
 * Bug fix: result sets with ranked operands in boolean operations weren't
 * sorted.
 *
 * Revision 1.11  1996/10/07  16:05:29  quinn
 * Work.
 *
 * Revision 1.9  1995/12/11  09:15:26  adam
 * New set types: sand/sor/snot - ranked versions of and/or/not in
 * ranked/semi-ranked result sets.
 * Note: the snot not finished yet.
 * New rset member: flag.
 * Bug fix: r_delete in rsrel.c did free bad memory block.
 *
 * Revision 1.8  1995/12/05  11:25:45  adam
 * Doesn't include math.h.
 *
 * Revision 1.7  1995/10/12  12:41:57  adam
 * Private info (buf) moved from struct rset_control to struct rset.
 * Bug fixes in relevance.
 *
 * Revision 1.6  1995/10/10  14:00:04  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.5  1995/10/06  14:38:06  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.4  1995/09/14  07:48:56  adam
 * Other score calculation.
 *
 * Revision 1.3  1995/09/11  15:23:40  adam
 * More work on relevance search.
 *
 * Revision 1.2  1995/09/11  13:09:41  adam
 * More work on relevance feedback.
 *
 * Revision 1.1  1995/09/08  14:52:42  adam
 * Work on relevance feedback.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <isam.h>
#include <isamc.h>
#include <rsrel.h>
#include <zebrautl.h>

static void *r_create(const struct rset_control *sel, void *parms,
                      int *flags);
static RSFD r_open (RSET ct, int flag);
static void r_close (RSFD rfd);
static void r_delete (RSET ct);
static void r_rewind (RSFD rfd);
static int r_count (RSET ct);
static int r_hits (RSET ct, void *oi);
static int r_read (RSFD rfd, void *buf);
static int r_write (RSFD rfd, const void *buf);
static int r_score (RSFD rfd, int *score);

static const rset_control control = 
{
    "relevance",
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_hits,
    r_read,
    r_write,
    r_score
};

const rset_control *rset_kind_relevance = &control;

struct rset_rel_info {
    int     key_size;
    int     max_rec;
    int     no_rec;
    int     hits;                       /* hits count */
    int     (*cmp)(const void *p1, const void *p2);
    int     (*get_pos)(const void *p);
    char    *key_buf;                   /* key buffer */
    float   *score_buf;                 /* score buffer */
    int     *sort_idx;                  /* score sorted index */
    int     *sysno_idx;                 /* sysno sorted index (ring buffer) */
    struct rset_rel_rfd *rfd_list;
};

struct rset_rel_rfd {
    int     last_read_pos;
    int     position;
    int     flag;
    struct rset_rel_rfd *next;
    struct rset_rel_info *info;
};

static void add_rec (struct rset_rel_info *info, double score, void *key)
{
    int idx, i, j;

    (info->hits)++;
    for (i = 0; i<info->no_rec; i++)
    {
        idx = info->sort_idx[i];
        if (score <= info->score_buf[idx])
            break;
    }
    if (info->no_rec < info->max_rec)
    {                                        /* there is room for this entry */
        for (j = info->no_rec; j > i; --j)
            info->sort_idx[j] = info->sort_idx[j-1];
        idx = info->sort_idx[j] = info->no_rec;
        ++(info->no_rec);
    }
    else if (i == 0)
        return;                              /* score too low */
    else
    {
        idx = info->sort_idx[0];             /* remove this entry */

        --i;
        for (j = 0; j < i; ++j)              /* make room */
            info->sort_idx[j] = info->sort_idx[j+1];
        info->sort_idx[j] = idx;             /* allocate sort entry */
    }
    memcpy (info->key_buf + idx*info->key_size, key, info->key_size);
    info->score_buf[idx] = score;
}


static struct rset_rel_info *qsort_info;

static int qcomp (const void *p1, const void *p2)
{
    int i1 = *(int*) p1;
    int i2 = *(int*) p2;

    return qsort_info->cmp (qsort_info->key_buf + i1*qsort_info->key_size,
                            qsort_info->key_buf + i2*qsort_info->key_size);
}

#define NEW_RANKING 0

#define SCORE_SHOW 0.0                       /* base score for showing up */
#define SCORE_COOC 0.3                       /* component dependent on co-oc */
#define SCORE_DYN  (1-(SCORE_SHOW+SCORE_COOC)) /* dynamic component of score */

static void relevance (struct rset_rel_info *info, rset_relevance_parms *parms)
{
    char **isam_buf;
    char *isam_tmp_buf;
    int  *isam_r;
    int  *max_tf, *tf;

    int  *pos_tf = NULL;
    int score_sum = 0;
    int no_occur = 0;
    char *isam_prev_buf = NULL;
    int fact1, fact2;

    ISPT *isam_pt = NULL;
    ISAMC_PP *isamc_pp = NULL;
    int i;

    logf (LOG_DEBUG, "relevance");
    isam_buf = xmalloc (parms->no_isam_positions * sizeof(*isam_buf));
    isam_r = xmalloc (sizeof (*isam_r) * parms->no_isam_positions);
    if (parms->is)
        isam_pt = xmalloc (sizeof (*isam_pt) * parms->no_isam_positions);
    else if (parms->isc)
        isamc_pp = xmalloc (sizeof (*isamc_pp) * parms->no_isam_positions);
    else
    {
        logf (LOG_FATAL, "No isamc or isam in rs_rel");
        abort ();
    }
    isam_tmp_buf = xmalloc (info->key_size);
    max_tf = xmalloc (sizeof (*max_tf) * parms->no_terms);
    tf = xmalloc (sizeof (*tf) * parms->no_terms);

    for (i = 0; i<parms->no_terms; i++)
	max_tf[i] = 0;
    for (i = 0; i<parms->no_isam_positions; i++)
    {
        isam_buf[i] = xmalloc (info->key_size);
        if (isam_pt)
        {
            isam_pt[i] = is_position (parms->is, parms->isam_positions[i]);
            max_tf [parms->term_no[i]] = is_numkeys (isam_pt[i]);
            isam_r[i] = is_readkey (isam_pt[i], isam_buf[i]);
        } 
        else if (isamc_pp)
        {
            isamc_pp[i] = isc_pp_open (parms->isc, parms->isam_positions[i]);
            max_tf [parms->term_no[i]] = isc_pp_num (isamc_pp[i]);
            isam_r[i] = isc_pp_read (isamc_pp[i], isam_buf[i]);
        }
        logf (LOG_DEBUG, "max tf %d = %d", i, max_tf[i]);
    }
    switch (parms->method)
    {
    case RSREL_METHOD_B:
	while (1)
	{
	    int r, min = -1;
	    int pos = 0;
	    for (i = 0; i<parms->no_isam_positions; i++)
		if (isam_r[i] && 
		    (min < 0 ||
		     (r = (*parms->cmp)(isam_buf[i], isam_buf[min])) < 1))
		    min = i;
	    if (!isam_prev_buf)
	    {
		pos_tf = xmalloc (sizeof(*pos_tf) * parms->no_isam_positions);
		isam_prev_buf = xmalloc (info->key_size);
		fact1 = 100000/parms->no_isam_positions;
		fact2 = 100000/
		    (parms->no_isam_positions*parms->no_isam_positions);
		
		no_occur = score_sum = 0;
		memcpy (isam_prev_buf, isam_buf[min], info->key_size);
		for (i = 0; i<parms->no_isam_positions; i++)
		    pos_tf[i] = -10;
	    }
	    else if (min < 0 ||
		     (*parms->cmp)(isam_buf[min], isam_prev_buf) > 1)
	    {
		logf (LOG_LOG, "final occur = %d ratio=%d",
		      no_occur, score_sum / no_occur);
		add_rec (info, score_sum / (10000.0*no_occur), isam_prev_buf);
		if (min < 0)
		    break;
		no_occur = score_sum = 0;
		memcpy (isam_prev_buf, isam_buf[min], info->key_size);
		for (i = 0; i<parms->no_isam_positions; i++)
		    pos_tf[i] = -10;
	    }
	    pos = (*parms->get_pos)(isam_buf[min]);
	    logf (LOG_LOG, "pos=%d", pos);
	    for (i = 0; i<parms->no_isam_positions; i++)
	    {
		int d = pos - pos_tf[i];
		
		no_occur++;
		if (pos_tf[i] < 0 && i != min)
		    continue;
		if (d < 10)
		    d = 10;
		if (i == min)
		    score_sum += fact2 / d;
		else
		    score_sum += fact1 / d;
	    }
	    pos_tf[min] = pos;
	    logf (LOG_LOG, "score_sum = %d", score_sum);
	    i = min;
	    if (isam_pt)
		isam_r[i] = is_readkey (isam_pt[i], isam_buf[i]);
	    else if (isamc_pp)
		isam_r[i] = isc_pp_read (isamc_pp[i], isam_buf[i]);
	} /* while */
	xfree (isam_prev_buf);
	xfree (pos_tf);
	break;
    case RSREL_METHOD_A:
	while (1)
	{
	    int min = -1, i, r;
	    double score;
	    int co_oc, last_term;    /* Number of co-occurrences */
	    
	    last_term = -1;
	    /* find min with lowest sysno */
	    for (i = 0; i<parms->no_isam_positions; i++)
	    {
		if (isam_r[i] && 
		    (min < 0
		     || (r = (*parms->cmp)(isam_buf[i], isam_buf[min])) < 2))
		{
		    min = i;
		    co_oc = 1;
		}
		else if (!r && last_term != parms->term_no[i]) 
		    co_oc++;  /* new occurrence */
		last_term = parms->term_no[i];
	    }
	    
	    if (min < 0)
		break;
	    memcpy (isam_tmp_buf, isam_buf[min], info->key_size);
	    /* calculate for all with those sysno */
	    for (i = 0; i < parms->no_terms; i++)
		tf[i] = 0;
	    for (i = 0; i<parms->no_isam_positions; i++)
	    {
		int r;
		
		if (isam_r[i])
		    r = (*parms->cmp)(isam_buf[i], isam_tmp_buf);
		else 
		    r = 2;
		if (r <= 1 && r >= -1)
		{
		    do
		    {
			tf[parms->term_no[i]]++;
			if (isam_pt)
			    isam_r[i] = is_readkey (isam_pt[i], isam_buf[i]);
			else if (isamc_pp)
			    isam_r[i] = isc_pp_read (isamc_pp[i], isam_buf[i]);
		    } while (isam_r[i] && 
			     (*parms->cmp)(isam_buf[i], isam_tmp_buf) <= 1);
		}
	    }
	    /* calculate relevance value */
	    score = 0.0;
	    for (i = 0; i<parms->no_terms; i++)
		if (tf[i])
		    score += SCORE_SHOW + SCORE_COOC*co_oc/parms->no_terms +
			SCORE_DYN*tf[i]/max_tf[i];
	    /* if value is in the top score, then save it - don't emit yet */
	    add_rec (info, score/parms->no_terms, isam_tmp_buf);
	} /* while */
	break;
    } /* switch */
    for (i = 0; i<info->no_rec; i++)
        info->sysno_idx[i] = i;
    qsort_info = info;
    qsort (info->sysno_idx, info->no_rec, sizeof(*info->sysno_idx), qcomp);
    for (i = 0; i<parms->no_isam_positions; i++)
    {
        if (isam_pt)
            is_pt_free (isam_pt[i]);
        if (isamc_pp)
            isc_pp_close (isamc_pp[i]);
        xfree (isam_buf[i]);
    }
    xfree (max_tf);
    xfree (isam_tmp_buf);
    xfree (isam_buf);
    xfree (isam_r);
    xfree (isam_pt);
    xfree (isamc_pp);
    xfree(tf);
}

static void *r_create (const struct rset_control *sel, void *parms,
                       int *flags)
{
    rset_relevance_parms *r_parms = parms;
    struct rset_rel_info *info;

    *flags |= RSET_FLAG_RANKED;
    info = xmalloc (sizeof(struct rset_rel_info));
    info->key_size = r_parms->key_size;
    assert (info->key_size > 1);
    info->max_rec = r_parms->max_rec;
    assert (info->max_rec > 1);
    info->cmp = r_parms->cmp;
    info->get_pos = r_parms->get_pos;

    info->key_buf = xmalloc (info->key_size * info->max_rec);
    info->score_buf = xmalloc (sizeof(*info->score_buf) * info->max_rec);
    info->sort_idx = xmalloc (sizeof(*info->sort_idx) * info->max_rec);
    info->sysno_idx = xmalloc (sizeof(*info->sysno_idx) * info->max_rec);
    info->no_rec = 0;
    info->hits = 0;
    info->rfd_list = NULL;

    relevance (info, r_parms);
    return info;
}

static RSFD r_open (RSET ct, int flag)
{
    struct rset_rel_rfd *rfd;
    struct rset_rel_info *info = ct->buf;

    if (flag & RSETF_WRITE)
    {
	logf (LOG_FATAL, "relevance set type is read-only");
	return NULL;
    }
    rfd = xmalloc (sizeof(*rfd));
    rfd->flag = flag;
    rfd->next = info->rfd_list;
    rfd->info = info;
    info->rfd_list = rfd;
    r_rewind (rfd);
    return rfd;
}

static void r_close (RSFD rfd)
{
    struct rset_rel_info *info = ((struct rset_rel_rfd*)rfd)->info;
    struct rset_rel_rfd **rfdp;
    
    for (rfdp = &info->rfd_list; *rfdp; rfdp = &(*rfdp)->next)
        if (*rfdp == rfd)
        {
            *rfdp = (*rfdp)->next;
            xfree (rfd);
            return;
        }
    logf (LOG_FATAL, "r_close but no rfd match!");
    assert (0);
}

static void r_delete (RSET ct)
{
    struct rset_rel_info *info = ct->buf;

    assert (info->rfd_list == NULL);
    xfree (info->key_buf);
    xfree (info->score_buf);
    xfree (info->sort_idx);
    xfree (info->sysno_idx);
    xfree (info);
}

static void r_rewind (RSFD rfd)
{
    struct rset_rel_rfd *p = rfd;
    struct rset_rel_info *info = p->info;

    if (p->flag & RSETF_SORT_RANK)
        p->position = info->no_rec;
    else
        p->position = 0;
}

static int r_count (RSET ct)
{
    struct rset_rel_info *info = ct->buf;

    return info->no_rec;
}

static int r_hits (RSET ct, void *oi)
{
    struct rset_rel_info *info = ct->buf;

    return info->hits;
}

static int r_read (RSFD rfd, void *buf)
{
    struct rset_rel_rfd *p = rfd;
    struct rset_rel_info *info = p->info;

    if (p->flag & RSETF_SORT_RANK)
    {
        if (p->position <= 0)
            return 0;
        --(p->position);
        p->last_read_pos = info->sort_idx[p->position];
    }
    else
    {
        if (p->position == info->no_rec)
            return 0;
        p->last_read_pos = info->sysno_idx[p->position];
        ++(p->position);
    }
    memcpy ((char*) buf,
            info->key_buf + info->key_size * p->last_read_pos,
            info->key_size);
    return 1;
}

static int r_score (RSFD rfd, int *score)
{
    struct rset_rel_rfd *p = rfd;
    struct rset_rel_info *info = p->info;

    *score = (int) (1000*info->score_buf[p->last_read_pos]);
    return 1;
}

static int r_write (RSFD rfd, const void *buf)
{
    logf (LOG_FATAL, "relevance set type is read-only");
    return -1;
}
