/* $Id: zvrank.c,v 1.1 2003-02-27 22:55:40 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003
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

/* zvrank.c */
/* Vector Space Model for Zebra */
/*
** six (seven) letter identifier for weighting schema
** best document weighting:
**  tfc nfc (tpc npc) [original naming]
**  ntc atc  npc apc  [SMART naming, used here]
** best query weighting:
**  nfx tfx bfx (npx tpx bpx) [original naming]
**  atn ntn btn  apn npn bpn  [SMART naming]
*/

#include <math.h>  /* for log */

#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "index.h"

double blog2(double x) { /* sometimes log_e or log_10 is used */
  if (x <= 0)
    return 0;
  return log(x);
}

/* structures */

struct rs_info { /* for result set */
  int db_docs;        /* number of documents in database (collection) */
  int db_terms;       /* number of distinct terms in database */
  int db_f_max;       /* maximum of f_t in database */
  char *db_f_max_str; /* string (most frequent term) */
  /**/
  char rschema[8];    /* name of ranking schema */
  /**/
    int veclen;
  void (*d_tf_fct)(void *, void *);   /* doc term frequency function */
  void (*d_idf_fct)(void *, void *);  /* doc idf function */
  void (*d_norm_fct)(void *, void *); /* doc normalization function */
  /**/
  void (*q_tf_fct)(void *, void *);   /* query term frequency function */
  void (*q_idf_fct)(void *, void *);  /* query idf function */
  void (*q_norm_fct)(void *, void *); /* query normalization function */
  
  double (*sim_fct)(void *, void *);  /* similarity function (scoring function) */
  struct ds_info *qdoc;
  struct ds_info *rdoc;
};
typedef struct rs_info *RS;

void prn_rs(RS rs) {
  int i;
  fprintf(stdout, "* RS:\n");
  fprintf(stdout, " db_docs:   %d\n", rs->db_docs);
  fprintf(stdout, " db_terms:  %d\n", rs->db_terms);
  fprintf(stdout, " f_max:     %d\n", rs->db_f_max);
  fprintf(stdout, " f_max_str: %s\n", rs->db_f_max_str);
  fprintf(stdout, " veclen:    %d\n", rs->veclen);
  /* rschema implies functions */
  fprintf(stdout, " rschema:   %s\n", rs->rschema);
  return;
}

struct ds_info {       /* document info */
  char *docid;         /* unique doc identifier */
  int  docno;          /* doc number */
  int doclen;          /* document length */
  int d_f_max;         /* maximum number of any term in doc */
  char *d_f_max_str;   /* most frequent term in d */
  int veclen;          /* vector length */
  struct ts_info *terms;
  double docsim;       /* similarity in [0, ..., 1] (= score/1000) */
};
typedef struct ds_info* DS;

void prn_ds(DS ds) {
    fprintf(stdout, " * DS:\n");
    fprintf(stdout, " docid:      %s\n", ds->docid);
    fprintf(stdout, " docno:      %d\n", ds->docno);
    fprintf(stdout, " doclen:     %d\n", ds->doclen);
    fprintf(stdout, " d_f_max:    %d\n", ds->d_f_max);
    fprintf(stdout, " d_f_max_str:%s\n", ds->d_f_max_str);
    fprintf(stdout, " veclen:     %d\n", ds->veclen);
    return;
}

struct ts_info {       /* term info */
  char *name;
  int *id;
  /**/
  int gocc;
  int locc;
  double tf;
  double idf;
  double wt;
};
typedef struct ts_info *TS;

void prn_ts(TS ts) {
    fprintf(stdout, " * TERM:%s gocc:%d locc:%d  tf:%f idf:%f wt:%f\n",
	    ts->name, ts->gocc, ts->locc, ts->tf, ts->idf, ts->wt);
    return;
}

/* end structures */

/* *** */

/* 
** weighting functions 
*/

/* calculate new term frequency vector */
void tf_none(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int i;
  int veclen;
  int freq;
  /**/
  veclen=ds->veclen;
  for (i=0; i < veclen; i++) {
    freq=ds->terms[i].locc;
    ds->terms[i].tf=freq;
  }
  return;
}

void tf_binary(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
    int i;
    int veclen;
    int freq;
    /**/
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
      freq=ds->terms[i].locc;
      if (freq > 0)
	ds->terms[i].tf=1.0;
      else
	ds->terms[i].tf=0.0;
    }
    return;
}

