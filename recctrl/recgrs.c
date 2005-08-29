/* $Id: recgrs.c,v 1.86.2.4 2005-08-29 12:58:19 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003
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
#include <assert.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <yaz/log.h>
#include <yaz/oid.h>

#include <recctrl.h>
#include "grsread.h"

#define GRS_MAX_WORD 512

struct grs_handler {
    RecTypeGrs type;
    void *clientData;
    int initFlag;
    struct grs_handler *next;
};

struct grs_handlers {
    struct grs_handler *handlers;
};

static int read_grs_type (struct grs_handlers *h,
			  struct grs_read_info *p, const char *type,
			  data1_node **root)
{
    struct grs_handler *gh = h->handlers;
    const char *cp = strchr (type, '.');

    if (cp == NULL || cp == type)
    {
        cp = strlen(type) + type;
        *p->type = 0;
    }
    else
        strcpy (p->type, cp+1);
    for (gh = h->handlers; gh; gh = gh->next)
    {
        if (!memcmp (type, gh->type->type, cp-type) && 
	    gh->type->type[cp-type] == '\0')
	{
	    if (!gh->initFlag)
	    {
		gh->initFlag = 1;
		gh->clientData = (*gh->type->init)();
	    }
	    p->clientData = gh->clientData;
            *root = (gh->type->read)(p);
	    gh->clientData = p->clientData;
	    return 0;
	}
    }
    return 1;
}

static void grs_add_handler (struct grs_handlers *h, RecTypeGrs t)
{
    struct grs_handler *gh = (struct grs_handler *) xmalloc (sizeof(*gh));
    gh->next = h->handlers;
    h->handlers = gh;
    gh->initFlag = 0;
    gh->clientData = 0;
    gh->type = t;
}

static void *grs_init(RecType recType)
{
    struct grs_handlers *h = (struct grs_handlers *) xmalloc (sizeof(*h));
    h->handlers = 0;

    grs_add_handler (h, recTypeGrs_sgml);
    grs_add_handler (h, recTypeGrs_regx);
#if HAVE_TCL_H
    grs_add_handler (h, recTypeGrs_tcl);
#endif
    grs_add_handler (h, recTypeGrs_marc);
    grs_add_handler (h, recTypeGrs_marcxml);
#if HAVE_EXPAT_H
    grs_add_handler (h, recTypeGrs_xml);
#endif
#if HAVE_PERL
    grs_add_handler (h, recTypeGrs_perl);
#endif
    grs_add_handler (h, recTypeGrs_danbib);
    return h;
}

static void grs_destroy(void *clientData)
{
    struct grs_handlers *h = (struct grs_handlers *) clientData;
    struct grs_handler *gh = h->handlers, *gh_next;
    while (gh)
    {
	gh_next = gh->next;
	if (gh->initFlag)
	    (*gh->type->destroy)(gh->clientData);
	xfree (gh);
	gh = gh_next;
    }
    xfree (h);
}

struct source_parser {
    int len;
    const char *tok;
    const char *src;
    int lookahead;
};

static int sp_lex(struct source_parser *sp)
{
    while (*sp->src == ' ')
	(sp->src)++;
    sp->tok = sp->src;
    sp->len = 0;
    while (*sp->src && !strchr("<>();,-: ", *sp->src))
    {
	sp->src++;
	sp->len++;
    }
    if (sp->len)
	sp->lookahead = 't';
    else
    {
	sp->lookahead = *sp->src;
	if (*sp->src)
	    sp->src++;
    }
    return sp->lookahead;
}


static int sp_expr(struct source_parser *sp, data1_node *n, RecWord *wrd)
{
    if (sp->lookahead != 't')
	return 0;
    if (sp->len == 4 && !memcmp(sp->tok, "data", sp->len))
    {
	if (n->which == DATA1N_data)
	{
	    wrd->string = n->u.data.data;
	    wrd->length = n->u.data.len;
	}
	sp_lex(sp);
    }
    else if (sp->len == 3 && !memcmp(sp->tok, "tag", sp->len))
    {
	if (n->which == DATA1N_tag)
	{		
	    wrd->string = n->u.tag.tag;
	    wrd->length = strlen(n->u.tag.tag);
	}
	sp_lex(sp);
    }
    else if (sp->len == 4 && !memcmp(sp->tok, "attr", sp->len))
    {
	sp_lex(sp);
	if (sp->lookahead != '(')
	    return 0;
	sp_lex(sp);
	if (sp->lookahead != 't')
	    return 0;
	
	if (n->which == DATA1N_tag)
	{
	    data1_xattr *p = n->u.tag.attributes;
	    while (p && strlen(p->name) != sp->len && 
		   memcmp (p->name, sp->tok, sp->len))
		p = p->next;
	    if (p)
	    {
		wrd->string = p->value;
		wrd->length = strlen(p->value);
	    }
	}
	sp_lex(sp);
	if (sp->lookahead != ')')
	    return 0;
	sp_lex(sp);
    }
    else if (sp->len == 5 && !memcmp(sp->tok, "range", sp->len))
    {
	int start, len;
	sp_lex(sp);
	if (sp->lookahead != '(')
	    return 0;
	
	sp_lex(sp);
	sp_expr(sp, n, wrd);
	if (sp->lookahead != ',')
	    return 0;
	
	sp_lex(sp);
	if (sp->lookahead != 't')
	    return 0;
	start = atoi_n(sp->tok, sp->len);
	
	sp_lex(sp);
	if (sp->lookahead != ',')
	    return 0;
	
	sp_lex(sp);
	if (sp->lookahead != 't')
	    return 0;
	len = atoi_n(sp->tok, sp->len);
	
	sp_lex(sp);
	if (sp->lookahead != ')')
	    return 0;
	
	sp_lex(sp);
	if (wrd->string && wrd->length)
	{
	    wrd->string += start;
	    wrd->length -= start;
	    if (wrd->length > len)
		wrd->length = len;
	}
    }
    return 1;
}

static int sp_parse(data1_node *n, RecWord *wrd, const char *src)
{
    struct source_parser sp;
    sp.len = 0;
    sp.tok = 0;
    sp.src = src;
    sp.lookahead = 0;
    sp_lex(&sp);

    return sp_expr(&sp, n, wrd);
}

int d1_check_xpath_predicate(data1_node *n, struct xpath_predicate *p)
{
    int res = 1;
    char *attname;
    data1_xattr *attr;
    
    if (!p) {
        return 1;
    } else {
        if (p->which == XPATH_PREDICATE_RELATION) {
            if (p->u.relation.name[0]) {
                if (*p->u.relation.name != '@') {
                    yaz_log(LOG_WARN, 
                         "  Only attributes (@) are supported in xelm xpath predicates");
                    yaz_log(LOG_WARN, "predicate %s ignored", p->u.relation.name);
                    return (1);
                }
                attname = p->u.relation.name + 1;
                res = 0;
                /* looking for the attribute with a specified name */
                for (attr = n->u.tag.attributes; attr; attr = attr->next) {
                    yaz_log(LOG_DEBUG,"  - attribute %s <-> %s", attname, attr->name );
                    
                    if (!strcmp(attr->name, attname)) {
                        if (p->u.relation.op[0]) {
                            if (*p->u.relation.op != '=') {
                                yaz_log(LOG_WARN, 
                                     "Only '=' relation is supported (%s)",p->u.relation.op);
                                yaz_log(LOG_WARN, "predicate %s ignored", p->u.relation.name);
                                res = 1; break;
                            } else {
                                yaz_log(LOG_DEBUG,"    - value %s <-> %s", 
                                     p->u.relation.value, attr->value );
                                if (!strcmp(attr->value, p->u.relation.value)) {
                                    res = 1; break;
                                } 
                            }
                        } else {
                            /* attribute exists, no value specified */
                            res = 1; break;
                        }
                    }
                }
		yaz_log(LOG_DEBUG, "return %d", res);
                return res;
            } else {
                return 1;
            }
        } 
        else if (p->which == XPATH_PREDICATE_BOOLEAN) {
            if (!strcmp(p->u.boolean.op,"and")) {
                return d1_check_xpath_predicate(n, p->u.boolean.left) 
                    && d1_check_xpath_predicate(n, p->u.boolean.right); 
            }
            else if (!strcmp(p->u.boolean.op,"or")) {
                return (d1_check_xpath_predicate(n, p->u.boolean.left) 
                        || d1_check_xpath_predicate(n, p->u.boolean.right)); 
            } else {
                yaz_log(LOG_WARN, "Unknown boolean relation %s, ignored",p->u.boolean.op);
                return 1;
            }
        }
    }
    return 0;
}

