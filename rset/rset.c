/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.c,v $
 * Revision 1.7  1995-10-12 12:41:56  adam
 * Private info (buf) moved from struct rset_control to struct rset.
 * Bug fixes in relevance.
 *
 * Revision 1.6  1995/09/08  14:52:41  adam
 * Work on relevance feedback.
 *
 * Revision 1.5  1995/09/07  13:58:43  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.4  1995/09/06  16:11:56  adam
 * More work on boolean sets.
 *
 * Revision 1.3  1995/09/04  15:20:39  adam
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

    logf (LOG_DEBUG, "rs_create(%s)", sel->desc);
    new = xmalloc(sizeof(*new));
    new->control = sel;
    new->buf = (*sel->f_create)(sel, parms);
    return new;
}

void rset_delete (RSET rs)
{
    (*rs->control->f_delete)(rs);
    xfree(rs);
}
