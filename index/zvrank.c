/* $Id: zvrank.c,v 1.18 2005-08-19 09:21:34 adam Exp $
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

/*
Zvrank: an experimental ranking algorithm. See doc/zvrank.txt and
source in index/zvrank.c. Enable this by using rank: zvrank in zebra.cfg.
Contributed by Johannes Leveling <Johannes.Leveling at
fernuni-hagen.de>
*/

/* Zebra Vector Space Model RANKing 
**
** six (seven) letter identifier for weighting scheme
** best document weighting:
**  tfc nfc (tpc npc) [original naming]
**  ntc atc  npc apc  [SMART naming, used here]
** best query weighting:
**  nfx tfx bfx (npx tpx bpx) [original naming]
**  atn ntn btn  apn npn bpn  [SMART naming]
** -> should set zvrank.weighting-scheme to one of
** "ntc-atn", "atc-atn", etc.
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

static int log_level = 0;
static int log_initialized = 0;

static double blog(double x) { 
    /* log_2, log_e or log_10 is used, best to change it here if necessary */
    if (x <= 0)
        return 0.0;
    return log(x); /* / log(base) */
}

/* structures */

struct rank_class_info {
    char rscheme[8];    /* name of weighting scheme */
};


struct rs_info {      /* for result set */
    int db_docs;        /* number of documents in database (collection) */
    int db_terms;       /* number of distinct terms in database (debugging?) */
    int db_f_max;       /* maximum of f_t in database (debugging?) */
    char *db_f_max_str; /* string (most frequent term) - for debugging */
    /**/
    char rscheme[8];    /* name of weighting scheme */
    /**/
    int veclen;
    NMEM nmem;
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

static void prn_rs(RS rs) { /* for debugging */
    yaz_log(log_level, "* RS:");
    yaz_log(log_level, " db_docs:   %d", rs->db_docs);
    yaz_log(log_level, " db_terms:  %d", rs->db_terms);
    yaz_log(log_level, " f_max:     %d", rs->db_f_max);
    yaz_log(log_level, " f_max_str: %s", rs->db_f_max_str);
    yaz_log(log_level, " veclen:    %d", rs->veclen);
    /* rscheme implies functions */
    yaz_log(log_level, " rscheme:   %s", rs->rscheme);
    return;
}

struct ds_info {       /* document info */
    char *docid;         /* unique doc identifier */
    int  docno;          /* doc number */
    int doclen;          /* document length */
    int d_f_max;         /* maximum number of any term in doc (needed) */
    char *d_f_max_str;   /* most frequent term in d - for debugging */
    int veclen;          /* vector length */
    struct ts_info *terms;
    double docsim;       /* similarity in [0, ..., 1] (= score/1000) */
};
typedef struct ds_info* DS;

#if 0
static void prn_ds(DS ds) { /* for debugging */
    yaz_log(log_level, " * DS:");
    yaz_log(log_level, " docid:      %s", ds->docid);
    yaz_log(log_level, " docno:      %d", ds->docno);
    yaz_log(log_level, " doclen:     %d", ds->doclen);
    yaz_log(log_level, " d_f_max:    %d", ds->d_f_max);
    yaz_log(log_level, " d_f_max_str:%s", ds->d_f_max_str);
    yaz_log(log_level, " veclen:     %d", ds->veclen);
    return;
}
#endif

struct ts_info {       /* term info */
    char *name;
    int *id;
    /**/
    zint gocc;
    int locc;
    double tf;
    double idf;
    double wt;
};
typedef struct ts_info *TS;

#if 0
static void prn_ts(TS ts) { /* for debugging */
    yaz_log(log_level, " * TERM:%s gocc:%d locc:%d  tf:%f idf:%f wt:%f",
            ts->name, ts->gocc, ts->locc, ts->tf, ts->idf, ts->wt);
    return;
}
#endif

/* end structures */

/* *** */

/* 
** weighting functions 
** check: RS is not needed anymore
*/

/* calculate and store new term frequency vector */
static void tf_none(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen, freq;
    /* no conversion. 1 <= tf */
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        freq=ds->terms[i].locc;
        ds->terms[i].tf=freq;
    }
    return;
}

static void tf_binary(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen, freq;
    /* tf in {0, 1} */
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

static void tf_max_norm(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    double tf_max;
    int i, veclen, freq;
    /* divide each term by max, so 0 <= tf <= 1 */
    tf_max=ds->d_f_max; /* largest frequency of t in document */
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        freq=ds->terms[i].locc;
        if ((freq > 0) &&
            (tf_max > 0.0)) 
            ds->terms[i].tf=freq/tf_max;
        else
            ds->terms[i].tf=0.0;
    }
    return;
}

