/* $Id: rset.h,v 1.43 2004-12-08 14:02:36 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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



#ifndef RSET_H
#define RSET_H

#include <stdlib.h>

/* unfortunately we need the isam includes here, for the arguments for */
/* rsisamX_create */
#include <idzebra/isamb.h> 
#include <idzebra/isamc.h> 
#include <idzebra/isams.h> 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rsfd *RSFD; /* Rset "file descriptor" */
typedef struct rset *RSET; /* Result set */


/** 
 * rset_term is all we need to know of a term to do ranking etc. 
 * As far as the rsets are concerned, it is just a dummy pointer to
 * be passed around.
 */

struct rset_term {
    /** the term itself */
    char *name;
    char *flags;
    int  type;
    /** the rset corresponding to this term */
    RSET rset;
    /** private stuff for the ranking algorithm */
    void *rankpriv;
};

typedef struct rset_term *TERMID; 
TERMID rset_term_create (const char *name, int length, const char *flags,
                                    int type, NMEM nmem);



/** rsfd is a "file descriptor" for reading from a rset */
struct rsfd {  /* the stuff common to all rsfd's. */
    RSET rset;  /* ptr to the rset this FD is opened to */
    void *priv; /* private parameters for this type */
    RSFD next;  /* to keep lists of used/free rsfd's */
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

/** rset_get_no_terms is a getterms function for those that don't have any */
void rset_get_no_terms(RSET ct, TERMID *terms, int maxterms, int *curterm);

/** 
 * rset_get_one_term is a getterms function for those rsets that have
 * exactly one term, like all rsisamX types. 
 */
void rset_get_one_term(RSET ct,TERMID *terms,int maxterms,int *curterm);

/**
 * key_control contains all there is to know about the keys stored in 
 * an isam, and therefore operated by the rsets. Other than this info,
 * all we assume is that all keys are the same size, and they can be
 * memcpy'd around
 */
struct key_control {
    int key_size;
    int scope;  /* default for what level we operate (book/chapter/verse) on*/
                /* usual sysno/seqno is 2 */
    int (*cmp) (const void *p1, const void *p2);
    void (*key_logdump_txt) (int logmask, const void *p, const char *txt);
    zint (*getseq)(const void *p);
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
    const struct key_control *keycontrol;
    int  count;  /* reference count */
    void *priv;  /* stuff private to the given type of rset */
    NMEM nmem;    /* nibble memory for various allocs */
    char my_nmem; /* Should the nmem be destroyed with the rset?  */
                  /* 1 if created with it, 0 if passed from above */
    RSFD free_list; /* all rfd's allocated but not currently in use */
    int scope;    /* On what level do we count hits and compare them? */
    TERMID term; /* the term thing for ranking etc */
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
void rfd_delete_base(RSFD rfd);

RSET rset_create_base(const struct rset_control *sel, 
                      NMEM nmem,
                      const struct key_control *kcontrol,
                      int scope,
                      TERMID term);

void rset_delete(RSET rs);
RSET rset_dup (RSET rs);


#define RSETF_READ       0
#define RSETF_WRITE      1
/* RSFD rset_open(RSET rs, int wflag); */
#define rset_open(rs, wflag) (*(rs)->control->f_open)((rs), (wflag))

/* void rset_close(RSFD rfd); */
#define rset_close(rfd) (*(rfd)->rset->control->f_close)(rfd)

/* int rset_forward(RSFD rfd, void *buf, TERMID term, void *untilbuf); */
#define rset_forward(rfd, buf, term, untilbuf) \
    (*(rfd)->rset->control->f_forward)((rfd),(buf),(term),(untilbuf))

/* void rset_getterms(RSET ct, TERMID *terms, int maxterms, int *curterm); */
#define rset_getterms(ct, terms, maxterms, curterm) \
    (*(ct)->control->f_getterms)((ct),(terms),(maxterms),(curterm))

/* int rset_pos(RSFD fd, double *current, double *total); */
#define rset_pos(rfd,cur,tot) \
    (*(rfd)->rset->control->f_pos)( (rfd),(cur),(tot))

/* int rset_read(RSFD rfd, void *buf, TERMID term); */
#define rset_read(rfd, buf, term) \
    (*(rfd)->rset->control->f_read)((rfd), (buf), (term))

/* int rset_write(RSFD rfd, const void *buf); */
#define rset_write(rfd, buf) (*(rfd)->rset->control->f_write)((rfd), (buf))

/* int rset_type (RSET) */
#define rset_type(rs) ((rs)->control->desc)

/** rset_count counts or estimates the keys in it*/
zint rset_count(RSET rs);

RSET rstemp_create( NMEM nmem, const struct key_control *kcontrol,
                    int scope, 
                    const char *temp_path, TERMID term);

RSET rsnull_create(NMEM nmem, const struct key_control *kcontrol);

RSET rsbool_create_and( NMEM nmem, const struct key_control *kcontrol,
                        int scope, 
                        RSET rset_l, RSET rset_r);

RSET rsbool_create_or ( NMEM nmem, const struct key_control *kcontrol,
                        int scope,
                        RSET rset_l, RSET rset_r);

RSET rsbool_create_not( NMEM nmem, const struct key_control *kcontrol,
                        int scope,
                        RSET rset_l, RSET rset_r);

RSET rsbetween_create(  NMEM nmem, const struct key_control *kcontrol,
                        int scope, 
                        RSET rset_l, RSET rset_m, RSET rset_r, 
                        RSET rset_attr);

RSET rsmultior_create(  NMEM nmem, const struct key_control *kcontrol,
                        int scope, 
                        int no_rsets, RSET* rsets);

RSET rsmultiand_create( NMEM nmem, const struct key_control *kcontrol,
                        int scope, 
                        int no_rsets, RSET* rsets);

RSET rsprox_create( NMEM nmem, const struct key_control *kcontrol,
                        int scope, 
                    int rset_no, RSET *rset,
                    int ordered, int exclusion,
                    int relation, int distance);

RSET rsisamb_create( NMEM nmem, const struct key_control *kcontrol,
                        int scope, 
                     ISAMB is, ISAMB_P pos,
                     TERMID term);

RSET rsisamc_create( NMEM nmem, const struct key_control *kcontrol,
                        int scope, 
                     ISAMC is, ISAMC_P pos,
                     TERMID term);

RSET rsisams_create( NMEM nmem, const struct key_control *kcontrol,
                        int scope,
                     ISAMS is, ISAMS_P pos,
                     TERMID term);



#ifdef __cplusplus
}
#endif

#endif
