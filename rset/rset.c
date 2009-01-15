/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <string.h>
#include <idzebra/util.h>
#include <assert.h>
#include <yaz/nmem.h>
#include <rset.h>

static int log_level = 0;
static int log_level_initialized = 0;

/**
   \brief Common constuctor for RFDs
   \param rs Result set handle.
   
   Creates an rfd. Either allocates a new one, in which case the priv 
   pointer is null, and will have to be filled in, or picks up one 
   from the freelist, in which case the priv is already allocated,
   and presumably everything that hangs from it as well 
*/
RSFD rfd_create_base(RSET rs)
{
    RSFD rnew = rs->free_list;

    if (rnew) 
    {
        rs->free_list = rnew->next;
        assert(rnew->rset==rs);
        yaz_log(log_level, "rfd_create_base (fl): rfd=%p rs=%p fl=%p priv=%p", 
		rnew, rs, rs->free_list, rnew->priv); 
    } 
    else
    {
        rnew = nmem_malloc(rs->nmem, sizeof(*rnew));
	rnew->counted_buf = nmem_malloc(rs->nmem, rs->keycontrol->key_size);
	rnew->priv = 0;
        rnew->rset = rs;
        yaz_log(log_level, "rfd_create_base (new): rfd=%p rs=%p fl=%p priv=%p", 
		rnew, rs, rs->free_list, rnew->priv); 
    }
    rnew->next = rs->use_list;
    rs->use_list = rnew;
    rnew->counted_items = 0;
    return rnew;
}

static void rset_close_int(RSET rs, RSFD rfd)
{
    RSFD *pfd;
    (*rs->control->f_close)(rfd);
    
    yaz_log(log_level, "rfd_delete_base: rfd=%p rs=%p priv=%p fl=%p",
            rfd, rs, rfd->priv, rs->free_list); 
    for (pfd = &rs->use_list; *pfd; pfd = &(*pfd)->next)
	if (*pfd == rfd)
	{
	    *pfd = (*pfd)->next;
	    rfd->next = rs->free_list;
	    rs->free_list = rfd;
	    return;
	}
    yaz_log(YLOG_WARN, "rset_close handle not found. type=%s",
	    rs->control->desc);
}

void rset_set_hits_limit(RSET rs, zint l)
{
    rs->hits_limit = l;
}

/**
   \brief Closes a result set RFD handle
   \param rfd the RFD handle.
*/
void rset_close(RSFD rfd)
{
    RSET rs = rfd->rset;

    if (rs->hits_count == 0)
    {
	TERMID termid;
	char buf[100];

	while(rfd->counted_items <= rs->hits_limit
	      && rset_default_read(rfd, buf, &termid))
	    ;
	
	rs->hits_count = rfd->counted_items;
	yaz_log(log_level, "rset_close rset=%p hits_count=" ZINT_FORMAT
		" hits_limit=" ZINT_FORMAT,
		rs, rs->hits_count, rs->hits_limit);
	rs->hits_approx = 0;
	if (rs->hits_count > rs->hits_limit)
	{
	    double cur, tot;
	    zint est;
	    rset_pos(rfd, &cur, &tot); 
	    if (tot > 0) {
		int i;
		double ratio = cur/tot;
		est = (zint)(0.5 + rs->hits_count / ratio);
		yaz_log(log_level, "Estimating hits (%s) "
			"%0.1f->" ZINT_FORMAT
			"; %0.1f->" ZINT_FORMAT,
			rs->control->desc,
			cur, rs->hits_count,
			tot, est);
		i = 0; /* round to significant digits */
		while (est > rs->hits_round) {
		    est /= 10;
		    i++;
		}
		while (i--)
		    est *= 10;
		rs->hits_count = est;
		rs->hits_approx = 1;
	    }
	}
	yaz_log(log_level, "rset_close p=%p count=" ZINT_FORMAT, rs,
		rs->hits_count);
    }
    rset_close_int(rs, rfd);
}

