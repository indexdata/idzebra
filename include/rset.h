/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.h,v $
 * Revision 1.1  1994-11-03 14:13:22  quinn
 * Result set manipulation
 *
 */

#ifndef RSET_H
#define RSET_H

typedef struct rset_control
{
    char *buf;  /* state data stored by subsystem */
    int (*f_open)(rset_control *ct, int wflag);
    void (*f_close)(rset_control *ct *data);
    void (*f_delete)(rset_control *ct);
    void (*f_rewind)(rset_control *ct);
    int (*f_count)(rset_control *ct);
    int (*f_read)(...);
    int (*f_write)(...);
} rset_control;

typedef struct rset
{
    rset_control *control;
} rset *RSET;

RSET rset_create();       /* parameters? */
int rset_open(RSET rs, int wflag);
void rset_close(RSET rs);
void rset_delete(RSET rs);
void rset_rewind(RSET rs);
int rset_count(RSET rs);   /* more parameters? */
int rset_read(RSET rs, void *buf);   /* change parameters */
int rset_write(RSET rs, void *buf);  /* change parameters */

#endif
