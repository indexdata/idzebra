/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.h,v $
 * Revision 1.2  1994-11-04 13:21:21  quinn
 * Working.
 *
 * Revision 1.1  1994/11/03  14:13:22  quinn
 * Result set manipulation
 *
 */

#ifndef RSET_H
#define RSET_H

typedef struct rset_control
{
    char *desc; /* text description of set type (for debugging) */
    char *buf;  /* state data stored by subsystem */
    struct rset_control *(*f_create)(const struct rset_control *sel, void *parms);
    int (*f_open)(struct rset_control *ct, int wflag);
    void (*f_close)(struct rset_control *ct);
    void (*f_delete)(struct rset_control *ct);
    void (*f_rewind)(struct rset_control *ct);
    int (*f_count)(struct rset_control *ct);
    int (*f_read)(struct rset_control *ct, void *buf);
    int (*f_write)(struct rset_control *ct, const void *buf);
} rset_control;

typedef struct rset
{
    int is_open;
    rset_control *control;
} rset, *RSET;

RSET rset_create(const rset_control *sel, void *parms);       /* parameters? */

/* int rset_open(RSET rs, int wflag); */
#define rset_open(rs, wflag) ((*(rs)->control->f_open)((rs)->control, (wflag)))

/* void rset_close(RSET rs); */
#define rset_close(rs) ((*(rs)->control->f_close)((rs)->control))

void rset_delete(RSET rs);

/* void rset_rewind(RSET rs); */
#define rset_rewind(rs, wflag) ((*(rs)->control->f_rewind)((rs)->control))

/* int rset_count(RSET rs); */
#define rset_count(rs, wflag) ((*(rs)->control->f_count)((rs)->control))

int rset_read(RSET rs, void *buf);   /* change parameters */
int rset_write(RSET rs, void *buf);  /* change parameters */

#endif
