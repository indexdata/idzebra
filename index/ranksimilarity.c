/* $Id: ranksimilarity.c,v 1.9 2006-05-19 23:20:24 adam Exp $
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

/** term specific info and statistics to be used under ranking */
struct ranksimilarity_term_info {

  /** frequency of term within document field */
  int freq_term_docfield;

  /** frequency of term within result set of given term */
  zint freq_term_resset;

  /** number of docs within result set */
  zint no_docs_resset;

   /** number of docs with this fieldindex in database */
  zint no_docs_fieldindex;

  /**  number of terms in this fieldindex */
  zint no_terms_fieldindex;

  /** rank flag is one if term is to be included in ranking */
  int rank_flag;

  /** relative ranking weight of term fieldindex */
  int fieldindex_weight;

  /** term id used to access term name and other info */
  TERMID term;

  /** index number in terms[i] array */
  int term_index;
};

struct ranksimilarity_set_info {
  int last_pos;

  /** number of terms in query */
  int no_terms_query;

  /** number of terms in query which are included in ranking */
  int no_ranked_terms_query;

  /** number of documents in entire database */
  zint no_docs_database;

  /** number of terms in entire database */
  zint no_terms_database;

  /** array of size no_terms_query with statistics gathered per term */
  struct ranksimilarity_term_info *entries;

  NMEM nmem;
};