static int dfa_match_first(struct DFA_state **dfaar, const char *text)
{
    struct DFA_state *s = dfaar[0]; /* start state */
    struct DFA_tran *t;
    int i;
    const char *p = text;
    unsigned char c;
    
    for (c = *p++, t = s->trans, i = s->tran_no; --i >= 0; t++)
	if (c >= t->ch[0] && c <= t->ch[1])
	{
	    while (i >= 0)
	    {
		/* move to next state and return if we get a match */
		s = dfaar[t->to];
		if (s->rule_no)
		    return 1;
		/* next char */
		c = *p++;
		for (t = s->trans, i = s->tran_no; --i >= 0; t++)
		    if (c >= t->ch[0] && c <= t->ch[1])
			break;
	    }
	}
    return 0;
}


/* *ostrich*
   
New function, looking for xpath "element" definitions in abs, by
tagpath, using a kind of ugly regxp search.The DFA was built while
parsing abs, so here we just go trough them and try to match
against the given tagpath. The first matching entry is returned.

pop, 2002-12-13

Added support for enhanced xelm. Now [] predicates are considered
as well, when selecting indexing rules... (why the hell it's called
termlist???)

pop, 2003-01-17

*/

data1_termlist *xpath_termlist_by_tagpath(char *tagpath, data1_node *n)
{
    data1_absyn *abs = n->root->u.root.absyn;
    data1_xpelement *xpe = abs->xp_elements;
    data1_node *nn;
#ifdef ENHANCED_XELM 
    struct xpath_location_step *xp;

#endif
    char *pexpr = xmalloc(strlen(tagpath)+5);
    int ok = 0;

    sprintf (pexpr, "/%s\n", tagpath);
    yaz_log(LOG_DEBUG,"Checking tagpath %s",tagpath);
    while (xpe) 
    {
	int i;
	ok = dfa_match_first(xpe->dfa->states, pexpr);
	if (ok)
	    yaz_log(YLOG_DEBUG, " xpath got match %s",xpe->xpath_expr);
	else
	    yaz_log(YLOG_DEBUG, " xpath no match %s",xpe->xpath_expr);

        if (ok) {
#ifdef ENHANCED_XELM 
            /* we have to check the perdicates up to the root node */
            xp = xpe->xpath;
            
            /* find the first tag up in the node structure */
            nn = n; while (nn && nn->which != DATA1N_tag) {
                nn = nn->parent;
            }
            
            /* go from inside out in the node structure, while going
               backwards trough xpath location steps ... */
            for (i=xpe->xpath_len - 1; i>0; i--) {
                
                yaz_log(LOG_DEBUG,"Checking step %d: %s on tag %s",
		     i,xp[i].part,nn->u.tag.tag);
                
                if (!d1_check_xpath_predicate(nn, xp[i].predicate)) {
                    yaz_log(LOG_DEBUG,"  Predicates didn't match");
                    ok = 0;
                    break;
                }
                
                if (nn->which == DATA1N_tag) {
                    nn = nn->parent;
                }
            }
#endif
            if (ok) {
                break;
            }
	}
        xpe = xpe->next;
    } 
    
    xfree(pexpr);
    
    if (ok) {
      yaz_log(LOG_DEBUG,"Got it");
        return xpe->termlists;
    } else {
        return NULL;
    }
}

