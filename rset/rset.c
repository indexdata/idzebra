/* $Id: rset.c,v 1.40 2004-11-19 10:27:14 heikki Exp $
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

static int log_level=0;
static int log_level_initialized=0;

/**
 * creates an rfd. Either allocates a new one, in which case the priv 
 * pointer is null, and will have to be filled in, or picks up one 
 * from the freelist, in which case the priv is already allocated,
 * and presumably everything that hangs from it as well 
 */

RSFD rfd_create_base(RSET rs)
{
    if (!log_level_initialized) 
    {
        log_level=yaz_log_module_level("rset");
        log_level_initialized=1;
    }

    RSFD rnew=rs->free_list;
    if (rnew) 
    {
        rs->free_list=rnew->next;
        assert(rnew->rset==rs);
        yaz_log(log_level,"rfd-create_base (fl): rfd=%p rs=%p fl=%p priv=%p", 
                       rnew, rs, rs->free_list, rnew->priv); 
    } else {
        rnew=nmem_malloc(rs->nmem, sizeof(*rnew));
        rnew->priv=NULL;
        rnew->rset=rs;
        yaz_log(log_level,"rfd_create_base (new): rfd=%p rs=%p fl=%p priv=%p", 
                       rnew, rs, rs->free_list, rnew->priv); 
    }
    rnew->next=NULL; /* not part of any (free?) list */
    return rnew;
}

/* puts an rfd into the freelist of the rset. Only when the rset gets */
/* deleted, will all the nmem disappear */
void rfd_delete_base(RSFD rfd) 
{
    RSET rs=rfd->rset;
    yaz_log(log_level,"rfd_delete_base: rfd=%p rs=%p priv=%p fl=%p",
            rfd, rs, rfd->priv, rs->free_list); 
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
    yaz_log (log_level, "rs_create(%s) rs=%p (nm=%p)", sel->desc, rnew, nmem); 
    rnew->nmem=M;
    if (nmem)
        rnew->my_nmem=0;
    else 
        rnew->my_nmem=1;
    rnew->control = sel;
    rnew->count = 1; /* refcount! */
    rnew->priv = 0;
    rnew->free_list=NULL;
    rnew->keycontrol=kcontrol;
    rnew->scope=scope;
    rnew->term=term;
    if (term)
        term->rset=rnew;
    return rnew;
}

void rset_delete (RSET rs)
{
    (rs->count)--;
    yaz_log(log_level,"rs_delete(%s), rs=%p, count=%d",
            rs->control->desc, rs, rs->count); 
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
    yaz_log(log_level,"rs_dup(%s), rs=%p, count=%d",
            rs->control->desc, rs, rs->count); 
    return rs;
}

int rset_default_forward(RSFD rfd, void *buf, TERMID *term,
                           const void *untilbuf)
{
    int more=1;
    int cmp=rfd->rset->scope;
    if (log_level)
    {
        yaz_log (log_level, "rset_default_forward starting '%s' (ct=%p rfd=%p)",
                    rfd->rset->control->desc, rfd->rset, rfd);
        /* key_logdump(log_level, untilbuf); */
    }
    while ( (cmp>=rfd->rset->scope) && (more))
    {
        if (log_level)  /* time-critical, check first */
            yaz_log(log_level,"rset_default_forward looping m=%d c=%d",more,cmp);
        more=rset_read(rfd, buf, term);
        if (more)
            cmp=(rfd->rset->keycontrol->cmp)(untilbuf,buf);
/*        if (more)
            key_logdump(log_level,buf); */
    }
    if (log_level)
        yaz_log (log_level, "rset_default_forward exiting m=%d c=%d",more,cmp);

    return more;
}

/** 
 * rset_count uses rset_pos to get the total and returns that.
 * This is ok for rsisamb/c/s, and for some other rsets, but in case of
 * booleans etc it will give bad estimate, as nothing has been read
 * from that rset
 */
zint rset_count(RSET rs)
{
    double cur,tot;
    RSFD rfd=rset_open(rs,0);
    rset_pos(rfd,&cur,&tot);
    rset_close(rfd);
    return (zint)(tot);
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
    yaz_log (log_level, "term_create '%s' %d f=%s type=%d nmem=%p",
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
    t->type = type;
    t->rankpriv=0;
    t->rset=0;
    return t;
}


