/* $Id: rset.c,v 1.26 2004-08-23 12:38:53 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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



#include <stdio.h>
#include <string.h>
#include <zebrautl.h>
#include <assert.h>

#include <rset.h>
/* #include <../index/index.h> */ /* for log_keydump. Debugging only */

RSET rset_create(const struct rset_control *sel, void *parms)
{
    RSET rnew;

    logf (LOG_DEBUG, "rs_create(%s)", sel->desc);
    rnew = (RSET) xmalloc(sizeof(*rnew));
    rnew->control = sel;
    rnew->count = 1;
    rnew->buf = (*sel->f_create)(rnew, sel, parms);
    return rnew;
}

void rset_delete (RSET rs)
{
    (rs->count)--;
    if (!rs->count)
    {
        (*rs->control->f_delete)(rs);
        xfree(rs);
    }
}

RSET rset_dup (RSET rs)
{
    (rs->count)++;
    return rs;
}

void rset_default_pos (RSFD rfd, double *current, double *total)
{ /* This should never really be needed, but it is still used in */
  /* those rsets that we don't really plan to use, like isam-s */
    assert(rfd);
    assert(current);
    assert(total);
    *current=-1; /* signal that pos is not implemented */
    *total=-1;
} /* rset_default_pos */

int rset_default_forward(RSET ct, RSFD rfd, void *buf, 
                           int (*cmpfunc)(const void *p1, const void *p2), 
                           const void *untilbuf)
{
    int more=1;
    int cmp=2;
    logf (LOG_DEBUG, "rset_default_forward starting '%s' (ct=%p rfd=%p)",
                    ct->control->desc, ct,rfd);
    /* key_logdump(LOG_DEBUG, untilbuf); */
    while ( (cmp==2) && (more))
    {
        logf (LOG_DEBUG, "rset_default_forward looping m=%d c=%d",more,cmp);
        more=rset_read(ct, rfd, buf);
        if (more)
            cmp=(*cmpfunc)(untilbuf,buf);
/*        if (more)
            key_logdump(LOG_DEBUG,buf); */
    }
    logf (LOG_DEBUG, "rset_default_forward exiting m=%d c=%d",more,cmp);

    return more;
}