/* use
     1   start element (tag)
     2   end element
     3   start attr (and attr-exact)
     4   end attr

  1016   cdata
  1015   attr data

  *ostrich*

  Now, if there is a matching xelm described in abs, for the
  indexed element or the attribute,  then the data is handled according 
  to those definitions...

  modified by pop, 2002-12-13
*/

/* add xpath index for an attribute */
static void index_xpath_attr (char *tag_path, char *name, char *value,
			      char *structure, struct recExtractCtrl *p,
			      RecWord *wrd)
{
    wrd->attrSet = VAL_IDXPATH;
    wrd->attrUse = 1;
    wrd->reg_type = '0';
    wrd->string = tag_path;
    wrd->length = strlen(tag_path);
    (*p->tokenAdd)(wrd);
    
    if (value) {
        wrd->attrUse = 1015;
        wrd->reg_type = 'w';
        wrd->string = value;
        wrd->length = strlen(value);
        (*p->tokenAdd)(wrd);
    }
    
    wrd->attrUse = 2;
    wrd->reg_type = '0';
    wrd->string = tag_path;
    wrd->length = strlen(tag_path);
    (*p->tokenAdd)(wrd);
}


static void index_xpath (data1_node *n, struct recExtractCtrl *p,
                         int level, RecWord *wrd, int use)
{
    int i;
    char tag_path_full[1024];
    size_t flen = 0;
    data1_node *nn;
    int termlist_only = 1;
    data1_termlist *tl;
    int xpdone = 0;

    yaz_log(LOG_DEBUG, "index_xpath level=%d use=%d", level, use);
    if ((!n->root->u.root.absyn) ||
	(n->root->u.root.absyn->enable_xpath_indexing)) {
      termlist_only = 0;
    }

    switch (n->which)
    {
    case DATA1N_data:
        wrd->string = n->u.data.data;
        wrd->length = n->u.data.len;
        xpdone = 0;
        flen = 0;
            
	/* we have to fetch the whole path to the data tag */
	for (nn = n; nn; nn = nn->parent) {
	    if (nn->which == DATA1N_tag) {
		size_t tlen = strlen(nn->u.tag.tag);
		if (tlen + flen > (sizeof(tag_path_full)-2)) return;
		memcpy (tag_path_full + flen, nn->u.tag.tag, tlen);
		flen += tlen;
		tag_path_full[flen++] = '/';
	    }
	    else if (nn->which == DATA1N_root)  break;
	}
	
	tag_path_full[flen] = 0;
	
	/* If we have a matching termlist... */
	if (n->root->u.root.absyn && 
	    (tl = xpath_termlist_by_tagpath(tag_path_full, n)))
	{
	    for (; tl; tl = tl->next)
	    {
		/* need to copy recword because it may be changed */
		RecWord wrd_tl;
		wrd->reg_type = *tl->structure;
		/* this is the ! case, so structure is for the xpath index */
		memcpy (&wrd_tl, wrd, sizeof(*wrd));
		if (tl->source)
		    sp_parse(n, &wrd_tl, tl->source);
		if (!tl->att) {
		    wrd_tl.attrSet = VAL_IDXPATH;
		    wrd_tl.attrUse = use;
		    if (p->flagShowRecords)
		    {
			int i;
		        printf("%*sXPath index", (level + 1) * 4, "");
			printf (" XData:\"");
			for (i = 0; i<wrd_tl.length && i < 40; i++)
			    fputc (wrd_tl.string[i], stdout);
			fputc ('"', stdout);
			if (wrd_tl.length > 40)
			    printf (" ...");
			fputc ('\n', stdout);
		    }
		    else
			(*p->tokenAdd)(&wrd_tl);
			
		    xpdone = 1;
		} else {
		    /* this is just the old fashioned attribute based index */
		    wrd_tl.attrSet = (int) (tl->att->parent->reference);
		    wrd_tl.attrUse = tl->att->locals->local;
		    if (p->flagShowRecords)
		    {
			int i;
			printf("%*sIdx: [%s]", (level + 1) * 4, "",
			       tl->structure);
			printf("%s:%s [%d] %s",
			       tl->att->parent->name,
			       tl->att->name, tl->att->value,
			       tl->source);
			printf (" XData:\"");
			for (i = 0; i<wrd_tl.length && i < 40; i++)
			    fputc (wrd_tl.string[i], stdout);
			fputc ('"', stdout);
			if (wrd_tl.length > 40)
			    printf (" ...");
			fputc ('\n', stdout);
		    }
		    else
			(*p->tokenAdd)(&wrd_tl);
		}
	    }
	}
	/* xpath indexing is done, if there was no termlist given, 
	   or no ! in the termlist, and default indexing is enabled... */
	if (!p->flagShowRecords && !xpdone && !termlist_only)
	{
	    wrd->attrSet = VAL_IDXPATH;
	    wrd->attrUse = use;
	    wrd->reg_type = 'w';
	    (*p->tokenAdd)(wrd);
	}
	else
	    wrd->seqno++;
        break;
    case DATA1N_tag:
        flen = 0;
        for (nn = n; nn; nn = nn->parent)
        {
            if (nn->which == DATA1N_tag)
            {
                size_t tlen = strlen(nn->u.tag.tag);
                if (tlen + flen > (sizeof(tag_path_full)-2))
                    return;
                memcpy (tag_path_full + flen, nn->u.tag.tag, tlen);
                flen += tlen;
                tag_path_full[flen++] = '/';
            }
            else if (nn->which == DATA1N_root)
                break;
        }


        wrd->reg_type = '0';
        wrd->string = tag_path_full;
        wrd->length = flen;
        wrd->attrSet = VAL_IDXPATH;
        wrd->attrUse = use;
        if (p->flagShowRecords)
        {
            printf("%*s tag=", (level + 1) * 4, "");
            for (i = 0; i<wrd->length && i < 40; i++)
                fputc (wrd->string[i], stdout);
            if (i == 40)
                printf (" ..");
            printf("\n");
        }
        else
        {
            data1_xattr *xp;
            data1_termlist *tl;
	    int do_xpindex;
            
	    tag_path_full[flen] = 0;
            
            /* Add tag start/end xpath index, only when there is a ! in the apropriate xelm
               directive, or default xpath indexing is enabled */
	    if (!(do_xpindex = 1 - termlist_only)) {
                if ((tl = xpath_termlist_by_tagpath(tag_path_full, n))) {
                    for (; tl; tl = tl->next)
		    {
			if (!tl->att)
			    do_xpindex = 1;
		    }
                }
	    }
	    if (do_xpindex) {
                (*p->tokenAdd)(wrd);   /* index element pag (AKA tag path) */
	    }
            
            if (use == 1) /* only for the starting tag... */
            {
#define MAX_ATTR_COUNT 50
                data1_termlist *tll[MAX_ATTR_COUNT];
                
                int i = 0;
                
                /* get termlists for attributes, and find out, if we have to do xpath indexing */
                for (xp = n->u.tag.attributes; xp; xp = xp->next) {
                    i++;
                }
                
                i = 0;
                for (xp = n->u.tag.attributes; xp; xp = xp->next) {
                    char comb[512];
                    int do_xpindex = 1 - termlist_only;
                    data1_termlist *tl;
                    char attr_tag_path_full[1024]; 
                    int int_len = flen;
                    
                    /* this could be cached as well */
                    sprintf (attr_tag_path_full, "@%s/%.*s",
                             xp->name, int_len, tag_path_full);
                    
                    tll[i] = xpath_termlist_by_tagpath(attr_tag_path_full,n);
                    
                    /* if there is a ! in the xelm termlist, or default indexing is on, 
                       proceed with xpath idx */
                    if ((tl = tll[i]))
                    {
                        for (; tl; tl = tl->next)
                        { 
                            if (!tl->att)
                                do_xpindex = 1;
                        }
                    }
                    
                    if (do_xpindex) {
                        
                        /* attribute  (no value) */
                        wrd->reg_type = '0';
                        wrd->attrUse = 3;
                        wrd->string = xp->name;
                        wrd->length = strlen(xp->name);
                        
                        wrd->seqno--;
                        (*p->tokenAdd)(wrd);
                        
                        if (xp->value &&
                            strlen(xp->name) + strlen(xp->value) < sizeof(comb)-2) {
                            
                            /* attribute value exact */
                            strcpy (comb, xp->name);
                            strcat (comb, "=");
                            strcat (comb, xp->value);
                            
                            wrd->attrUse = 3;
                            wrd->reg_type = '0';
                            wrd->string = comb;
                            wrd->length = strlen(comb);
                            wrd->seqno--;
                            
                            (*p->tokenAdd)(wrd);
                        }
                    }                
                    i++;
                }
                
                i = 0;
                for (xp = n->u.tag.attributes; xp; xp = xp->next) {
                    data1_termlist *tl;
                    char attr_tag_path_full[1024];
                    int int_len = flen;
                    int xpdone = 0;
                    
                    sprintf (attr_tag_path_full, "@%s/%.*s",
                             xp->name, int_len, tag_path_full);
                    
                    if ((tl = tll[i]))
                    {
                        /* If there is a termlist given (=xelm directive) */
                        for (; tl; tl = tl->next)
                        {
                            if (!tl->att) {
                                /* add xpath index for the attribute */
                                index_xpath_attr (attr_tag_path_full, xp->name,
                                                  xp->value, tl->structure,
                                                  p, wrd);
                                xpdone = 1;
                            } 
#if 0
			    else 
			    {
                                /* add attribute based index for the attribute */
                                if (xp->value) {
                                    wrd->attrSet = (int) 
                                        (tl->att->parent->reference);
                                    wrd->attrUse = tl->att->locals->local;
                                    wrd->reg_type = *tl->structure;
                                    wrd->string = xp->value;
                                    wrd->length = strlen(xp->value);
                                    (*p->tokenAdd)(wrd);
                                }
                            }
#endif
                        }
                    }
                    /* if there was no termlist for the given path, 
                       or the termlist didn't have a ! element, index 
                       the attribute as "w" */
                    if ((!xpdone) && (!termlist_only))
                    {
                        index_xpath_attr (attr_tag_path_full, xp->name,
                                          xp->value,  "w", p, wrd);
                    }
                    i++;
                }
            }
	}
    }
}