static void tf_aug_norm(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    double K; 
    double tf_max;
    int i, veclen, freq;
    /* augmented normalized tf. 0.5 <= tf <= 1  for K = 0.5 */
    tf_max=ds->d_f_max; /* largest frequency of t in document */
    veclen=ds->veclen;
    K=0.5; /* zvrank.const-K */
    for (i=0; i < veclen; i++) {
        freq=ds->terms[i].locc;
        if ((freq > 0) &&
            (tf_max > 0.0)) 
            ds->terms[i].tf=K+(1.0-K)*(freq/tf_max);
        else
            ds->terms[i].tf=0.0;
    }
    return;
}

static void tf_square(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen, freq;
    /* tf ^ 2 */
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

static void tf_log(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen, freq;
    /* logarithmic tf */    
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        freq=ds->terms[i].locc;
        if (freq > 0) 
            ds->terms[i].tf=1.0+blog(freq);
        else
            ds->terms[i].tf=0.0;
    }
    return;
}

/* calculate and store inverse document frequency vector */
static void idf_none(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen;
    /* no conversion */
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        ds->terms[i].idf=1.0;
    }
    return;
}

static void idf_tfidf(void *rsi, void *dsi) {
    RS rs=(RS)rsi;
    DS ds=(DS)dsi;
    zint num_docs, gocc;
    int i, veclen;
    double idf;
    /* normal tfidf weight */
    veclen=ds->veclen;
    num_docs=rs->db_docs;
    for (i=0; i < veclen; i++) {
        gocc=ds->terms[i].gocc;
        if (gocc==0) 
            idf=0.0; 
        else
            idf=blog((double) (num_docs/gocc));
        ds->terms[i].idf=idf;
    }
    return;
}

static void idf_prob(void *rsi, void *dsi) {
    RS rs=(RS)rsi;
    DS ds=(DS)dsi;
    zint num_docs, gocc;
    int i, veclen;
    double idf;
    /* probabilistic formulation */
    veclen=ds->veclen;
    num_docs=rs->db_docs;
    for (i=0; i < veclen; i++) {
        gocc=ds->terms[i].gocc;
        if (gocc==0)
            idf=0.0; 
        else
            idf=blog((double) ((num_docs-gocc)/gocc));
        ds->terms[i].idf=idf;
    }
    return;
}

static void idf_freq(void *rsi, void *dsi) {
    RS rs=(RS)rsi;
    DS ds=(DS)dsi;
    int num_docs;
    int i, veclen;
    double idf;
    /* frequency formulation */
    veclen=ds->veclen;
    num_docs=rs->db_docs;
    if (num_docs==0)
        idf=0.0;
    else
        idf=1.0/num_docs;
    for (i=0; i < veclen; i++) {
        ds->terms[i].idf=idf;
    }
    return;
}

static void idf_squared(void *rsi, void *dsi) {
    RS rs=(RS)rsi;
    DS ds=(DS)dsi;
    zint num_docs, gocc;
    int i, veclen;
    double idf;
    /* idf ^ 2 */
    veclen=ds->veclen;
    num_docs=rs->db_docs;
    yaz_log(log_level, "idf_squared: db_docs required");
    for (i=0; i < veclen; i++) {
        gocc=ds->terms[i].gocc;
        if (gocc==0)
            idf=0.0;
        else 
            idf=blog(CAST_ZINT_TO_DOUBLE(num_docs/gocc));
        idf=idf*idf;
        ds->terms[i].idf=idf;
    }
    return;
}

/* calculate and store normalized weight (tf-idf) vector */
static void norm_none(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen;
    /* no normalization */
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
    }
    return;
}

static void norm_sum(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen;
    double tfs=0.0;
    /**/
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
        tfs+=ds->terms[i].wt;
    } 
    if (tfs > 0.0)
        for (i=0; i < veclen; i++) {
            ds->terms[i].wt=ds->terms[i].wt/tfs;
        }
    /* else: tfs==0 && ds->terms[i].wt==0 */
    return;
}

static void norm_cosine(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen;
    double tfs=0.0;
    /**/
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
        tfs+=(ds->terms[i].wt*ds->terms[i].wt);
    } 
    tfs=sqrt(tfs); 
    if (tfs > 0.0)
        for (i=0; i < veclen; i++) {
            ds->terms[i].wt=ds->terms[i].wt/tfs;
        }
    /* else: tfs==0 && ds->terms[i].wt==0 */
    return;
}

