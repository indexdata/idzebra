/* $Id: rset.c,v 1.37 2004-10-22 10:58:29 heikki Exp $
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


/* creates an rfd. Either allocates a new one, in which case the priv */
/* pointer is null, and will have to be filled in, or picks up one */
/* from the freelist, in which case the priv is already allocated, */
/* and presumably everything that hangs from it as well */

RSFD rfd_create_base(RSET rs)
{
    RSFD rnew=rs->free_list;
    if (rnew) {
        rs->free_list=rnew->next;
        assert(rnew->rset==rs);
  /*    logf(LOG_DEBUG,"rfd-create_base (fl): rfd=%p rs=%p fl=%p priv=%p", 
                       rnew, rs, rs->free_list, rnew->priv); */
    } else {
        rnew=nmem_malloc(rs->nmem, sizeof(*rnew));
        rnew->priv=NULL;
        rnew->rset=rs;
  /*    logf(LOG_DEBUG,"rfd_create_base (new): rfd=%p rs=%p fl=%p priv=%p", 
                       rnew, rs, rs->free_list, rnew->priv); */
    }
    rnew->next=NULL; /* not part of any (free?) list */
    return rnew;
}

/* puts an rfd into the freelist of the rset. Only when the rset gets */
/* deleted, will all the nmem disappear */
void rfd_delete_base(RSFD rfd) 
{
    RSET rs=rfd->rset;
 /* logf(LOG_DEBUG,"rfd_delete_base: rfd=%p rs=%p priv=%p fl=%p",
            rfd, rs, rfd->priv, rs->free_list); */
    assert(NULL == rfd->next); 
    rfd->next=rs->free_list;
    rs->free_list=rfd;
}


RSET rset_create_base(const struct rset_control *sel, 
                      NMEM nmem, const struct key_control *kcontrol,
                      int scope, TERMID term)
{
    RSET rnew;
    NMEM M;
    /* assert(nmem); */ /* can not yet be used, api/t4 fails */
    if (nmem) 
        M=nmem;
    else
        M=nmem_create();
    rnew = (RSET) nmem_malloc(M,sizeof(*rnew));
 /* logf (LOG_DEBUG, "rs_create(%s) rs=%p (nm=%p)", sel->desc, rnew, nmem); */
    rnew->nmem=M;
    if (nmem)
        rnew->my_nmem=0;
    else 
        rnew->my_nmem=1;
    rnew->control = sel;
    rnew->count = 1;
    rnew->priv = 0;
    rnew->free_list=NULL;
    rnew->keycontrol=kcontrol;
    rnew->scope=scope;
    rnew->term=term;
    return rnew;
}

void rset_delete (RSET rs)
{
    (rs->count)--;
/*  logf(LOG_DEBUG,"rs_delete(%s), rs=%p, count=%d",
            rs->control->desc, rs, rs->count); */
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

#if 0
void rset_default_pos (RSFD rfd, double *current, double *total)
{ /* This should never really be needed, but it is still used in */
  /* those rsets that we don't really plan to use, like isam-s */
    assert(rfd);
    assert(current);
    assert(total);
    *current=-1; /* signal that pos is not implemented */
    *total=-1;
} /* rset_default_pos */
#endif

int rset_default_forward(RSFD rfd, void *buf, TERMID *term,
                           const void *untilbuf)
{
    int more=1;
    int cmp=rfd->rset->scope;
    logf (LOG_DEBUG, "rset_default_forward starting '%s' (ct=%p rfd=%p)",
                    rfd->rset->control->desc, rfd->rset, rfd);
    /* key_logdump(LOG_DEBUG, untilbuf); */
    while ( (cmp>=rfd->rset->scope) && (more))
    {
        logf (LOG_DEBUG, "rset_default_forward looping m=%d c=%d",more,cmp);
        more=rset_read(rfd, buf, term);
        if (more)
            cmp=(rfd->rset->keycontrol->cmp)(untilbuf,buf);
/*        if (more)
            key_logdump(LOG_DEBUG,buf); */
    }
    logf (LOG_DEBUG, "rset_default_forward exiting m=%d c=%d",more,cmp);

    return more;
}

/** rset_get_no_terms is a getterms function for those that don't have any */
void rset_get_no_terms(RSET ct, TERMID *terms, int maxterms, int *curterm)
{
    return;
}

/* rset_get_one_term gets that one term from an rset. Used by rsisamX */
void rset_get_one_term(RSET ct,TERMID *terms,int maxterms,int *curterm)
{
    if (ct->term)
    {
        if (*curterm < maxterms)
            terms[*curterm]=ct->term;
        (*curterm)++;
    }
}


TERMID rset_term_create (const char *name, int length, const char *flags,
                                    int type, NMEM nmem)

{
    TERMID t;
    logf (LOG_DEBUG, "term_create '%s' %d f=%s type=%d nmem=%p",
            name, length, flags, type, nmem);
    t= (TERMID) nmem_malloc (nmem, sizeof(*t));
    if (!name)
        t->name = NULL;
    else if (length == -1)
        t->name = nmem_strdup(nmem,name);
    else
    {
        t->name = (char*) nmem_malloc(nmem,length+1);
        memcpy (t->name, name, length);
        t->name[length] = '\0';
    }
    if (!flags)
        t->flags = NULL;
    else
        t->flags = nmem_strdup(nmem,flags);
    t->nn = -1;
    t->count = 0;
    t->type = type;
    return t;
}


