/* $Id: rset.h,v 1.33 2004-09-01 15:01:32 heikki Exp $
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
#include <isamb.h> 
#include <isamc.h> 
#include <isams.h> 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rsfd *RSFD; /* Rset "file descriptor" */
typedef struct rset *RSET; /* Result set */

struct rsfd {  /* the stuff common to all rsfd's. */
    RSET rset;  /* ptr to the rset this FD is opened to */
    void *priv; /* private parameters for this type */
    RSFD next;  /* to keep lists of used/free rsfd's */
};


struct rset_control
{
    char *desc; /* text description of set type (for debugging) */
/* RSET rs_something_create(const struct rset_control *sel, ...); */
    void (*f_delete)(RSET ct);
    RSFD (*f_open)(RSET ct, int wflag);
    void (*f_close)(RSFD rfd);
    void (*f_rewind)(RSFD rfd);
    int (*f_forward)(RSFD rfd, void *buf, const void *untilbuf);
    void (*f_pos)(RSFD rfd, double *current, double *total);
       /* returns -1,-1 if pos function not implemented for this type */
    int (*f_read)(RSFD rfd, void *buf);
    int (*f_write)(RSFD rfd, const void *buf);
};

int rset_default_forward(RSFD rfd, void *buf, 
                     const void *untilbuf);

struct key_control {
    int key_size;
    int (*cmp) (const void *p1, const void *p2);
    void (*key_logdump_txt) (int logmask, const void *p, const char *txt);
    zint (*getseq)(const void *p);
      /* FIXME - Should not need a getseq, it won't make much sense with */
      /* higher-order keys. Use a (generalized) cmp instead, or something */
    /* FIXME - decode and encode, and lots of other stuff */
};

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
} rset;
/* rset is a "virtual base class", which will never exist on its own */
/* all instances are rsets of some specific type, like rsisamb, or rsbool */
/* They keep their own stuff behind the priv pointer. */

RSFD rfd_create_base(RSET rs);
void rfd_delete_base(RSFD rfd);

RSET rset_create_base(const struct rset_control *sel, 
                      NMEM nmem,
                      const struct key_control *kcontrol);
void rset_delete(RSET rs);
RSET rset_dup (RSET rs);


#define RSETF_READ       0
#define RSETF_WRITE      1
/* RSFD rset_open(RSET rs, int wflag); */
#define rset_open(rs, wflag) (*(rs)->control->f_open)((rs), (wflag))

/* void rset_close(RSFD rfd); */
#define rset_close(rfd) (*(rfd)->rset->control->f_close)(rfd)

/* void rset_rewind(RSFD rfd); */
#define rset_rewind(rfd) (*(rfd)->rset->control->f_rewind)((rfd))

/* int rset_forward(RSFD rfd, void *buf, void *untilbuf); */
#define rset_forward(rfd, buf, untilbuf) \
    (*(rfd)->rset->control->f_forward)((rfd),(buf),(untilbuf))

/* int rset_pos(RSFD fd, double *current, double *total); */
#define rset_pos(rfd,cur,tot) \
    (*(rfd)->rset->control->f_pos)( (rfd),(cur),(tot))

/* int rset_read(RSFD rfd, void *buf); */
#define rset_read(rfd, buf) (*(rfd)->rset->control->f_read)((rfd), (buf))

/* int rset_write(RSFD rfd, const void *buf); */
#define rset_write(rfd, buf) (*(rfd)->rset->control->f_write)((rfd), (buf))

/* int rset_type (RSET) */
#define rset_type(rs) ((rs)->control->desc)

RSET rstemp_create( NMEM nmem, const struct key_control *kcontrol,
                    const char *temp_path);

RSET rsnull_create(NMEM nmem, const struct key_control *kcontrol);

RSET rsbool_create_and( NMEM nmem, const struct key_control *kcontrol,
                        RSET rset_l, RSET rset_r);

RSET rsbool_create_or ( NMEM nmem, const struct key_control *kcontrol,
                        RSET rset_l, RSET rset_r);

RSET rsbool_create_not( NMEM nmem, const struct key_control *kcontrol,
                        RSET rset_l, RSET rset_r);

RSET rsbetween_create( NMEM nmem, const struct key_control *kcontrol,
                        RSET rset_l, RSET rset_m, RSET rset_r, 
                        RSET rset_attr);

RSET rsmultior_create( NMEM nmem, const struct key_control *kcontrol,
                      int no_rsets, RSET* rsets);

RSET rsprox_create( NMEM nmem, const struct key_control *kcontrol,
                    int rset_no, RSET *rset,
                    int ordered, int exclusion,
                    int relation, int distance);

RSET rsisamb_create( NMEM nmem, const struct key_control *kcontrol,
                     ISAMB is, ISAMB_P pos);

RSET rsisamc_create( NMEM nmem, const struct key_control *kcontrol,
                     ISAMC is, ISAMC_P pos);

RSET rsisams_create( NMEM nmem, const struct key_control *kcontrol,
                     ISAMS is, ISAMS_P pos);



#ifdef __cplusplus
}
#endif

#endif
