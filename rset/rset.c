/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.c,v $
 * Revision 1.3  1995-09-04 15:20:39  adam
 * More work on temp sets. is_open member removed.
 *
 * Revision 1.2  1995/09/04  12:33:56  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1994/11/04  13:21:28  quinn
 * Working.
 *
 */

#include <stdio.h>
#include <alexutil.h>

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
    rset_close(rs);
    (*rs->control->f_delete)(rs->control);
    xfree(rs);
}