/**
   \brief Common constuctor for RSETs
   \param sel The interface control handle
   \param nmem The memory handle for it.
   \param kcontrol Key control info (decode, encode, comparison etc)
   \param scope scope for set
   \param term Information about term for it (NULL for none).
   \param no_children number of child rsets (0 for none)
   \param children child rsets (NULL for none).
   
   Creates an rfd. Either allocates a new one, in which case the priv 
   pointer is null, and will have to be filled in, or picks up one 
   from the freelist, in which case the priv is already allocated,
   and presumably everything that hangs from it as well 
*/
RSET rset_create_base(const struct rset_control *sel, 
                      NMEM nmem, struct rset_key_control *kcontrol,
                      int scope, TERMID term,
		      int no_children, RSET *children)
{
    RSET rset;
    assert(nmem);
    if (!log_level_initialized) 
    {
        log_level = yaz_log_module_level("rset");
        log_level_initialized = 1;
    }

    rset = (RSET) nmem_malloc(nmem, sizeof(*rset));
    yaz_log(log_level, "rs_create(%s) rs=%p (nm=%p)", sel->desc, rset, nmem); 
    yaz_log(log_level, " ref_id=%s limit=" ZINT_FORMAT, 
	    (term && term->ref_id ? term->ref_id : "null"),
	    rset->hits_limit);
    rset->nmem = nmem;
    rset->control = sel;
    rset->refcount = 1;
    rset->priv = 0;
    rset->free_list = NULL;
    rset->use_list = NULL;
    rset->hits_count = 0;
    rset->hits_limit = 0;
    rset->hits_round = 1000;
    rset->keycontrol = kcontrol;

    (*kcontrol->inc)(kcontrol);
    rset->scope = scope;
    rset->term = term;
    if (term)
    {
        term->rset = rset;
	rset->hits_limit = term->hits_limit;
    }
    rset->no_children = no_children;
    rset->children = 0;
    if (no_children)
    {
	rset->children = (RSET*)
	    nmem_malloc(rset->nmem, no_children*sizeof(RSET *));
	memcpy(rset->children, children, no_children*sizeof(RSET *));
    }
    return rset;
}

/**
   \brief Destructor RSETs
   \param rs Handle for result set.
   
   Destroys a result set and all its children.
   The f_delete method of control is called for the result set.
*/
void rset_delete(RSET rs)
{
    (rs->refcount)--;
    yaz_log(log_level, "rs_delete(%s), rs=%p, refcount=%d",
            rs->control->desc, rs, rs->refcount); 
    if (!rs->refcount)
    {
	int i;
	if (rs->use_list)
	    yaz_log(YLOG_WARN, "rs_delete(%s) still has RFDs in use",
		    rs->control->desc);
	for (i = 0; i<rs->no_children; i++)
	    rset_delete(rs->children[i]);
        (*rs->control->f_delete)(rs);
	(*rs->keycontrol->dec)(rs->keycontrol);
    }
}

/**
   \brief Test for last use of RFD
   \param rfd RFD handle.
   
   Returns 1 if this RFD is the last reference to it; 0 otherwise.
*/
int rfd_is_last(RSFD rfd)
{
    if (rfd->rset->use_list == rfd && rfd->next == 0)
	return 1;
    return 0;
}

/**
   \brief Duplicate an RSET
   \param rs Handle for result set.
   
   Duplicates a result set by incrementing the reference count to it.
*/
RSET rset_dup (RSET rs)
{
    (rs->refcount)++;
    yaz_log(log_level, "rs_dup(%s), rs=%p, refcount=%d",
            rs->control->desc, rs, rs->refcount); 
    return rs;
}

/** 
    \brief Estimates hit count for result set.
    \param rs Result Set.

    rset_count uses rset_pos to get the total and returns that.
    This is ok for rsisamb/c/s, and for some other rsets, but in case of
    booleans etc it will give bad estimate, as nothing has been read
    from that rset
*/
zint rset_count(RSET rs)
{
    double cur, tot;
    RSFD rfd = rset_open(rs, 0);
    rset_pos(rfd, &cur, &tot);
    rset_close_int(rs, rfd);
    return (zint) tot;
}

/**
   \brief is a getterms function for those that don't have any
   \param ct result set handle
   \param terms array of terms (0..maxterms-1)
   \param maxterms length of terms array
   \param curterm current size of terms array

   If there is a term associated with rset the term is appeneded; otherwise
   the terms array is untouched but curterm is incremented anyway.
*/
void rset_get_one_term(RSET ct, TERMID *terms, int maxterms, int *curterm)
{
    if (ct->term)
    {
        if (*curterm < maxterms)
            terms[*curterm] = ct->term;
	(*curterm)++;
    }
}