static void norm_fourth(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen;
    double tfs=0.0, fr;
    /**/
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
        fr=(ds->terms[i].wt*ds->terms[i].wt);
        fr=fr*fr; /* ^ 4 */
        tfs+=fr; 
    }
    if (tfs > 0.0)
        for (i=0; i < veclen; i++) {
            ds->terms[i].wt=ds->terms[i].wt/tfs;
        }
    /* else: tfs==0 && ds->terms[i].wt==0 */
    return;
}

static void norm_max(void *rsi, void *dsi) {
    DS ds=(DS)dsi;
    int i, veclen;
    double tfm=0.0;
    /**/
    veclen=ds->veclen;
    for (i=0; i < veclen; i++) {
        ds->terms[i].wt=ds->terms[i].tf*ds->terms[i].idf;
        if (ds->terms[i].wt > tfm)
            tfm=ds->terms[i].wt;
    }
    if (tfm > 0.0)
        for (i=0; i < veclen; i++) {
            ds->terms[i].wt=ds->terms[i].wt/tfm;
        }
    /* else: tfs==0 && ds->terms[i].wt==0 */
    return;
}

/* FIXME add norm_pivot, ... */

static double sim_cosine(void *dsi1, void *dsi2) {
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

/* FIXME: add norm_jaccard, norm_dice, ... */

/* end weighting functions */

/* *** */

static void zv_init_scheme(RS rs, const char *sname) {
    int slen;
    char c0, c1, c2, c3, c4, c5, c6;
    const char *def_rscheme="ntc-atn"; /* a good default */
    /**/
    yaz_log(log_level, "zv_init_scheme");
    slen=strlen(sname);
    if (slen < 7) 
        yaz_log(log_level, "zvrank: invalid weighting-scheme \"%s\"", sname);
    if (slen > 0) c0=sname[0]; else c0=def_rscheme[0];
    if (slen > 1) c1=sname[1]; else c1=def_rscheme[1];
    if (slen > 2) c2=sname[2]; else c2=def_rscheme[2];
    c3='-';
    if (slen > 4) c4=sname[4]; else c4=def_rscheme[4];
    if (slen > 5) c5=sname[5]; else c5=def_rscheme[5];
    if (slen > 6) c6=sname[6]; else c6=def_rscheme[6];
    
    /* assign doc functions */
    switch (c0) 
    {
        case 'b':
            rs->d_tf_fct=tf_binary;
            rs->rscheme[0]='b';
            break;
        case 'm':
            rs->d_tf_fct=tf_max_norm;
            rs->rscheme[0]='m';
            yaz_log(log_level, "tf_max_norm: d_f_max required");
            break;
        case 'a':
            rs->d_tf_fct=tf_aug_norm;
            rs->rscheme[0]='a';
            yaz_log(log_level, "tf_aug_norm: d_f_max required");
            break;
        case 's':
            rs->d_tf_fct=tf_square;
            rs->rscheme[0]='s';
            break;
        case 'l':
            rs->d_tf_fct=tf_log;
            rs->rscheme[0]='l';
            break;
        default: /* 'n' */
            rs->d_tf_fct=tf_none;
            rs->rscheme[0]='n';
    }
    switch (c1) 
    {
        case 't':
            rs->d_idf_fct=idf_tfidf;
            rs->rscheme[1]='t';
            yaz_log(log_level, "idf_tfidf: db_docs required");
            break;
        case 'p':
            rs->d_idf_fct=idf_prob;
            rs->rscheme[1]='p';
            yaz_log(log_level, "idf_prob: db_docs required");
            break;
        case 'f':
            rs->d_idf_fct=idf_freq;
            rs->rscheme[1]='f';
            yaz_log(log_level, "idf_freq: db_docs required");
            break;
        case 's':
            rs->d_idf_fct=idf_squared;
            rs->rscheme[1]='s';
            yaz_log(log_level, "idf_squared: db_docs required");
            break;
        default: /* 'n' */
            rs->d_idf_fct=idf_none;
            rs->rscheme[1]='n';
    }
    switch (c2) 
    {
        case 's':
            rs->d_norm_fct=norm_sum;
            rs->rscheme[2]='s';
            break;
        case 'c':
            rs->d_norm_fct=norm_cosine;
            rs->rscheme[2]='c';
            break;
        case 'f':
            rs->d_norm_fct=norm_fourth;
            rs->rscheme[2]='t';
            break;
        case 'm':
            rs->d_norm_fct=norm_max;
            rs->rscheme[2]='m';
            break;
        default: /* 'n' */
            rs->d_norm_fct=norm_none;
            rs->rscheme[2]='n';
    }
    rs->rscheme[3]='-';
    /* assign query functions */
    switch (c4) 
    {
        case 'b':
            rs->q_tf_fct=tf_binary;
            rs->rscheme[4]='b';
            break;
        case 'm':
            rs->q_tf_fct=tf_max_norm;
            yaz_log(log_level, "tf_max_norm: d_f_max required");
            rs->rscheme[4]='m';
            break;
        case 'a':
            rs->q_tf_fct=tf_aug_norm;
            rs->rscheme[4]='a';
            yaz_log(log_level, "tf_aug_norm: d_f_max required");
            break;
        case 's':
            rs->q_tf_fct=tf_square;
            rs->rscheme[4]='s';
            break;
        case 'l':
            rs->q_tf_fct=tf_log;
            rs->rscheme[4]='l';
            break;
        default: /* 'n' */
            rs->q_tf_fct=tf_none;
            rs->rscheme[4]='n';
    }
    switch (c5) 
    {
        case 't':
            rs->q_idf_fct=idf_tfidf;
            rs->rscheme[5]='t';
            yaz_log(log_level, "idf_tfidf: db_docs required");
            break;
        case 'p':
            rs->q_idf_fct=idf_prob;
            rs->rscheme[5]='p';
            yaz_log(log_level, "idf_prob: db_docs required");
            break;
        case 'f':
            rs->q_idf_fct=idf_freq;
            rs->rscheme[5]='f';
            yaz_log(log_level, "idf_freq: db_docs required");
            break;
        case 's':
            rs->q_idf_fct=idf_squared;
            rs->rscheme[5]='s';
            yaz_log(log_level, "idf_squared: db_docs required");
            break;
        default: /* 'n' */
            rs->q_idf_fct=idf_none;
            rs->rscheme[5]='n';
    }
    switch (c6) 
    {
        case 's':
            rs->q_norm_fct=norm_sum;
            rs->rscheme[6]='s';
            break;
        case 'c':
            rs->q_norm_fct=norm_cosine;
            rs->rscheme[6]='c';
            break;
        case 'f':
            rs->q_norm_fct=norm_fourth;
            rs->rscheme[6]='f';
            break;
        case 'm':
            rs->q_norm_fct=norm_max;
            rs->rscheme[6]='m';
            break;
        default: /* 'n' */
            rs->q_norm_fct=norm_none;
            rs->rscheme[6]='n';
    }
    rs->rscheme[7]='\0';
    rs->sim_fct=sim_cosine;
    yaz_log(log_level, "zv_scheme %s", rs->rscheme);
}

static void zv_init(RS rs, const char *rscheme) {
    yaz_log(log_level, "zv_init");
    /**/
    rs->db_docs=100000;   /* assign correct value here */
    rs->db_terms=500000;  /* assign correct value here (for debugging) */
    rs->db_f_max=50;      /* assign correct value here */
    rs->db_f_max_str="a"; /* assign correct value here (for debugging) */
    /* FIXME - get those values from somewhere */
    zv_init_scheme(rs, rscheme);
    return;
}

/******/

/*
 * zv_create: Creates/Initialises this rank handler. This routine is 
 *  called exactly once. The routine returns the class_handle.
 */
static void *zv_create (ZebraHandle zh) {
    int i;
    Res res = zh->res;
    const char *wscheme;
    struct rank_class_info *ci = (struct rank_class_info *)
        xmalloc (sizeof(*ci));
    if (!log_initialized)
    {
        log_level = yaz_log_module_level("zvrank");
        log_initialized = 1;
    }

    yaz_log(log_level, "zv_create");
    wscheme=  res_get_def(res, "zvrank.weighting-scheme", "");
    for (i = 0; wscheme[i] && i < 8; i++) 
        ci->rscheme[i]=wscheme[i];
    ci->rscheme[i] = '\0';
    return ci;
}

/*
 * zv_destroy: Destroys this rank handler. This routine is called
 *  when the handler is no longer needed - i.e. when the server
 *  dies. The class_handle was previously returned by create.
 */
static void zv_destroy (struct zebra_register *reg, void *class_handle) {
    struct rank_class_info *ci = (struct rank_class_info *) class_handle;
    yaz_log(log_level, "zv_destroy");
    xfree (ci);
}


/*
 * zv_begin: Prepares beginning of "real" ranking. Called once for
 *  each result set. The returned handle is a "set handle" and
 *  will be used in each of the handlers below.
 */
static void *zv_begin(struct zebra_register *reg, void *class_handle, 
                      RSET rset, NMEM nmem, TERMID *terms, int numterms)
{
    struct rs_info *rs=(struct rs_info *)nmem_malloc(nmem,sizeof(*rs));
    struct rank_class_info *ci=(struct rank_class_info *)class_handle;
    int i;
    int veclen;
    int *ip;
    zint gocc;
    /**/
    yaz_log(log_level, "zv_begin");
    veclen= numterms;
    zv_init(rs, ci->rscheme);
    rs->nmem=nmem;
    rs->veclen=veclen;
    prn_rs(rs);
  
    rs->qdoc=(struct ds_info *)nmem_malloc(nmem,sizeof(*rs->qdoc));
    rs->qdoc->terms=(struct ts_info *)nmem_malloc(nmem,
                                sizeof(*rs->qdoc->terms)*rs->veclen);
    rs->qdoc->veclen=veclen;
    rs->qdoc->d_f_max=1; /* no duplicates */ 
    rs->qdoc->d_f_max_str=""; 

    rs->rdoc=(struct ds_info *)nmem_malloc(nmem,sizeof(*rs->rdoc));
    rs->rdoc->terms=(struct ts_info *)nmem_malloc(nmem,
                         sizeof(*rs->rdoc->terms)*rs->veclen);
    rs->rdoc->veclen=veclen;
    rs->rdoc->d_f_max=10; /* just a guess */
    rs->rdoc->d_f_max_str=""; 
    /* yaz_log(log_level, "zv_begin_init"); */
    for (i = 0; i < rs->veclen; i++)
    {
        gocc= rset_count(terms[i]->rset);
        terms[i]->rankpriv=ip=nmem_malloc(nmem, sizeof(int));
        *ip=i; /* save the index for add() */
        /* yaz_log(log_level, "zv_begin_init i=%d gocc=%d", i, gocc); */
        rs->qdoc->terms[i].gocc=gocc;
        rs->qdoc->terms[i].locc=1;  /* assume query has no duplicate terms */
        rs->rdoc->terms[i].gocc=gocc;
        rs->rdoc->terms[i].locc=0;
    }
    (*rs->q_tf_fct)(rs, rs->qdoc); /* we do this once only */
    (*rs->q_idf_fct)(rs, rs->qdoc);
    (*rs->q_norm_fct)(rs, rs->qdoc);
    return rs;
}

/*
 * zv_end: Terminates ranking process. Called after a result set
 *  has been ranked.
 */
static void zv_end (struct zebra_register *reg, void *rsi)
{
    yaz_log(log_level, "zv_end");
    /* they all are nmem'd */
    return;
}

/*
 * zv_add: Called for each word occurence in a result set. This routine
 *  should be as fast as possible. This routine should "incrementally"
 *  update the score.
 */
static void zv_add (void *rsi, int seqno, TERMID term) {
    RS rs=(RS)rsi;
    int *ip = term->rankpriv;
    int i=*ip;
    if (!term) 
    {
        yaz_log(log_level, "zvrank zv_add seqno=%d NULL term",seqno );
        return;
    }
    rs->rdoc->terms[i].locc++;
    yaz_log(log_level, "zvrank zv_add seqno=%d '%s' term_index=%d cnt=%d", 
             seqno, term->name, i, rs->rdoc->terms[i].locc );
}

/*
 * zv_calc: Called for each document in a result. This handler should 
 *  produce a score based on previous call(s) to the add handler. The
 *  score should be between 0 and 1000. If score cannot be obtained
 *  -1 should be returned.
 */
static int zv_calc (void *rsi, zint sysno, zint staticrank)
{
    int i, veclen; 
    int score=0;
    double dscore=0.0;
    RS rs=(RS)rsi;
    /* yaz_log(log_level, "zv_calc"); */
    /**/
    veclen=rs->veclen;
    if (veclen==0)
        return -1;
    for (i = 0; i < veclen; i++) {
        /* qdoc weight has already been calculated */
        (*rs->d_tf_fct)(rs, rs->rdoc);
        (*rs->d_idf_fct)(rs, rs->rdoc);
        (*rs->d_norm_fct)(rs, rs->rdoc);
        dscore=rs->sim_fct(rs->qdoc, rs->rdoc);
    }
    score = (int) (dscore * 1000 +.5);
    yaz_log (log_level, "zv_calc: sysno=" ZINT_FORMAT " score=%d", 
            sysno, score);
    if (score > 1000) /* should not happen */
        score = 1000;
    /* reset counts for the next record */
    for (i = 0; i < rs->veclen; i++)
        rs->rdoc->terms[i].locc=0;
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
    "zvrank",
    zv_create,
    zv_destroy,
    zv_begin,
    zv_end,
    zv_calc,
    zv_add,
};
 
struct rank_control *rank_zv_class = &rank_control_vsm;