void tf_max_norm(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
    int tf_max;
    int i;
    int veclen;
    int freq;
    /**/
    tf_max=rs->db_f_max;
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
      freq=ds->terms[i].locc;
      if ((freq > 0) &&
	  (tf_max > 0)) 
	ds->terms[i].tf=freq/tf_max;
      else
	ds->terms[i].tf=0.0;
    }
    return;
}

void tf_aug_norm(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
    double K;
    double tf_max;
    int i;
    int veclen;
    int freq;
    /**/
    tf_max=rs->db_f_max;
    veclen=ds->veclen;
    K=0.5;
    for (i=0; i < veclen; i++) {
      freq=ds->terms[i].locc;
      if ((freq > 0) &&
	  (tf_max > 0)) 
	ds->terms[i].tf=K+(1-K)*(freq/tf_max);
      else
	ds->terms[i].tf=0.0;
    }
    return;
}

void tf_square(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
    int i;
    int veclen;
    int freq;
    /**/
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
      freq=ds->terms[i].locc;
      if (freq > 0) 
	ds->terms[i].tf=freq*freq;
      else
	ds->terms[i].tf=0.0;
    }
    return;
}

void tf_log(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
    int i;
    int veclen;
    int freq;
    /**/    
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
      freq=ds->terms[i].locc;
      if (freq > 0) 
	ds->terms[i].tf=1+blog2(freq);
      else
	ds->terms[i].tf=0.0;
    }
    return;
}

/* calculate inverse document frequency vector */
void idf_none(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int i, veclen;
  int gocc;
  /**/
  veclen=ds->veclen;
  for (i=0; i < veclen; i++) {
    ds->terms[i].idf=1.0;
  }
  return;
}

void idf_tfidf(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int num_docs;
  int i, veclen;
  int gocc;
  double idf;
  /**/
  veclen=ds->veclen;
  num_docs=rs->db_docs;
  for (i=0; i < veclen; i++) {
    gocc=ds->terms[i].gocc;
    if (gocc==0) 
      idf=0.0; 
    else
      idf=blog2(num_docs/gocc);
    ds->terms[i].idf=idf;
  }
  return;
}

void idf_prob(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int num_docs;
  int i, veclen;
  int gocc;
  double idf;
  /**/
  veclen=ds->veclen;
  num_docs=rs->db_docs;
  for (i=0; i < veclen; i++) {
    gocc=ds->terms[i].gocc;
    if (gocc==0)
      idf=0.0; 
    else
      idf=(num_docs-gocc)/gocc;
    ds->terms[i].idf=idf;
  }
  return;
}

void idf_freq(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int num_docs;
  int i, veclen;
  int gocc;
  double idf;
  /**/
  veclen=ds->veclen;
  num_docs=rs->db_docs;
  if (num_docs==0)
    idf=0.0;
  else
    idf=1/num_docs;
  for (i=0; i < veclen; i++) {
    // gocc=ds->terms[i].gocc;
    ds->terms[i].idf=idf;
  }
  return;
}

void idf_squared(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int num_docs;
  int i, veclen;
  int gocc;
  double idf;
  /**/
  veclen=ds->veclen;
  num_docs=rs->db_docs;
  for (i=0; i < veclen; i++) {
    gocc=ds->terms[i].gocc;
    if (gocc==0.0)
      idf=0.0;
    else
      idf=blog2(num_docs/gocc);
      idf=idf*idf;
      ds->terms[i].idf=idf;
  }
  return;
}

/* calculate normalized weight (tf-idf) vector */
void norm_none(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int i, veclen;
  /**/
  veclen=ds->veclen;
  for (i=0; i < veclen; i++) {
    ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
  }
  return;
}

void norm_sum(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int i, veclen;
  double tfs=0.0;
  /**/
  veclen=ds->veclen;
  for (i=0; i < veclen; i++) {
    ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
    tfs+=ds->terms[i].wt;
  } 
  for (i=0; i < veclen; i++) {
    if (tfs > 0)
      ds->terms[i].wt=ds->terms[i].wt/tfs;
  }
  return;
}

void norm_cosine(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int i, veclen;
  double tfs=0.0;
  /**/
  veclen=ds->veclen;
  for (i=0; i < veclen; i++) {
    ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
    tfs+=(ds->terms[i].wt*ds->terms[i].wt);
  } 
  for (i=0; i < veclen; i++) {
    if (tfs > 0)
      ds->terms[i].wt=ds->terms[i].wt/tfs;
  }
  return;
}

