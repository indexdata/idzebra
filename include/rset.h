/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef RSET_H
#define RSET_H

#include <yaz/yaz-util.h>
/* unfortunately we need the isam includes here, for the arguments for */
/* rsisamX_create */
#include <idzebra/isamb.h> 
#include <idzebra/isamc.h> 
#include <idzebra/isams.h> 

YAZ_BEGIN_CDECL

typedef struct rsfd *RSFD;
typedef struct rset *RSET;

struct ord_list {
    int ord;
    struct ord_list *next;
};

struct ord_list *ord_list_create(NMEM nmem);
struct ord_list *ord_list_append(NMEM nmem, struct ord_list *list, int ord);
struct ord_list *ord_list_dup(NMEM nmem, struct ord_list *list);

/** 
 * rset_term is all we need to know of a term to do ranking etc. 
 * As far as the rsets are concerned, it is just a dummy pointer to
 * be passed around.
 */
struct rset_term {
    char *name;    /** the term itself in internal encoding (UTF-8/raw) */
    char *flags;   /** flags for rank method */
    int  type;     /** Term_type from RPN Query. Actually this
		       is Z_Term_general, Z_Term_numeric,
		       Z_Term_characterString, ..
		       This info is used to return encoded term back for
		       search-result-1 .
		   */
    int reg_type;  /** register type */
    RSET rset;     /** the rset corresponding to this term */
    void *rankpriv;/** private stuff for the ranking algorithm */
    zint hits_limit;/** limit for hits if > 0 */
    char *ref_id;  /** reference for this term */
    struct ord_list *ol;
};

typedef struct rset_term *TERMID; 
TERMID rset_term_create (const char *name, int length, const char *flags,
			 int type, NMEM nmem, struct ord_list *ol,
			 int reg_type, zint hits_limit, const char *ref_id);

/** rsfd is a "file descriptor" for reading from a rset */
struct rsfd {  /* the stuff common to all rsfd's. */
    RSET rset;  /* ptr to the rset this FD is opened to */
    void *priv; /* private parameters for this type */
    RSFD next;  /* to keep lists of used/free rsfd's */
    zint counted_items;
    char *counted_buf;
};


/** 
 * rset_control has function pointers to all the important functions
 * of a rset. Each type of rset will have its own control block, pointing
 * to the functions for that type. They all have their own create function
 * which is not part of the control block, as it takes different args for
 * each type.
 */
struct rset_control
{
    /** text description of set type (for debugging) */
    char *desc; 
/* RSET rs_something_create(const struct rset_control *sel, ...); */
    void (*f_delete)(RSET ct);

    /** recursively fills the terms array with terms. call with curterm=0 */
    /* always counts them all into cur, but of course won't touch the term */
    /* array past max. You can use this to count, set max=0 */
    void (*f_getterms)(RSET ct, TERMID *terms, int maxterms, int *curterm);

    RSFD (*f_open)(RSET ct, int wflag);
    void (*f_close)(RSFD rfd);
    /** forward behaves like a read, but it skips some items first */
    int (*f_forward)(RSFD rfd, void *buf, TERMID *term, const void *untilbuf);
    void (*f_pos)(RSFD rfd, double *current, double *total);
       /* returns -1,-1 if pos function not implemented for this type */
    int (*f_read)(RSFD rfd, void *buf, TERMID *term);
    int (*f_write)(RSFD rfd, const void *buf);
};

/** rset_default_forward implements a generic forward with a read-loop */
int rset_default_forward(RSFD rfd, void *buf, TERMID *term,
                     const void *untilbuf);

/** rset_default_read implements a generic read */
int rset_default_read(RSFD rfd, void *buf, TERMID *term);

void rset_get_one_term(RSET ct,TERMID *terms,int maxterms,int *curterm);

/**
 * key_control contains all there is to know about the keys stored in 
 * an isam, and therefore operated by the rsets. Other than this info,
 * all we assume is that all keys are the same size, and they can be
 * memcpy'd around
 */
struct rset_key_control {
    void *context;
    int key_size;
    int scope;  /* default for what level we operate (book/chapter/verse) on*/
                /* usual sysno/seqno is 2 */
    int (*cmp)(const void *p1, const void *p2);
    void (*key_logdump_txt) (int logmask, const void *p, const char *txt);
    zint (*getseq)(const void *p);
    zint (*get_segment)(const void *p);
    int (*filter_func)(const void *p, void *data);
    void *filter_data;
    void (*inc)(struct rset_key_control *kc);
    void (*dec)(struct rset_key_control *kc);
    /* FIXME - Should not need a getseq, it won't make much sense with */
    /* higher-order keys. Use a (generalized) cmp instead, or something */
    /* FIXME - decode and encode, and lots of other stuff */
};

/**
 * A rset is an ordered sequence of keys, either directly from an underlaying
 * isam, or from one of the higher-level operator rsets (and, or, ...).
 * Actually, it is "virtual base class", no pure rsets exist in the system,
 * they all are of some derived type.
 */
