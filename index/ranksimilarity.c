/* $Id: ranksimilarity.c,v 1.1 2006-05-03 09:31:26 marc Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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
#include <assert.h>
#include <limits.h>
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "index.h"
#include "rank.h"

static int log_level = 0;
static int log_initialized = 0;

struct ranksimilarity_class_info {
    int dummy;
};

struct ranksimilarity_term_info {
    int local_occur;
    zint global_occur;
    int global_inv;
    int rank_flag;
    int rank_weight;
    TERMID term;
    int term_index;
};

struct ranksimilarity_set_info {
    int last_pos;
    int no_entries;
    int no_rank_entries;
    struct ranksimilarity_term_info *entries;
    NMEM nmem;
};


/*
 * create: Creates/Initialises this rank handler. This routine is 
 *  called exactly once. The routine returns the class_handle.
 */
static void *create (ZebraHandle zh)
{
    struct ranksimilarity_class_info *ci = 
        (struct ranksimilarity_class_info *) xmalloc (sizeof(*ci));

    if (!log_initialized)
    {
        log_level = yaz_log_module_level("ranksimilarity");
        log_initialized = 1;
    }
    yaz_log(log_level, "create()");
    return 0;
}

/*
 * destroy: Destroys this rank handler. This routine is called
 *  when the handler is no longer needed - i.e. when the server
 *  dies. The class_handle was previously returned by create.
 */
static void destroy (struct zebra_register *reg, void *class_handle)
{
    struct ranksimilarity_class_info *ci 
      = (struct ranksimilarity_class_info *) class_handle;
    yaz_log(log_level, "destroy()");
    xfree (ci);
}


/**
 * begin: Prepares beginning of "real" ranking. Called once for
 *  each result set. The returned handle is a "set handle" and
 *  will be used in each of the handlers below.
 */
static void *begin (struct zebra_register *reg, 
                    void *class_handle, RSET rset, NMEM nmem,
                    TERMID *terms, int numterms)
{
    struct ranksimilarity_set_info *si = 
        (struct ranksimilarity_set_info *) nmem_malloc (nmem, sizeof(*si));
    int i;

    yaz_log(log_level, "begin()");

    /* count how many terms are ranked (2=102 or similar) */
    si->no_entries = numterms;
    si->no_rank_entries = 0;
    si->nmem=nmem;
    si->entries = (struct ranksimilarity_term_info *)
	nmem_malloc (si->nmem, sizeof(*si->entries)*numterms); 

    /* looping all terms in a specific field of query */
    for (i = 0; i < numterms; i++)
    {
	struct ord_list *ol = terms[i]->ol;

        yaz_log(log_level, "begin() term i=%d flags=%s '%s'", i, 
                terms[i]->flags, terms[i]->name );

	for (; ol; ol = ol->next)
	{
	    int index_type = 0;
	    const char *db = 0;
	    const char *string_index = 0;
	    int set = -1;
	    int use = -1;

	    zebraExplain_lookup_ord(reg->zei,
				    ol->ord, &index_type, &db, &set, &use,
				    &string_index);

	    if (string_index)
		yaz_log(log_level, "begin() ord=%d index_type=%c db=%s str-index=%s",
		    ol->ord, index_type, db, string_index);
	    else
		yaz_log(log_level, "begin() ord=%d index_type=%c db=%s set=%d use=%d",
		    ol->ord, index_type, db, set, use);
	}
	if (!strncmp (terms[i]->flags, "rank,", 5)) 
	    (si->no_rank_entries)++;

        /* setting next entry in term */
        terms[i]->rankpriv = &(si->entries[i]);
    }
    return si;
}

/*
 * end: Terminates ranking process. Called after a result set
 *  has been ranked.
 */
static void end (struct zebra_register *reg, void *set_handle)
{
    yaz_log(log_level, "end()");
}


/**
 * add: Called for each word occurence in a result set. This routine
 *  should be as fast as possible. This routine should "incrementally"
 *  update the score.
 */
static void add (void *set_handle, int seqno, TERMID term)
{
  struct ranksimilarity_set_info *si = (struct ranksimilarity_set_info *) set_handle;
  struct ranksimilarity_term_info *ti; 
    assert(si);
    if (!term)
    {
      /* yaz_log(log_level, "add() NULL term"); */
        return;
    }

    
    ti= (struct ranksimilarity_term_info *) term->rankpriv;
    assert(ti);
    si->last_pos = seqno;
    ti->local_occur++;
    /* yaz_log(log_level, "add() seqno=%d term=%s count=%d", 
       seqno, term->name,ti->local_occur); */
}

/*
 * calc: Called for each document in a result. This handler should 
 *  produce a score based on previous call(s) to the add handler. The
 *  score should be between 0 and 1000. If score cannot be obtained
 *  -1 should be returned.
 */
static int calc (void *set_handle, zint sysno, zint staticrank,
		 int *stop_flag)
{
  int i, score = 0;
  struct ranksimilarity_set_info *si 
    = (struct ranksimilarity_set_info *) set_handle;
  
  yaz_log(log_level, "calc()");
  
  if (!si->no_rank_entries)
    return -1;   /* ranking not enabled for any terms */

  /* here you put in your own score function */
  
  
  /* reset the counts for the next term */
  for (i = 0; i < si->no_entries; i++)
    si->entries[i].local_occur = 0;
  
  /* if we set *stop_flag = 1, we stop processing (of result set list) */
  /* staticrank = 0 is highest, MAXINT lowest */


  /* here goes your formula to compute a scoring function */
  /* you may use all the gathered statistics here */

  score = INT_MAX - staticrank;  /* but score is reverse (logical) */



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
    "rank-similarity",
    create,
    destroy,
    begin,
    end,
    calc,
    add,
};
 
struct rank_control *rank_similarity_class = &rank_control;