void norm_fourth(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int i, veclen;
  double tfs=0.0, fr;
  /**/
  veclen=ds->veclen;
  for (i=0; i < veclen; i++) {
    ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
    fr=(ds->terms[i].wt*ds->terms[i].wt);
    fr=fr*fr;
    tfs+=fr; /* ^ 4 */
  }
  for (i=0; i < veclen; i++) {
    if (tfs > 0)
      ds->terms[i].wt=ds->terms[i].wt/tfs;
  }
  return;
}

void norm_max(void *rsi, void *dsi) {
  RS rs=(RS)rsi;
  DS ds=(DS)dsi;
  int i, veclen;
  double tfm;
  /**/
  veclen=ds->veclen;
  for (i=0; i < veclen; i++) {
    ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
    if (ds->terms[i].wt > tfm)
      tfm=ds->terms[i].wt;
  }
  for (i=0; i < veclen; i++) {
    if (tfm > 0)
      ds->terms[i].wt=ds->terms[i].wt/tfm;
  }
  return;
}

/* add: norm_pivot, ... */

double sim_cosine(void *dsi1, void *dsi2) {
  DS ds1=(DS)dsi1;
  DS ds2=(DS)dsi2;
    int i, veclen;
    double smul=0.0, sdiv=0.0, sqr11=0.0, sqr22=0.0;
    double v1, v2;
    /**/
    veclen=ds1->veclen; /* and ds2->veclen */
    for (i=0; i < veclen; i++) {
      v1=ds1->terms[i].wt;
      v2=ds2->terms[i].wt;
      smul +=(v1*v2);
      sqr11+=(v1*v1);
      sqr22+=(v2*v2);
    }
    sdiv=sqrt(sqr11*sqr22);
    if (sdiv==0.0)
      return 0.0;
    return (smul/sdiv);
  }

/* add: norm_jaccard, norm_dice, ... */

/* end weighting functions */

/* *** */

/* best-fully-weighted */
const char* def_rschema="ntc-atn";

/* prototype */
void zv_init_schema(RS, const char*);

void zv_init(RS rs) {
  char *sname="ntc-atn";/* obtain from configuration file */ 
  fprintf(stdout, "zv_init\n");
  /* alloc rs */
  rs->db_docs=100000;   /* assign correct value here */
  rs->db_terms=500000;  /* assign correct value here */
  rs->db_f_max=50;      /* assign correct value here */
  rs->db_f_max_str="a"; /* assign correct value here */
  zv_init_schema(rs, sname);
  return;
}

