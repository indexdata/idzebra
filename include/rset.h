/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.h,v $
 * Revision 1.5  1995-09-04 15:20:13  adam
 * More work on temp sets. is_open member removed.
 *
 * Revision 1.4  1995/09/04  09:09:52  adam
 * String arg in dict lookup is const.
 * Minor changes.
 *
 * Revision 1.3  1994/11/22  13:15:27  quinn
 * Simple
 *
 * Revision 1.2  1994/11/04  13:21:21  quinn
 * Working.
 *
 * Revision 1.1  1994/11/03  14:13:22  quinn
 * Result set manipulation
 *
 */

#ifndef RSET_H
#define RSET_H

#include <stdlib.h>

typedef struct rset_control
{
    char *desc; /* text description of set type (for debugging) */
    void *buf;  /* state data stored by subsystem */
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

/* int rset_read(RSET rs, void *buf); */
#define rset_read(rs, buf) ((*(rs)->control->f_read)((rs)->control, (buf)))

/* int rset_write(RSET rs, const void *buf); */
#define rset_write(rs, buf) ((*(rs)->control->f_write)((rs)->control, (buf)))

#endif