static void index_termlist (data1_node *par, data1_node *n,
                            struct recExtractCtrl *p, int level, RecWord *wrd)
{
    data1_termlist *tlist = 0;
    data1_datatype dtype = DATA1K_string;

    /*
     * cycle up towards the root until we find a tag with an att..
     * this has the effect of indexing locally defined tags with
     * the attribute of their ancestor in the record.
     */
    
    while (!par->u.tag.element)
        if (!par->parent || !(par=get_parent_tag(p->dh, par->parent)))
            break;
    if (!par || !(tlist = par->u.tag.element->termlists))
        return;
    if (par->u.tag.element->tag)
        dtype = par->u.tag.element->tag->kind;
    
    for (; tlist; tlist = tlist->next)
    {
	/* consider source */
	wrd->string = 0;
	assert(tlist->source);
	sp_parse(n, wrd, tlist->source);

	if (wrd->string)
	{
	    if (p->flagShowRecords)
	    {
		int i;
		printf("%*sIdx: [%s]", (level + 1) * 4, "",
		       tlist->structure);
		printf("%s:%s [%d] %s",
		       tlist->att->parent->name,
		       tlist->att->name, tlist->att->value,
		       tlist->source);
		printf (" XData:\"");
		for (i = 0; i<wrd->length && i < 40; i++)
		    fputc (wrd->string[i], stdout);
		fputc ('"', stdout);
		if (wrd->length > 40)
		    printf (" ...");
		fputc ('\n', stdout);
	    }
	    else
	    {
		wrd->reg_type = *tlist->structure;
		wrd->attrSet = (int) (tlist->att->parent->reference);
		wrd->attrUse = tlist->att->locals->local;
		(*p->tokenAdd)(wrd);
	    }
	}
    }
}