struct ord_list *ord_list_create(NMEM nmem)
{
    return 0;
}

struct ord_list *ord_list_append(NMEM nmem, struct ord_list *list,
					int ord)
{
    struct ord_list *n = nmem_malloc(nmem, sizeof(*n));
    n->ord = ord;
    n->next = list;
    return n;
}

struct ord_list *ord_list_dup(NMEM nmem, struct ord_list *list)
{
    struct ord_list *n = ord_list_create(nmem);
    for (; list; list = list->next)
	n = ord_list_append(nmem, n, list->ord);
    return n;
}

void ord_list_print(struct ord_list *list)
{
    for (; list; list = list->next)
        yaz_log(YLOG_LOG, "ord_list %d", list->ord);
}
/**
   \brief Creates a TERMID entry.
   \param name Term/Name buffer with given length
   \param length of term
   \param flags for term
   \param type Term Type, Z_Term_general, Z_Term_characterString,..
   \param nmem memory for term.
   \param ol ord list
   \param reg_type register type
   \param hits_limit limit before counting stops and gets approximate
   \param ref_id supplied ID for term that can be used to identify this
*/
TERMID rset_term_create(const char *name, int length, const char *flags,
			int type, NMEM nmem, struct ord_list *ol,
			int reg_type,
			zint hits_limit, const char *ref_id)

{
    TERMID t;
    yaz_log (log_level, "term_create '%s' %d f=%s type=%d nmem=%p",
            name, length, flags, type, nmem);
    t= (TERMID) nmem_malloc(nmem, sizeof(*t));
    if (!name)
        t->name = NULL;
    else if (length == -1)
        t->name = nmem_strdup(nmem, name);
    else
	t->name = nmem_strdupn(nmem, name, length);
    if (!ref_id)
	t->ref_id = 0;
    else
	t->ref_id = nmem_strdup(nmem, ref_id);
    if (!flags)
        t->flags = NULL;
    else
        t->flags = nmem_strdup(nmem, flags);
    t->hits_limit = hits_limit;
    t->type = type;
    t->reg_type = reg_type;
    t->rankpriv = 0;
    t->rset = 0;
    t->ol = ord_list_dup(nmem, ol);
    return t;
}

int rset_default_read(RSFD rfd, void *buf, TERMID *term)
{
    RSET rset = rfd->rset;
    int rc = (*rset->control->f_read)(rfd, buf, term);
    if (rc > 0)
    {
        int got_scope;
        if (rfd->counted_items == 0)
            got_scope = rset->scope+1;
        else
            got_scope = rset->keycontrol->cmp(buf, rfd->counted_buf);
       
#if 0
        key_logdump_txt(YLOG_LOG, buf, "rset_default_read");
        yaz_log(YLOG_LOG, "rset_scope=%d got_scope=%d", rset->scope, got_scope);
#endif
        if (got_scope > rset->scope)
	{
	    memcpy(rfd->counted_buf, buf, rset->keycontrol->key_size);
	    rfd->counted_items++;
	}
    }
    return rc;
}

int rset_default_forward(RSFD rfd, void *buf, TERMID *term,
			 const void *untilbuf)
{
    RSET rset = rfd->rset;
    int more;

    if (rset->control->f_forward &&
	rfd->counted_items >= rset->hits_limit)
    {
	assert (rset->control->f_forward != rset_default_forward);
	return rset->control->f_forward(rfd, buf, term, untilbuf);
    }
    
    while ((more = rset_read(rfd, buf, term)) > 0)
    {
	if ((rfd->rset->keycontrol->cmp)(untilbuf, buf) <= 1)
	    break;
    }
    if (log_level)
	yaz_log (log_level, "rset_default_forward exiting m=%d c=%d",
		 more, rset->scope);
    
    return more;
}

void rset_visit(RSET rset, int level)
{
    int i;
    yaz_log(YLOG_LOG, "%*s%c " ZINT_FORMAT, level, "",
	    rset->hits_approx ? '~' : '=',
	    rset->hits_count);
    for (i = 0; i<rset->no_children; i++)
	rset_visit(rset->children[i], level+1);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

