/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.c,v $
 * Revision 1.9  1996-10-29 13:55:21  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.8  1995/12/11 09:15:23  adam
 * New set types: sand/sor/snot - ranked versions of and/or/not in
 * ranked/semi-ranked result sets.
 * Note: the snot not finished yet.
 * New rset member: flag.
 * Bug fix: r_delete in rsrel.c did free bad memory block.
 *
 * Revision 1.7  1995/10/12  12:41:56  adam
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
#include <zebrautl.h>

#include <rset.h>

RSET rset_create(const rset_control *sel, void *parms)
{
    RSET rnew;

    logf (LOG_DEBUG, "rs_create(%s)", sel->desc);
    rnew = xmalloc(sizeof(*rnew));
    rnew->control = sel;
    rnew->flags = 0;
    rnew->buf = (*sel->f_create)(sel, parms, &rnew->flags);
    return rnew;
}

void rset_delete (RSET rs)
{
    (*rs->control->f_delete)(rs);
    xfree(rs);
}