static int dumpkeys(data1_node *n, struct recExtractCtrl *p, int level,
                    RecWord *wrd)
{
    for (; n; n = n->next)
    {
	if (p->flagShowRecords) /* display element description to user */
	{
	    if (n->which == DATA1N_root)
	    {
		printf("%*s", level * 4, "");
                printf("Record type: '%s'\n", n->u.root.type);
	    }
	    else if (n->which == DATA1N_tag)
	    {
		data1_element *e;

		printf("%*s", level * 4, "");
		if (!(e = n->u.tag.element))
		    printf("Local tag: '%s'\n", n->u.tag.tag);
		else
		{
		    printf("Elm: '%s' ", e->name);
		    if (e->tag)
		    {
			data1_tag *t = e->tag;

			printf("TagNam: '%s' ", t->names->name);
			printf("(");
			if (t->tagset)
			    printf("%s[%d],", t->tagset->name, t->tagset->type);
			else
			    printf("?,");
			if (t->which == DATA1T_numeric)
			    printf("%d)", t->value.numeric);
			else
			    printf("'%s')", t->value.string);
		    }
		    printf("\n");
		}
	    }
	}

	if (n->which == DATA1N_tag)
	{
            index_termlist (n, n, p, level, wrd);
            /* index start tag */
	    if (n->root->u.root.absyn)
      	        index_xpath (n, p, level, wrd, 1);
 	}

	if (n->child)
	    if (dumpkeys(n->child, p, level + 1, wrd) < 0)
		return -1;


	if (n->which == DATA1N_data)
	{
	    data1_node *par = get_parent_tag(p->dh, n);

	    if (p->flagShowRecords)
	    {
		printf("%*s", level * 4, "");
		printf("Data: ");
		if (n->u.data.len > 256)
		    printf("'%.170s ... %.70s'\n", n->u.data.data,
			   n->u.data.data + n->u.data.len-70);
		else if (n->u.data.len > 0)
		    printf("'%.*s'\n", n->u.data.len, n->u.data.data);
		else
		    printf("NULL\n");
	    }

	    if (par)
		index_termlist (par, n, p, level, wrd);

	    index_xpath (n, p, level, wrd, 1016);
 	}

	if (n->which == DATA1N_tag)
	{
            /* index end tag */
	    index_xpath (n, p, level, wrd, 2);
	}

	if (p->flagShowRecords && n->which == DATA1N_root)
	{
	    printf("%*s-------------\n\n", level * 4, "");
	}
    }
    return 0;
}

int grs_extract_tree(struct recExtractCtrl *p, data1_node *n)
{
    oident oe;
    int oidtmp[OID_SIZE];
    RecWord wrd;

    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_SCHEMA;
    if (n->u.root.absyn)
    {
        oe.value = n->u.root.absyn->reference;
        
        if ((oid_ent_to_oid (&oe, oidtmp)))
            (*p->schemaAdd)(p, oidtmp);
    }
    (*p->init)(p, &wrd);

    return dumpkeys(n, p, 0, &wrd);
}

