/* $Id: rset.h,v 1.23.2.1 2006-08-14 10:38:56 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#ifndef RSET_H
#define RSET_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void *RSFD;

typedef struct rset *RSET;
typedef struct rset_term *RSET_TERM;

struct rset_control
{
    char *desc; /* text description of set type (for debugging) */
    void *(*f_create)(RSET ct, const struct rset_control *sel, void *parms);
    RSFD (*f_open)(RSET ct, int wflag);
    void (*f_close)(RSFD rfd);
    void (*f_delete)(RSET ct);
    void (*f_rewind)(RSFD rfd);
    int (*f_forward)(RSET ct, RSFD rfd, void *buf,  int *term_index,
                     int (*cmpfunc)(const void *p1, const void *p2), 
                     const void *untilbuf);
    void (*f_pos)(RSFD rfd, int *current, int *total);
       /* FIXME - Should be 64-bit ints !*/
       /* returns -1,-1 if pos function not implemented for this type */
    int (*f_read)(RSFD rfd, void *buf, int *term_index);
    int (*f_write)(RSFD rfd, const void *buf);
};

int rset_default_forward(RSET ct, RSFD rfd, void *buf, int *term_index, 
                     int (*cmpfunc)(const void *p1, const void *p2), 
                     const void *untilbuf);
void rset_default_pos(RSFD rfd, int *current, int *total);

struct rset_term {
    char *name;
    int  nn;
    char *flags;
    int  count;
    int  type;
};

typedef struct rset
{
    const struct rset_control *control;
    int  flags;
    int  count;
    void *buf;
    RSET_TERM *rset_terms;
    int no_rset_terms;
} rset;

RSET_TERM rset_term_create (const char *name, int length, const char *flags,
                            int type);
void rset_term_destroy (RSET_TERM t);
RSET_TERM rset_term_dup (RSET_TERM t);

#define RSETF_READ       0
#define RSETF_WRITE      1

RSET rset_create(const struct rset_control *sel, void *parms); 
/* parameters? */

/* int rset_open(RSET rs, int wflag); */
#define rset_open(rs, wflag) (*(rs)->control->f_open)((rs), (wflag))

/* void rset_close(RSET rs); */
#define rset_close(rs, rfd) (*(rs)->control->f_close)(rfd)

void rset_delete(RSET rs);

RSET rset_dup (RSET rs);

/* void rset_rewind(RSET rs); */
#define rset_rewind(rs, rfd) (*(rs)->control->f_rewind)((rfd))

/* int rset_forward(RSET rs, void *buf, int *indx, void *untilbuf); */
#define rset_forward(rs, fd, buf, indx, cmpfunc, untilbuf) \
    (*(rs)->control->f_forward)((rs), (fd), (buf), (indx), (cmpfunc), (untilbuf))

/* int rset_count(RSET rs); */
#define rset_count(rs) (*(rs)->control->f_count)(rs)

/* int rset_read(RSET rs, void *buf, int *indx); */
#define rset_read(rs, fd, buf, indx) (*(rs)->control->f_read)((fd), (buf), indx)

/* int rset_write(RSET rs, const void *buf); */
#define rset_write(rs, fd, buf) (*(rs)->control->f_write)((fd), (buf))

/* int rset_type (RSET) */
#define rset_type(rs) ((rs)->control->desc)

#define RSET_FLAG_VOLATILE 1

#define rset_is_volatile(rs) ((rs)->flags & RSET_FLAG_VOLATILE)

#ifdef __cplusplus
}
#endif

#endif
