/*
 * Copyright (C) 1998-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rank1.c,v $
 * Revision 1.9  2002-04-11 11:39:59  heikki
 * Removed to logf calls from tight inside loops
 *
 * Revision 1.8  2002/04/04 14:14:13  adam
 * Multiple registers (alpha early)
 *
 * Revision 1.7  2001/11/14 22:06:27  adam
 * Rank-weight may be controlled via query.
 *
 * Revision 1.6  2000/03/15 15:00:30  adam
 * First work on threaded version.
 *
 * Revision 1.5  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.4  1999/02/02 14:51:01  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.3  1998/06/12 12:21:53  adam
 * Fixed memory-leak.
 *
 * Revision 1.2  1998/03/05 13:03:29  adam
 * Improved ranking.
 *
 * Revision 1.1  1998/03/05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 */

#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "index.h"

struct rank_class_info {
    int dummy;
};

struct rank_term_info {
    int local_occur;
    int global_occur;
    int global_inv;
    int rank_flag;
    int rank_weight;
};

struct rank_set_info {
    int last_pos;
    int no_entries;
    int no_rank_entries;
    struct rank_term_info *entries;
};

static int log2_int (unsigned g)
{
    int n = 0;
    while ((g = g>>1))
	n++;
    return n;
}

/*
 * create: Creates/Initialises this rank handler. This routine is 
 *  called exactly once. The routine returns the class_handle.
 */
static void *create (struct zebra_register *reg)
{
    struct rank_class_info *ci = (struct rank_class_info *)
	xmalloc (sizeof(*ci));

    logf (LOG_DEBUG, "rank-1 create");
    return ci;
}

/*
 * destroy: Destroys this rank handler. This routine is called
 *  when the handler is no longer needed - i.e. when the server
 *  dies. The class_handle was previously returned by create.
 */
static void destroy (struct zebra_register *reg, void *class_handle)
{
    struct rank_class_info *ci = (struct rank_class_info *) class_handle;

    logf (LOG_DEBUG, "rank-1 destroy");
    xfree (ci);
}


/*
 * begin: Prepares beginning of "real" ranking. Called once for
 *  each result set. The returned handle is a "set handle" and
 *  will be used in each of the handlers below.
 */
static void *begin (struct zebra_register *reg, void *class_handle, RSET rset)
{
    struct rank_set_info *si = (struct rank_set_info *) xmalloc (sizeof(*si));
    int i;

    logf (LOG_DEBUG, "rank-1 begin");
    si->no_entries = rset->no_rset_terms;
    si->no_rank_entries = 0;
    si->entries = (struct rank_term_info *)
	xmalloc (sizeof(*si->entries)*si->no_entries);
    for (i = 0; i < si->no_entries; i++)
    {
	int g = rset->rset_terms[i]->nn;
	if (!strncmp (rset->rset_terms[i]->flags, "rank,", 5))
	{
            yaz_log (LOG_LOG, "%s", rset->rset_terms[i]->flags);
	    si->entries[i].rank_flag = 1;
            si->entries[i].rank_weight = atoi (rset->rset_terms[i]->flags+5);
            yaz_log (LOG_LOG, "i=%d weight=%d", i, si->entries[i].rank_weight);
	    (si->no_rank_entries)++;
	}
	else
	    si->entries[i].rank_flag = 0;
	si->entries[i].local_occur = 0;
	si->entries[i].global_occur = g;
	si->entries[i].global_inv = 32 - log2_int (g);
	logf (LOG_DEBUG, "-------- %d ------", 32 - log2_int (g));
    }
    return si;
}

/*
 * end: Terminates ranking process. Called after a result set
 *  has been ranked.
 */
static void end (struct zebra_register *reg, void *set_handle)
{
    struct rank_set_info *si = (struct rank_set_info *) set_handle;
    logf (LOG_DEBUG, "rank-1 end");
    xfree (si->entries);
    xfree (si);
}

/*
 * add: Called for each word occurence in a result set. This routine
 *  should be as fast as possible. This routine should "incrementally"
 *  update the score.
 */
static void add (void *set_handle, int seqno, int term_index)
{
    struct rank_set_info *si = (struct rank_set_info *) set_handle;
    /*logf (LOG_DEBUG, "rank-1 add seqno=%d term_index=%d", seqno, term_index);*/
    si->last_pos = seqno;
    si->entries[term_index].local_occur++;
}

/*
 * calc: Called for each document in a result. This handler should 
 *  produce a score based on previous call(s) to the add handler. The
 *  score should be between 0 and 1000. If score cannot be obtained
 *  -1 should be returned.
 */
static int calc (void *set_handle, int sysno)
{
    int i, lo, divisor, score = 0;
    struct rank_set_info *si = (struct rank_set_info *) set_handle;

    if (!si->no_rank_entries)
	return -1;

    for (i = 0; i < si->no_entries; i++)
	if (si->entries[i].rank_flag && (lo = si->entries[i].local_occur))
	    score += (8+log2_int (lo)) * si->entries[i].global_inv *
                si->entries[i].rank_weight;
    divisor = si->no_rank_entries * (8+log2_int (si->last_pos/si->no_entries));
    score = score / divisor;
    yaz_log (LOG_LOG, "sysno=%d score=%d", sysno, score);
    if (score > 1000)
	score = 1000;
    for (i = 0; i < si->no_entries; i++)
	si->entries[i].local_occur = 0;
    return score;
}

/*
 * Pseudo-meta code with sequence of calls as they occur in a
 * server. Handlers are prefixed by --:
 *
 *     server init
 *     -- create
 *     foreach search
 *        rank result set
 *        -- begin
 *        foreach record
 *           foreach word
 *              -- add
 *           -- calc
 *        -- end
 *     -- destroy
 *     server close
 */

static struct rank_control rank_control = {
    "rank-1",
    create,
    destroy,
    begin,
    end,
    calc,
    add,
};
 
struct rank_control *rank1_class = &rank_control;