static int grs_extract_sub(struct grs_handlers *h, struct recExtractCtrl *p,
			   NMEM mem)
{
    data1_node *n;
    struct grs_read_info gri;
    oident oe;
    int oidtmp[OID_SIZE];
    RecWord wrd;

    gri.readf = p->readf;
    gri.seekf = p->seekf;
    gri.tellf = p->tellf;
    gri.endf = p->endf;
    gri.fh = p->fh;
    gri.offset = p->offset;
    gri.mem = mem;
    gri.dh = p->dh;

    if (read_grs_type (h, &gri, p->subType, &n))
	return RECCTRL_EXTRACT_ERROR_NO_SUCH_FILTER;
    if (!n)
        return RECCTRL_EXTRACT_EOF;
    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_SCHEMA;
#if 0
    if (!n->u.root.absyn)
        return RECCTRL_EXTRACT_ERROR;
#endif
    if (n->u.root.absyn)
    {
        oe.value = n->u.root.absyn->reference;
        if ((oid_ent_to_oid (&oe, oidtmp)))
            (*p->schemaAdd)(p, oidtmp);
    }
    data1_concat_text(p->dh, mem, n);

    /* ensure our data1 tree is UTF-8 */
    data1_iconv (p->dh, mem, n, "UTF-8", data1_get_encoding(p->dh, n));

#if 0
    data1_pr_tree (p->dh, n, stdout);
#endif

    (*p->init)(p, &wrd);
    if (dumpkeys(n, p, 0, &wrd) < 0)
    {
	data1_free_tree(p->dh, n);
	return RECCTRL_EXTRACT_ERROR_GENERIC;
    }
    data1_free_tree(p->dh, n);
    return RECCTRL_EXTRACT_OK;
}

static int grs_extract(void *clientData, struct recExtractCtrl *p)
{
    int ret;
    NMEM mem = nmem_create ();
    struct grs_handlers *h = (struct grs_handlers *) clientData;

    ret = grs_extract_sub(h, p, mem);
    nmem_destroy(mem);
    return ret;
}

/*
 * Return: -1: Nothing done. 0: Ok. >0: Bib-1 diagnostic.
 */
static int process_comp(data1_handle dh, data1_node *n, Z_RecordComposition *c)
{
    data1_esetname *eset;
    Z_Espec1 *espec = 0;
    Z_ElementSpec *p;

    switch (c->which)
    {
    case Z_RecordComp_simple:
	if (c->u.simple->which != Z_ElementSetNames_generic)
	    return 26; /* only generic form supported. Fix this later */
	if (!(eset = data1_getesetbyname(dh, n->u.root.absyn,
					 c->u.simple->u.generic)))
	{
	    yaz_log(LOG_LOG, "Unknown esetname '%s'", c->u.simple->u.generic);
	    return 25; /* invalid esetname */
	}
	yaz_log(LOG_DEBUG, "Esetname '%s' in simple compspec",
	     c->u.simple->u.generic);
	espec = eset->spec;
	break;
    case Z_RecordComp_complex:
	if (c->u.complex->generic)
	{
	    /* insert check for schema */
	    if ((p = c->u.complex->generic->elementSpec))
	    {
		switch (p->which)
		{
		case Z_ElementSpec_elementSetName:
		    if (!(eset =
			  data1_getesetbyname(dh, n->u.root.absyn,
					      p->u.elementSetName)))
		    {
			yaz_log(LOG_LOG, "Unknown esetname '%s'",
			     p->u.elementSetName);
			return 25; /* invalid esetname */
		    }
		    yaz_log(LOG_DEBUG, "Esetname '%s' in complex compspec",
			 p->u.elementSetName);
		    espec = eset->spec;
		    break;
		case Z_ElementSpec_externalSpec:
		    if (p->u.externalSpec->which == Z_External_espec1)
		    {
			yaz_log(LOG_DEBUG, "Got Espec-1");
			espec = p->u.externalSpec-> u.espec1;
		    }
		    else
		    {
			yaz_log(LOG_LOG, "Unknown external espec.");
			return 25; /* bad. what is proper diagnostic? */
		    }
		    break;
		}
	    }
	}
	else
	    return 26; /* fix */
    }
    if (espec)
    {
        yaz_log(LOG_DEBUG, "Element: Espec-1 match");
	return data1_doespec1(dh, n, espec);
    }
    else
    {
	yaz_log(LOG_DEBUG, "Element: all match");
	return -1;
    }
}

/* Add Zebra info in separate namespace ...
        <root 
         ...
         <metadata xmlns="http://www.indexdata.dk/zebra/">
          <size>359</size>
          <localnumber>447</localnumber>
          <filename>records/genera.xml</filename>
         </metadata>
        </root>
*/

static void zebra_xml_metadata (struct recRetrieveCtrl *p, data1_node *top,
                                NMEM mem)
{
    const char *idzebra_ns[3];
    const char *i2 = "\n  ";
    const char *i4 = "\n    ";
    data1_node *n;

    idzebra_ns[0] = "xmlns";
    idzebra_ns[1] = "http://www.indexdata.dk/zebra/";
    idzebra_ns[2] = 0;

    data1_mk_text (p->dh, mem, i2, top);

    n = data1_mk_tag (p->dh, mem, "idzebra", idzebra_ns, top);

    data1_mk_text (p->dh, mem, "\n", top);

    data1_mk_text (p->dh, mem, i4, n);
    
    data1_mk_tag_data_int (p->dh, n, "size", p->recordSize, mem);

    if (p->score != -1)
    {
        data1_mk_text (p->dh, mem, i4, n);
        data1_mk_tag_data_int (p->dh, n, "score", p->score, mem);
    }
    data1_mk_text (p->dh, mem, i4, n);
    data1_mk_tag_data_int (p->dh, n, "localnumber", p->localno, mem);
    if (p->fname)
    {
        data1_mk_text (p->dh, mem, i4, n);
        data1_mk_tag_data_text(p->dh, n, "filename", p->fname, mem);
    }
    data1_mk_text (p->dh, mem, i2, n);
}

