/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.h,v $
 * Revision 1.7  1995-09-07 13:58:08  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.6  1995/09/06  16:10:58  adam
 * More work on boolean sets.
 *
 * Revision 1.5  1995/09/04  15:20:13  adam
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

typedef void *RSFD;

typedef struct rset_control
{
    char *desc; /* text description of set type (for debugging) */
    void *buf;  /* state data stored by subsystem */
    struct rset_control *(*f_create)(const struct rset_control *sel, void *parms);
    RSFD (*f_open)(struct rset_control *ct, int wflag);
    void (*f_close)(RSFD rfd);
    void (*f_delete)(struct rset_control *ct);
    void (*f_rewind)(RSFD rfd);
    int (*f_count)(struct rset_control *ct);
    int (*f_read)(RSFD rfd, void *buf);
    int (*f_write)(RSFD rfd, const void *buf);
} rset_control;

typedef struct rset
{
    rset_control *control;
} rset, *RSET;

RSET rset_create(const rset_control *sel, void *parms);       /* parameters? */

/* int rset_open(RSET rs, int wflag); */
#define rset_open(rs, wflag) ((*(rs)->control->f_open)((rs)->control, (wflag)))

/* void rset_close(RSET rs); */
#define rset_close(rs, rfd) ((*(rs)->control->f_close)((rfd)))

void rset_delete(RSET rs);

/* void rset_rewind(RSET rs); */
#define rset_rewind(rs, rfd) ((*(rs)->control->f_rewind)((rfd)))

/* int rset_count(RSET rs); */
#define rset_count(rs) ((*(rs)->control->f_count)((rs)->control))

/* int rset_read(RSET rs, void *buf); */
#define rset_read(rs, fd, buf) ((*(rs)->control->f_read)((fd), (buf)))

/* int rset_write(RSET rs, const void *buf); */
#define rset_write(rs, fd, buf) ((*(rs)->control->f_write)((fd), (buf)))

#endif