typedef struct rset
{
    const struct rset_control *control;
    struct rset_key_control *keycontrol;
    int  refcount;   /* reference count */
    void *priv;      /* stuff private to the given type of rset */
    NMEM nmem;       /* nibble memory for various allocs */
    RSFD free_list;  /* all rfd's allocated but not currently in use */
    RSFD use_list;   /* all rfd's in use */
    int scope;       /* On what level do we count hits and compare them? */
    TERMID term;     /* the term thing for ranking etc */
    int no_children;
    RSET *children;
    zint hits_limit;
    zint hits_count;
    zint hits_round;
    int hits_approx; 
} rset;
/* rset is a "virtual base class", which will never exist on its own 
 * all instances are rsets of some specific type, like rsisamb, or rsbool
 * They keep their own stuff behind the priv pointer.  */

/* On the old sysno-seqno type isams, the scope was hard-coded to be 2.
 * This means that we count hits on the sysno level, and when matching an
 * 'and', we consider it a match if both term occur within the same sysno.
 * In more complex isams we can specify on what level we wish to do the
 * matching and counting of hits. For example, we can have book / chapter /
 * verse, and a seqno. Scope 2 means then "give me all verses that match",
 * 3 would be chapters, 4 books. 
 * The resolution tells how much of the occurences we need to return. If we 
 * are doing some sort of proximity, we need to get the seqnos of all
 * occurences, whereas if we are only counting hits, we do not need anything
 * below the scope. Again 1 is seqnos, 2 sysnos (or verses), 3 books, etc.
 */

RSFD rfd_create_base(RSET rs);
int rfd_is_last(RSFD rfd);

RSET rset_create_base(const struct rset_control *sel, 
                      NMEM nmem,
		      struct rset_key_control *kcontrol,
                      int scope,
                      TERMID term,
		      int no_children, RSET *children);

void rset_delete(RSET rs);
RSET rset_dup (RSET rs);
void rset_close(RSFD rfd);

#define RSETF_READ       0
#define RSETF_WRITE      1
/* RSFD rset_open(RSET rs, int wflag); */
#define rset_open(rs, wflag) (*(rs)->control->f_open)((rs), (wflag))

/* int rset_forward(RSFD rfd, void *buf, TERMID term, void *untilbuf); */
#define rset_forward(rfd, buf, term, untilbuf) \
    rset_default_forward((rfd), (buf), (term), (untilbuf))

/* void rset_getterms(RSET ct, TERMID *terms, int maxterms, int *curterm); */
#define rset_getterms(ct, terms, maxterms, curterm) \
    (*(ct)->control->f_getterms)((ct),(terms),(maxterms),(curterm))

/* int rset_pos(RSFD fd, double *current, double *total); */
#define rset_pos(rfd,cur,tot) \
    (*(rfd)->rset->control->f_pos)((rfd),(cur),(tot))

/* int rset_read(RSFD rfd, void *buf, TERMID term); */
#define rset_read(rfd, buf, term) rset_default_read((rfd), (buf), (term))

/* int rset_write(RSFD rfd, const void *buf); */
#define rset_write(rfd, buf) (*(rfd)->rset->control->f_write)((rfd), (buf))

/* int rset_type (RSET) */
#define rset_type(rs) ((rs)->control->desc)

/** rset_count counts or estimates the keys in it*/
zint rset_count(RSET rs);

RSET rset_create_temp(NMEM nmem, struct rset_key_control *kcontrol,
                      int scope, const char *temp_path, TERMID term);

RSET rset_create_null(NMEM nmem, struct rset_key_control *kcontrol, TERMID term);

RSET rset_create_not(NMEM nmem, struct rset_key_control *kcontrol,
                     int scope, RSET rset_l, RSET rset_r);

RSET rset_create_between(NMEM nmem, struct rset_key_control *kcontrol,
                         int scope, RSET rset_l, RSET rset_m, RSET rset_r, 
                         RSET rset_attr);

RSET rset_create_or(NMEM nmem, struct rset_key_control *kcontrol,
                          int scope, TERMID termid, int no_rsets, RSET* rsets);

RSET rset_create_and(NMEM nmem, struct rset_key_control *kcontrol,
                     int scope, int no_rsets, RSET* rsets);

RSET rset_create_prox(NMEM nmem, struct rset_key_control *kcontrol,
                      int scope, int rset_no, RSET *rset,
                      int ordered, int exclusion, int relation, int distance);

RSET rsisamb_create(NMEM nmem, struct rset_key_control *kcontrol,
		    int scope, ISAMB is, ISAM_P pos, TERMID term);

RSET rsisamc_create(NMEM nmem, struct rset_key_control *kcontrol,
		    int scope, ISAMC is, ISAM_P pos, TERMID term);

RSET rsisams_create(NMEM nmem, struct rset_key_control *kcontrol,
		    int scope, ISAMS is, ISAM_P pos, TERMID term);

void rset_visit(RSET rset, int level);

void rset_set_hits_limit(RSET rs, zint l);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