void zv_init_schema(RS rs, const char *sname) {
  int slen;
  char c0, c1, c2, c3, c4, c5, c6;
  /**/
  fprintf(stdout, "zv_init_schema\n");
  slen=strlen(sname);
  if (slen>0) c0=sname[0]; else c0=def_rschema[0];
  if (slen>0) c1=sname[1]; else c0=def_rschema[1];
  if (slen>0) c2=sname[2]; else c0=def_rschema[2];
  c3='-';
  if (slen>0) c4=sname[4]; else c0=def_rschema[4];
  if (slen>0) c5=sname[5]; else c0=def_rschema[5];
  if (slen>0) c6=sname[6]; else c0=def_rschema[6];
  /**/
  /* assign doc functions */
  switch (c0) {
  case 'b':
    rs->d_tf_fct=tf_binary;
    rs->rschema[0]='b';
    break;
  case 'm':
    rs->d_tf_fct=tf_max_norm;
    rs->rschema[0]='m';
    break;
  case 'a':
    rs->d_tf_fct=tf_aug_norm;
    rs->rschema[0]='a';
    break;
  case 's':
    rs->d_tf_fct=tf_square;
    rs->rschema[0]='s';
    break;
  case 'l':
    rs->d_tf_fct=tf_log;
    rs->rschema[0]='l';
    break;
  default: /* 'n' */
    rs->d_tf_fct=tf_none;
    rs->rschema[0]='n';
  }
  switch (c1) {
  case 't':
    rs->d_idf_fct=idf_tfidf;
    rs->rschema[1]='t';
    break;
  case 'p':
    rs->d_idf_fct=idf_prob;
    rs->rschema[1]='p';
    break;
  case 'f':
    rs->d_idf_fct=idf_freq;
    rs->rschema[1]='f';
    break;
  case 's':
    rs->d_idf_fct=idf_squared;
    rs->rschema[1]='s';
    break;
  default: /* 'n' */
    rs->d_idf_fct=idf_none;
    rs->rschema[1]='n';
  }
  switch (c2) {
  case 's':
    rs->d_norm_fct=norm_sum;
    rs->rschema[2]='s';
    break;
  case 'c':
    rs->d_norm_fct=norm_cosine;
    rs->rschema[2]='c';
    break;
  case 'f':
    rs->d_norm_fct=norm_fourth;
    rs->rschema[2]='t';
    break;
  case 'm':
    rs->d_norm_fct=norm_max;
    rs->rschema[2]='m';
    break;
  default: /* 'n' */
    rs->d_norm_fct=norm_none;
    rs->rschema[2]='n';
  }
  /**/
  rs->rschema[3]='-';
  /* assign query functions */
  switch (c4) {
  case 'b':
    rs->q_tf_fct=tf_binary;
    rs->rschema[4]='b';
    break;
  case 'm':
    rs->q_tf_fct=tf_max_norm;
    rs->rschema[4]='m';
    break;
  case 'a':
    rs->q_tf_fct=tf_aug_norm;
    rs->rschema[4]='a';
    break;
  case 's':
    rs->q_tf_fct=tf_square;
    rs->rschema[4]='s';
    break;
  case 'l':
    rs->q_tf_fct=tf_log;
    rs->rschema[4]='l';
    break;
  default: /* 'n' */
    rs->q_tf_fct=tf_none;
    rs->rschema[4]='n';
  }
  switch (c5) {
  case 't':
    rs->q_idf_fct=idf_tfidf;
    rs->rschema[5]='t';
    break;
  case 'p':
    rs->q_idf_fct=idf_prob;
    rs->rschema[5]='p';
    break;
  case 'f':
    rs->q_idf_fct=idf_freq;
    rs->rschema[5]='f';
    break;
  case 's':
    rs->q_idf_fct=idf_squared;
    rs->rschema[5]='s';
    break;
  default: /* 'n' */
    rs->q_idf_fct=idf_none;
    rs->rschema[5]='n';
  }
  switch (c6) {
  case 's':
    rs->q_norm_fct=norm_sum;
    rs->rschema[6]='s';
    break;
  case 'c':
    rs->q_norm_fct=norm_cosine;
    rs->rschema[6]='c';
    break;
  case 'f':
    rs->q_norm_fct=norm_fourth;
    rs->rschema[6]='f';
    break;
  case 'm':
    rs->q_norm_fct=norm_max;
    rs->rschema[6]='m';
    break;
  default: /* 'n' */
    rs->q_norm_fct=norm_none;
    rs->rschema[6]='n';
  }
  rs->rschema[7]='\0';
  /**/
  rs->sim_fct=sim_cosine;
  fprintf(stdout, "zv_schema %s\n", rs->rschema);

  return;
}

/******/

struct rank_class_info { /* where do we need this ? */
    int dummy;
};

/*
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
*/


/*
 * zv_create: Creates/Initialises this rank handler. This routine is 
 *  called exactly once. The routine returns the class_handle.
 */
static void *zv_create (struct zebra_register *reg) {
    struct rank_class_info *ci = (struct rank_class_info *)
        xmalloc (sizeof(*ci));
    fprintf(stdout, "zv_create\n");
    logf (LOG_DEBUG, "zv_create");
    return ci;
}

/*
 * zv_destroy: Destroys this rank handler. This routine is called
 *  when the handler is no longer needed - i.e. when the server
 *  dies. The class_handle was previously returned by create.
 */
static void zv_destroy (struct zebra_register *reg, void *class_handle) {
    struct rank_class_info *ci = (struct rank_class_info *) class_handle;
    fprintf(stdout, "zv_destroy\n");
    logf (LOG_DEBUG, "zv_destroy");
    xfree (ci);
}


/*
 * zv_begin: Prepares beginning of "real" ranking. Called once for
 *  each result set. The returned handle is a "set handle" and
 *  will be used in each of the handlers below.
 */