/* local clean-up function */
static void  ranksimilar_rec_reset(struct ranksimilarity_set_info *si)
{
  int i;
  
  for (i = 0; i < si->no_terms_query; i++)
    {
      si->entries[i].freq_term_docfield = 0;
    }
}


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
      log_level = yaz_log_module_level("rank-similarity");
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
    
  yaz_log(log_level, "begin() numterms=%d", numterms);
 
  /* setting database global statistics */
   si->no_docs_database = -1;  /* TODO */
   si->no_terms_database = -1;  /* TODO */

  /* setting query statistics */
   si->no_terms_query = numterms;
   si->no_ranked_terms_query = 0;

  /* setting internal data structures */
  si->nmem=nmem;
  si->entries = (struct ranksimilarity_term_info *)
    nmem_malloc (si->nmem, sizeof(*si->entries)*numterms); 

  /* reset the counts for the next term */
  ranksimilar_rec_reset(si);


  /* looping all terms in a specific fieldindex of query */
  for (i = 0; i < numterms; i++)
    {
      struct ord_list *ol = NULL;


      /* adding to number of rank entries */
      if (strncmp (terms[i]->flags, "rank,", 5)) 
        {
          si->entries[i].rank_flag = 0;
          yaz_log(log_level, "begin() terms[%d]: '%s' flags=%s not ranked", 
                  i, terms[i]->name, terms[i]->flags);
        } 
      else 
        {
          const char *cp = strstr(terms[i]->flags+4, ",w=");

          zint no_docs_fieldindex = 0;
          zint no_terms_fieldindex = 0;
 
          yaz_log(log_level, "begin() terms[%d]: '%s' flags=%s", 
                  i, terms[i]->name, terms[i]->flags);

          (si->no_ranked_terms_query)++;
          ol = terms[i]->ol;

          si->entries[i].rank_flag = 1;
          /* notice that the call to rset_count(rset) has he side-effect 
             of setting rset->hits_limit = rset_count(rset) ??? */
          si->entries[i].freq_term_resset = rset_count(terms[i]->rset);
          si->entries[i].no_docs_resset =  terms[i]->rset->hits_count;


          if (cp)
            si->entries[i].fieldindex_weight = atoi (cp+3);
          else
            si->entries[i].fieldindex_weight = 34; /* sqrroot of 1000 */


          /*
          yaz_log(log_level, "begin() rset_count(terms[%d]->rset) = " 
                  ZINT_FORMAT, i, rset_count(terms[i]->rset)); 
          yaz_log(log_level, "begin() terms[%d]->rset->hits_limit = "
                  ZINT_FORMAT, i, terms[i]->rset->hits_limit); 
          yaz_log(log_level, "begin() terms[%d]->rset->hits_count = "
                  ZINT_FORMAT, i, terms[i]->rset->hits_count); 
          yaz_log(log_level, "begin() terms[%d]->rset->hits_round = "
                  ZINT_FORMAT, i, terms[i]->rset->hits_round); 
          yaz_log(log_level, "begin() terms[%d]->rset->hits_approx = %d", 
                  i, terms[i]->rset->hits_approx);
          */
          
          /* looping indexes where term terms[i] is found */
          
         for (; ol; ol = ol->next)
            {
              int index_type = 0;
              const char *db = 0;
              const char *string_index = 0;

              zebraExplain_lookup_ord(reg->zei,
                                      ol->ord, &index_type, &db,
                                      &string_index);
              
              no_docs_fieldindex 
                  += zebraExplain_ord_get_doc_occurrences(reg->zei, ol->ord);
              no_terms_fieldindex 
                  += zebraExplain_ord_get_term_occurrences(reg->zei, ol->ord);

              if (string_index)
		yaz_log(log_level, 
                        "begin()    index: ord=%d type=%c db=%s str-index=%s",
                        ol->ord, index_type, db, string_index);
              else
		yaz_log(log_level, 
                        "begin()    index: ord=%d type=%c db=%s",
                        ol->ord, index_type, db);
            }
     
          si->entries[i].no_docs_fieldindex = no_docs_fieldindex;
          si->entries[i].no_terms_fieldindex = no_terms_fieldindex;
        }
        
      si->entries[i].term = terms[i];
      si->entries[i].term_index=i;
        
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
  struct ranksimilarity_set_info *si 
    = (struct ranksimilarity_set_info *) set_handle;
  struct ranksimilarity_term_info *ti; 
  assert(si);
  if (!term)
    {
      /* yaz_log(log_level, "add() seqno=%d NULL term", seqno); */
      return;
    }

  ti= (struct ranksimilarity_term_info *) term->rankpriv;
  assert(ti);
  si->last_pos = seqno;
  ti->freq_term_docfield++;
  /*yaz_log(log_level, "add() seqno=%d term=%s freq_term_docfield=%d", 
    seqno, term->name, ti->freq_term_docfield); */
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
  
  
  yaz_log(log_level, "calc() sysno =      " ZINT_FORMAT, sysno);
  yaz_log(log_level, "calc() staticrank = " ZINT_FORMAT, staticrank);
        
  yaz_log(log_level, "calc() si->no_terms_query = %d", 
          si->no_terms_query);
  yaz_log(log_level, "calc() si->no_ranked_terms_query = %d", 
          si->no_ranked_terms_query);
  yaz_log(log_level, "calc() si->no_docs_database = " ZINT_FORMAT,  
          si->no_docs_database); 
  yaz_log(log_level, "calc() si->no_terms_database = " ZINT_FORMAT,  
          si->no_terms_database); 

  
  if (!si->no_ranked_terms_query)
    return -1;   /* ranking not enabled for any terms */

  
  /* if we set *stop_flag = 1, we stop processing (of result set list) */


  /* here goes your formula to compute a scoring function */
  /* you may use all the gathered statistics here */
  for (i = 0; i < si->no_terms_query; i++)
    {
      yaz_log(log_level, "calc() entries[%d] termid %p", 
              i, si->entries[i].term);
      if (si->entries[i].term){
        yaz_log(log_level, "calc() entries[%d] term '%s' flags=%s", 
                i, si->entries[i].term->name, si->entries[i].term->flags);
        yaz_log(log_level, "calc() entries[%d] rank_flag %d", 
                i, si->entries[i].rank_flag );
        yaz_log(log_level, "calc() entries[%d] fieldindex_weight %d", 
                i, si->entries[i].fieldindex_weight );
        yaz_log(log_level, "calc() entries[%d] freq_term_docfield %d", 
                i, si->entries[i].freq_term_docfield );
        yaz_log(log_level, "calc() entries[%d] freq_term_resset " ZINT_FORMAT,
                i, si->entries[i].freq_term_resset );
        yaz_log(log_level, "calc() entries[%d] no_docs_resset " ZINT_FORMAT, 
                i, si->entries[i].no_docs_resset );
        yaz_log(log_level, "calc() entries[%d] no_docs_fieldindex " 
                ZINT_FORMAT, 
                i, si->entries[i].no_docs_fieldindex );
        yaz_log(log_level, "calc() entries[%d] no_terms_fieldindex " 
                ZINT_FORMAT, 
                i, si->entries[i].no_terms_fieldindex );
      }
    }
  

  /* reset the counts for the next term */
  ranksimilar_rec_reset(si);
 

  /* staticrank = 0 is highest, MAXINT lowest */
  score = INT_MAX - staticrank;  /* but score is reverse (logical) */


  /* debugging statistics output */
  yaz_log(log_level, "calc() statistics: score = %d", score); 

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
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

