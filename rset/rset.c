/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.c,v $
 * Revision 1.1  1994-11-04 13:21:28  quinn
 * Working.
 *
 */

/* TODO: mem management */

#include <util.h>

#include <rset.h>

RSET rset_create(const rset_control *sel, void *parms)
{
    RSET new;

    new = xmalloc(sizeof(*new));     /* make dynamic alloc scheme */
    if (!(new->control = (*sel->f_create)(sel, parms)))
    	return 0;
    return new;
}

void rset_delete(RSET rs)
{
    if (rs->is_open)
    	rset_close(rs);
    (*rs->control->f_delete)(rs->control);
    xfree(rs);
}
