/*
    $Id: marcomp.c,v 1.5 2005-01-03 19:27:53 adam Exp $

    marcomp.c - compiler of MARC statements.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <yaz/yaz-util.h>

#include "marcomp.h"

static mc_token mc_gettoken(mc_context *c);
static void mc_ungettoken(mc_context *c);
static int mc_getval(mc_context *c);
static int mc_getdata(mc_context *c, char *s, int sz);
static void mc_getinterval(mc_context *c, int *start, int *end);

static mc_subfield *mc_mk_subfield(mc_subfield *parent);
static mc_field *mc_mk_field(void);

static struct mc_errmsg
{
    mc_errcode code;
    const char *msg;
} mc_errmsg[] = {
{EMCOK, "OK"},
{EMCNOMEM, "NO mem"},
{EMCF, "not complete field"},
{EMCSF, "not complete subfield"},
{EMCSFGROUP, "not closed GROUP"},
{EMCSFVAR, "not closed VARIANT"},
{EMCSFINLINE, "not closed IN-LINE"},
{EMCEND, "not correct errno"}
};
mc_errcode mc_errno(mc_context *c)
{
    return c->errcode;
}
const char *mc_error(mc_errcode no)
{
    if (no >= EMCOK && no<EMCEND)
	return mc_errmsg[no].msg;
    else
	return mc_errmsg[EMCEND].msg;
}
mc_context *mc_mk_context(const char *s)
{
    mc_context *p=0;
    
    if (s && strlen(s))
    {
	p = (mc_context*) xmalloc(sizeof(*p));
	
	if (!p)
	    return 0;
	
	memset(p, 0, sizeof(*p));
	p->errcode = EMCOK;
	p->data = s;
	p->len = strlen(s);
	p->crrtok = NOP;
    }
    
    return p;
}
void mc_destroy_context(mc_context *c)
{
    if (c) xfree(c);
}
mc_token mc_gettoken(mc_context *c)
{
    if (c->offset >= c->len)
	return NOP;

    switch (*(c->data+c->offset))
    {
	case '{': c->crrtok = LVARIANT; break;
	case '}': c->crrtok = RVARIANT; break;
	case '(': c->crrtok = LGROUP; break;
	case ')': c->crrtok = RGROUP; break;
	case '<': c->crrtok = LINLINE; break;
	case '>': c->crrtok = RINLINE; break;
	case '$': c->crrtok = SUBFIELD; break;
	case '[': c->crrtok = LINTERVAL; break;
	case ']': c->crrtok = RINTERVAL; break;
	default:
	    if (isspace(*(unsigned char *) (c->data+c->offset)) 
			    || *(c->data+c->offset) == '\n')
	    {
		c->crrtok = NOP;
	    }
	    else
	    {
		c->crrtok = REGULAR;
		c->crrval = *(c->data+c->offset);
	    }
    }
#ifdef DEBUG
    fprintf(stderr, "gettoken(): offset: %d", c->offset);
    if (c->crrtok == REGULAR)
	fprintf(stderr, "<%c>", c->crrval);
    fprintf(stderr, "\n");
#endif
    c->offset++;
    return c->crrtok;
}
void mc_ungettoken(mc_context *c)
{
    if (c->offset > 0)
	c->offset--;
}
int mc_getval(mc_context *c)
{
    return c->crrval;
}
int mc_getdata(mc_context *c, char *s, int sz)
{
    int i;
    
    for (i=0; i<sz; i++)
    {
	if (mc_gettoken(c)!=REGULAR)
	{
	    mc_ungettoken(c);
	    break;
	}
	s[i] = mc_getval(c);
    }
    s[i] = '\0';
    
    return i;
}
void mc_getinterval(mc_context *c, int *start, int *end)
{
    char buf[6+1];
    int start_pos, end_pos;
    
    start_pos = end_pos = -1;
       
    if (mc_gettoken(c) == LINTERVAL)
    {
        int i;
	
	for (i=0;i<6;i++)
        {
            mc_token tok = mc_gettoken(c);
	    
	    if (tok == RINTERVAL || tok == NOP)
		break;
		
	    buf[i] = mc_getval(c);
	}
	
	buf[i] = '\0';
	i = sscanf(buf, "%d-%d", &start_pos, &end_pos);
	
	if (i == 1)
	    end_pos = start_pos;
	else if ( i == 0)
	{
	    start_pos = 0;
	}
    }
    *start = start_pos;
    *end = end_pos;
}
mc_field *mc_mk_field(void)
{
    mc_field *p = (mc_field *)xmalloc(sizeof(*p));

    if (p)
    {
	memset(p, 0, sizeof(*p));
        p->name = (char *)xmalloc(SZ_FNAME+1);
	*p->name = '\0';
        p->ind1 = (char *)xmalloc(SZ_IND+1);
	*p->ind1 = '\0';
        p->ind2 = (char *)xmalloc(SZ_IND+1);
	*p->ind2 = '\0';
	p->interval.start = p->interval.end = -1;
    }
    return p;
}    
void mc_destroy_field(mc_field *p)
{
    if (!p)
        return;
    if (p->name) xfree(p->name);     
    if (p->ind1) xfree(p->ind1);     
    if (p->ind2) xfree(p->ind2);
    if (p->list) mc_destroy_subfields_recursive(p->list);
    xfree(p);
}
mc_field *mc_getfield(mc_context *c)
{
    mc_field *pf;

    pf = mc_mk_field();
    
    if (!pf)
    {
	c->errcode = EMCNOMEM;
    	return 0;
    }

    if (mc_getdata(c, pf->name, SZ_FNAME) == SZ_FNAME)
    {
	mc_token nexttok = mc_gettoken(c);
	
	mc_ungettoken(c);
	
	if (nexttok == LINTERVAL)
	{
	    mc_getinterval(c, &pf->interval.start, &pf->interval.end);
#ifdef DEBUG
	    fprintf(stderr, "ineterval (%d)-(%d)\n", pf->interval.start,
		pf->interval.end);
#endif
	}
	
	if ((mc_getdata(c, pf->ind1, SZ_IND) == SZ_IND) &&
	    (mc_getdata(c, pf->ind2, SZ_IND) == SZ_IND))
	{
	    pf->list = mc_getsubfields(c, 0);
	}
    }
    else
    {
	c->errcode = EMCF;
	mc_destroy_field(pf);
	return 0;
    }
    	
    return pf;
}
mc_subfield *mc_mk_subfield(mc_subfield *parent)
{
    mc_subfield *p = (mc_subfield*)xmalloc(sizeof(*p));

    if (p)
    {
	memset(p, 0, sizeof(*p));
	p->which = MC_SF;
        p->name = (char *)xmalloc(SZ_SFNAME+1);
	*p->name = '\0';
        p->prefix = (char *)xmalloc(SZ_PREFIX+1);
	*p->prefix = '\0';
        p->suffix = (char *)xmalloc(SZ_SUFFIX+1);
	*p->suffix = '\0';
	p->parent = parent;
	p->interval.start = p->interval.end = -1;
    }
    return p;
}
void mc_destroy_subfield(mc_subfield *p)
{
    if (!p)
        return;
    
    if (p->which == MC_SFGROUP || p->which == MC_SFVARIANT)
    {
        if (p->u.child)
            mc_destroy_subfields_recursive(p->u.child);
    }
    else if (p->which == MC_SF)
    {
        if (p->u.in_line)
            mc_destroy_field(p->u.in_line);
    }
    if (p->name) xfree(p->name);     
    if (p->prefix) xfree(p->prefix);     
    if (p->suffix) xfree(p->suffix);
    if (p->parent) p->parent->next = p->next;
    xfree(p);
}
void mc_destroy_subfields_recursive(mc_subfield *p)
{
    if (!p)
        return;

    mc_destroy_subfields_recursive(p->next);
	
    if (p->which == MC_SFGROUP || p->which == MC_SFVARIANT)
    {
        if (p->u.child)
            mc_destroy_subfields_recursive(p->u.child);
    }
    else if (p->which == MC_SF)
    {
        if (p->u.in_line)
            mc_destroy_field(p->u.in_line);
    }
	
    if (p->name) xfree(p->name);
    if (p->prefix) xfree(p->prefix);
    if (p->suffix) xfree(p->suffix);
    if (p->parent) p->parent->next = 0;
    xfree(p);
}
mc_subfield *mc_getsubfields(mc_context *c, mc_subfield *parent)
{
    mc_subfield *psf=0;
    mc_token tok = mc_gettoken(c);
    
    if (tok == NOP)
        return 0;
	
    if (tok == LGROUP)
    {
        if (!(psf = mc_mk_subfield(parent)))
	{
	    c->errcode = EMCNOMEM;
	    return 0;
	}

	psf->which = MC_SFGROUP;
	psf->u.child = mc_getsubfields(c, psf);
	
	if (mc_gettoken(c) == RGROUP)
	    psf->next = mc_getsubfields(c, psf);
	else
	{
	    c->errcode = EMCSFGROUP;
	    mc_destroy_subfield(psf);
	    return 0;
	}
    }
    else if (tok == LVARIANT)
    {
        if (!(psf = mc_mk_subfield(parent)))
	{
	    c->errcode = EMCNOMEM;
	    return 0;
	}

	psf->which = MC_SFVARIANT;
	psf->u.child = mc_getsubfields(c, psf);
	
	if (mc_gettoken(c) == RVARIANT)
	    psf->next = mc_getsubfields(c, psf);
	else
	{
	    c->errcode = EMCSFVAR;
	    mc_destroy_subfield(psf);
	    return 0;
	}
    }
    else if (tok == RGROUP || tok == RVARIANT || tok == RINLINE)
    {
	mc_ungettoken(c);
	return 0;
    }
    else if (tok == REGULAR)
    {
        if (!(psf = mc_mk_subfield(parent)))
	{
	    c->errcode = EMCNOMEM;
	    return 0;
	}

	mc_ungettoken(c);

	if((mc_getdata(c, psf->prefix, SZ_PREFIX) == SZ_PREFIX) &&
	    (mc_gettoken(c) == SUBFIELD) &&
		(mc_getdata(c, psf->name, SZ_SFNAME) == SZ_SFNAME))
	{
            mc_token tok = mc_gettoken(c);

	    mc_ungettoken(c);
	    
	    if (tok == LINTERVAL)
            {
        	mc_getinterval(c, &psf->interval.start, &psf->interval.end);
            }
	    else if (tok == LINLINE)
	    {
		mc_gettoken(c);
		psf->u.in_line = mc_getfield(c);
		if (mc_gettoken(c) != RINLINE)
		{
		    c->errcode = EMCSFINLINE;
		    mc_destroy_subfield(psf);
		    return 0;
		}
	    }
	
	    if (mc_getdata(c, psf->suffix, SZ_SUFFIX) == SZ_SUFFIX)
	    {
        	psf->which = MC_SF;
            	psf->next = mc_getsubfields(c, psf);
	    }
            else
	    {
		c->errcode = EMCSF;
		mc_destroy_subfield(psf);
		return 0;
            }
        }
    }     
    return psf;
}
