/* $Id: rset.c,v 1.28 2004-08-24 15:00:16 heikki Exp $
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
#include <yaz/nmem.h>
#include <rset.h>

RSET rset_create_base(const struct rset_control *sel, NMEM nmem)
        /* FIXME - Add keysize and cmp function */
        /* FIXME - Add a general key-func block for cmp, dump, etc */
{
    RSET rnew;
    NMEM M;
    logf (LOG_DEBUG, "rs_create(%s)", sel->desc);
    if (nmem) 
        M=nmem;
    else
        M=nmem_create();
    rnew = (RSET) nmem_malloc(M,sizeof(*rnew));
    rnew->nmem=M;
    if (nmem)
        rnew->my_nmem=0;
    else 
        rnew->my_nmem=1;
    rnew->control = sel;
    rnew->count = 1;
    rnew->priv = 0;
    
    return rnew;
}

void rset_delete (RSET rs)
{
    (rs->count)--;
    if (!rs->count)
    {
        (*rs->control->f_delete)(rs);
        if (rs->my_nmem)
            nmem_destroy(rs->nmem);
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