static int grs_retrieve(void *clientData, struct recRetrieveCtrl *p)
{
    data1_node *node = 0, *onode = 0, *top;
    data1_node *dnew;
    data1_maptab *map;
    int res, selected = 0;
    NMEM mem;
    struct grs_read_info gri;
    const char *tagname;
    struct grs_handlers *h = (struct grs_handlers *) clientData;
    int requested_schema = VAL_NONE;
    data1_marctab *marctab;
    int dummy;
    
    mem = nmem_create();
    gri.readf = p->readf;
    gri.seekf = p->seekf;
    gri.tellf = p->tellf;
    gri.endf = NULL;
    gri.fh = p->fh;
    gri.offset = 0;
    gri.mem = mem;
    gri.dh = p->dh;

    yaz_log(LOG_DEBUG, "grs_retrieve");
    if (read_grs_type (h, &gri, p->subType, &node))
    {
	p->diagnostic = 14;
        nmem_destroy (mem);
	return 0;
    }
    if (!node)
    {
	p->diagnostic = 14;
        nmem_destroy (mem);
	return 0;
    }
    data1_concat_text(p->dh, mem, node);

    /* ensure our data1 tree is UTF-8 */
    data1_iconv (p->dh, mem, node, "UTF-8", data1_get_encoding(p->dh, node));

#if 0
    data1_pr_tree (p->dh, node, stdout);
#endif
    top = data1_get_root_tag (p->dh, node);

    yaz_log(LOG_DEBUG, "grs_retrieve: size");
    tagname = data1_systag_lookup(node->u.root.absyn, "size", "size");
    if (tagname &&
        (dnew = data1_mk_tag_data_wd(p->dh, top, tagname, mem)))
    {
	dnew->u.data.what = DATA1I_text;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->recordSize);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }
    
    tagname = data1_systag_lookup(node->u.root.absyn, "rank", "rank");

    if (tagname && p->score >= 0 &&
	(dnew = data1_mk_tag_data_wd(p->dh, top, tagname, mem)))
    {
        yaz_log(LOG_DEBUG, "grs_retrieve: %s", tagname);
	dnew->u.data.what = DATA1I_num;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->score);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    tagname = data1_systag_lookup(node->u.root.absyn, "sysno",
                                  "localControlNumber");
    if (tagname && p->localno > 0 &&
        (dnew = data1_mk_tag_data_wd(p->dh, top, tagname, mem)))
    {
        yaz_log(LOG_DEBUG, "grs_retrieve: %s", tagname);
	dnew->u.data.what = DATA1I_text;
	dnew->u.data.data = dnew->lbuf;
        
	sprintf(dnew->u.data.data, "%d", p->localno);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    if (p->input_format == VAL_TEXT_XML)
        zebra_xml_metadata (p, top, mem);

#if 0
    data1_pr_tree (p->dh, node, stdout);
#endif
#if YAZ_VERSIONL >= 0x010903L
    if (p->comp && p->comp->which == Z_RecordComp_complex &&
	p->comp->u.complex->generic &&
        p->comp->u.complex->generic->which == Z_Schema_oid &&
        p->comp->u.complex->generic->schema.oid)
    {
	oident *oe = oid_getentbyoid (p->comp->u.complex->generic->schema.oid);
	if (oe)
	    requested_schema = oe->value;
    }
#else
    if (p->comp && p->comp->which == Z_RecordComp_complex &&
	p->comp->u.complex->generic && p->comp->u.complex->generic->schema)
    {
	oident *oe = oid_getentbyoid (p->comp->u.complex->generic->schema);
	if (oe)
	    requested_schema = oe->value;
    }
#endif

    /* If schema has been specified, map if possible, then check that
     * we got the right one 
     */
    if (requested_schema != VAL_NONE)
    {
	yaz_log(LOG_DEBUG, "grs_retrieve: schema mapping");
	for (map = node->u.root.absyn->maptabs; map; map = map->next)
	{
	    if (map->target_absyn_ref == requested_schema)
	    {
		onode = node;
		if (!(node = data1_map_record(p->dh, onode, map, mem)))
		{
		    p->diagnostic = 14;
		    nmem_destroy (mem);
		    return 0;
		}
		break;
	    }
	}
	if (node->u.root.absyn &&
	    requested_schema != node->u.root.absyn->reference)
	{
	    p->diagnostic = 238;
	    nmem_destroy (mem);
	    return 0;
	}
    }
    /*
     * Does the requested format match a known syntax-mapping? (this reflects
     * the overlap of schema and formatting which is inherent in the MARC
     * family)
     */
    yaz_log(LOG_DEBUG, "grs_retrieve: syntax mapping");
    if (node->u.root.absyn)
        for (map = node->u.root.absyn->maptabs; map; map = map->next)
        {
            if (map->target_absyn_ref == p->input_format)
            {
                onode = node;
                if (!(node = data1_map_record(p->dh, onode, map, mem)))
                {
                    p->diagnostic = 14;
                    nmem_destroy (mem);
                    return 0;
                }
                break;
            }
        }
    yaz_log(LOG_DEBUG, "grs_retrieve: schemaIdentifier");
    if (node->u.root.absyn &&
	node->u.root.absyn->reference != VAL_NONE &&
	p->input_format == VAL_GRS1)
    {
	oident oe;
	Odr_oid *oid;
	int oidtmp[OID_SIZE];
	
	oe.proto = PROTO_Z3950;
	oe.oclass = CLASS_SCHEMA;
	oe.value = node->u.root.absyn->reference;
	
	if ((oid = oid_ent_to_oid (&oe, oidtmp)))
	{
	    char tmp[128];
	    data1_handle dh = p->dh;
	    char *p = tmp;
	    int *ii;
	    
	    for (ii = oid; *ii >= 0; ii++)
	    {
		if (p != tmp)
			*(p++) = '.';
		sprintf(p, "%d", *ii);
		p += strlen(p);
	    }
	    if ((dnew = data1_mk_tag_data_wd(dh, top, 
                                             "schemaIdentifier", mem)))
	    {
		dnew->u.data.what = DATA1I_oid;
		dnew->u.data.data = (char *) nmem_malloc(mem, p - tmp);
		memcpy(dnew->u.data.data, tmp, p - tmp);
		dnew->u.data.len = p - tmp;
	    }
	}
    }

    yaz_log(LOG_DEBUG, "grs_retrieve: element spec");
    if (p->comp && (res = process_comp(p->dh, node, p->comp)) > 0)
    {
	p->diagnostic = res;
	if (onode)
	    data1_free_tree(p->dh, onode);
	data1_free_tree(p->dh, node);
	nmem_destroy(mem);
	return 0;
    }
    else if (p->comp && !res)
	selected = 1;

#if 0
    data1_pr_tree (p->dh, node, stdout);
#endif
    yaz_log(LOG_DEBUG, "grs_retrieve: transfer syntax mapping");
    switch (p->output_format = (p->input_format != VAL_NONE ?
				p->input_format : VAL_SUTRS))
    {
    case VAL_TEXT_XML:

#if 0
        data1_pr_tree (p->dh, node, stdout);
#endif

        if (p->encoding)
            data1_iconv (p->dh, mem, node, p->encoding, "UTF-8");

	if (!(p->rec_buf = data1_nodetoidsgml(p->dh, node, selected,
					      &p->rec_len)))
	    p->diagnostic = 238;
	else
	{
	    char *new_buf = (char*) odr_malloc (p->odr, p->rec_len);
	    memcpy (new_buf, p->rec_buf, p->rec_len);
	    p->rec_buf = new_buf;
	}
	break;
    case VAL_GRS1:
	dummy = 0;
	if (!(p->rec_buf = data1_nodetogr(p->dh, node, selected,
					  p->odr, &dummy)))
	    p->diagnostic = 238; /* not available in requested syntax */
	else
	    p->rec_len = (size_t) (-1);
	break;
    case VAL_EXPLAIN:
	if (!(p->rec_buf = data1_nodetoexplain(p->dh, node, selected,
					       p->odr)))
	    p->diagnostic = 238;
	else
	    p->rec_len = (size_t) (-1);
	break;
    case VAL_SUMMARY:
	if (!(p->rec_buf = data1_nodetosummary(p->dh, node, selected,
					       p->odr)))
	    p->diagnostic = 238;
	else
	    p->rec_len = (size_t) (-1);
	break;
    case VAL_SUTRS:
        if (p->encoding)
            data1_iconv (p->dh, mem, node, p->encoding, "UTF-8");
	if (!(p->rec_buf = data1_nodetobuf(p->dh, node, selected,
					   &p->rec_len)))
	    p->diagnostic = 238;
	else
	{
	    char *new_buf = (char*) odr_malloc (p->odr, p->rec_len);
	    memcpy (new_buf, p->rec_buf, p->rec_len);
	    p->rec_buf = new_buf;
	}
	break;
    case VAL_SOIF:
	if (!(p->rec_buf = data1_nodetosoif(p->dh, node, selected,
					    &p->rec_len)))
	    p->diagnostic = 238;
	else
	{
	    char *new_buf = (char*) odr_malloc (p->odr, p->rec_len);
	    memcpy (new_buf, p->rec_buf, p->rec_len);
	    p->rec_buf = new_buf;
	}
	break;
    default:
	if (!node->u.root.absyn)
	{
	    p->diagnostic = 238;
	    break;
	}
	for (marctab = node->u.root.absyn->marc; marctab;
	     marctab = marctab->next)
	    if (marctab->reference == p->input_format)
		break;
	if (!marctab)
	{
	    p->diagnostic = 238;
	    break;
	}
        if (p->encoding)
            data1_iconv (p->dh, mem, node, p->encoding, "UTF-8");
	if (!(p->rec_buf = data1_nodetomarc(p->dh, marctab, node,
					selected, &p->rec_len)))
	    p->diagnostic = 238;
	else
	{
	    char *new_buf = (char*) odr_malloc (p->odr, p->rec_len);
	    memcpy (new_buf, p->rec_buf, p->rec_len);
		p->rec_buf = new_buf;
	}
    }
    if (node)
	data1_free_tree(p->dh, node);
    if (onode)
	data1_free_tree(p->dh, onode);
    nmem_destroy(mem);
    return 0;
}

static struct recType grs_type =
{
    "grs",
    grs_init,
    grs_destroy,
    grs_extract,
    grs_retrieve
};

RecType recTypeGrs = &grs_type;