static void *zv_begin (struct zebra_register *reg, void *class_handle, RSET rset)
{
    struct rs_info *rs=(struct rs_info *)xmalloc(sizeof(*rs));
    int i;
    int veclen, gocc;
    /**/
    logf (LOG_DEBUG, "rank-1 zvbegin");
    fprintf(stdout, "zv_begin\n");
    veclen=rset->no_rset_terms; /* smaller vector here */
    zv_init(rs);
    rs->veclen=veclen;
    prn_rs(rs);

    rs->qdoc=(struct ds_info *)xmalloc(sizeof(*rs->qdoc));
    rs->qdoc->terms=(struct ts_info *)xmalloc(sizeof(*rs->qdoc->terms)*rs->veclen);
    rs->qdoc->veclen=veclen;

    rs->rdoc=(struct ds_info *)xmalloc(sizeof(*rs->rdoc));
    rs->rdoc->terms=(struct ts_info *)xmalloc(sizeof(*rs->rdoc->terms)*rs->veclen);
    rs->rdoc->veclen=veclen;
    /*
    si->no_entries = rset->no_rset_terms;
    si->no_rank_entries = 0;
    si->entries = (struct rank_term_info *)
	xmalloc (sizeof(*si->entries)*si->no_entries);
    */
    /* fprintf(stdout, "zv_begin_init\n"); */
    for (i = 0; i < rs->veclen; i++)
    {

	gocc=rset->rset_terms[i]->nn;
	/* fprintf(stdout, "zv_begin_init i=%d gocc=%d\n", i, gocc); */
	if (!strncmp (rset->rset_terms[i]->flags, "rank,", 5)) {
            yaz_log (LOG_LOG, "%s", rset->rset_terms[i]->flags);
	    /*si->entries[i].rank_flag = 1;
	    (si->no_rank_entries)++;
	    */
	} else {
	    /* si->entries[i].rank_flag = 0; */
	}
	rs->qdoc->terms[i].gocc=gocc;
	rs->qdoc->terms[i].locc=1;  /* assume query has no duplicates */
	rs->rdoc->terms[i].gocc=gocc;
	rs->rdoc->terms[i].locc=0;
    }
    return rs;
}

/*
 * zv_end: Terminates ranking process. Called after a result set
 *  has been ranked.
 */
static void zv_end (struct zebra_register *reg, void *rsi)
{
    RS rs=(RS)rsi;
    fprintf(stdout, "zv_end\n");
    logf (LOG_DEBUG, "rank-1 end");
    xfree(rs->qdoc->terms);
    xfree(rs->rdoc->terms);
    xfree(rs->qdoc);
    xfree(rs->rdoc);
    xfree(rs);
    return;
}

/*
 * zv_add: Called for each word occurence in a result set. This routine
 *  should be as fast as possible. This routine should "incrementally"
 *  update the score.
 */
static void zv_add (void *rsi, int seqno, int i) {
    RS rs=(RS)rsi;
    /*logf (LOG_DEBUG, "rank-1 add seqno=%d term_index=%d", seqno, term_index);*/
    /*si->last_pos = seqno;*/
    rs->rdoc->terms[i].locc++;
}

/*
 * zv_calc: Called for each document in a result. This handler should 
 *  produce a score based on previous call(s) to the add handler. The
 *  score should be between 0 and 1000. If score cannot be obtained
 *  -1 should be returned.
 */
static int zv_calc (void *rsi, int sysno)
{
    int i, veclen; //lo, divisor, score = 0;
    int score=0;
    double dscore=0.0;
    RS rs=(RS)rsi;
    /* fprintf(stdout, "zv_calc\n"); */
    /**/
    veclen=rs->veclen;
    if (veclen==0)
	return -1;
    for (i = 0; i < veclen; i++) {
	(*rs->q_tf_fct)(rs, rs->qdoc); /* we should actually do this once */
	(*rs->q_idf_fct)(rs, rs->qdoc);
	(*rs->q_norm_fct)(rs, rs->qdoc);

	(*rs->d_tf_fct)(rs, rs->rdoc);
	(*rs->d_idf_fct)(rs, rs->rdoc);
	(*rs->d_norm_fct)(rs, rs->rdoc);
	
	dscore=rs->sim_fct(rs->qdoc, rs->rdoc);
    }
    score = dscore * 1000;
    yaz_log (LOG_LOG, "sysno=%d score=%d", sysno, score);
    if (score > 1000)
	score = 1000;
    /*
      for (i = 0; i < si->no_entries; i++)
	si->entries[i].local_occur = 0;
    */
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

static struct rank_control rank_control_vsm = {
    "zvrank", /* "zv_rank", */ /* zvrank */
    zv_create,
    zv_destroy,
    zv_begin,
    zv_end,
    zv_calc,
    zv_add,
};
 
struct rank_control *rankzv_class = &rank_control_vsm;

/* EOF */
