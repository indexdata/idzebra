/*
    $Id: inline.c,v 1.4 2004-12-10 11:56:22 heikki Exp $
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
/* Old yaz-util includes (FIXME - clean up what is not needed)*/
#include <yaz/yconfig.h>
#include <yaz/yaz-version.h>
#include <yaz/xmalloc.h>
#include <yaz/ylog.h>  
#include <yaz/tpath.h>
#include <yaz/options.h>
#include <yaz/wrbuf.h>
#include <yaz/nmem.h>
#include <yaz/readconf.h>
#include <yaz/marcdisp.h>
#include <yaz/yaz-iconv.h>

#include "inline.h"

static void inline_destroy_subfield_recursive(inline_subfield *p);

inline_field *inline_mk_field(void)
{
    inline_field *p = (inline_field *) xmalloc(sizeof(*p));

    if (p)
    {
	memset(p, 0, sizeof(*p));
	p->name = (char *) xmalloc(SZ_FNAME+1);
	*(p->name) = '\0';
	p->ind1 = (char *) xmalloc(SZ_IND+1);
	*(p->ind1) = '\0';
	p->ind2 = (char *) xmalloc(SZ_IND+1);
	*(p->ind2) = '\0';
    }
    return p;
}
void inline_destroy_field(inline_field *p)
{
    if (p)
    {
	if (p->name) xfree(p->name);
	if (p->ind1) xfree(p->ind1);
	if (p->ind2) xfree(p->ind2);
	if (p->list)
	    inline_destroy_subfield_recursive(p->list);
	xfree(p);
    }
}
static inline_subfield *inline_mk_subfield(inline_subfield *parent)
{
    inline_subfield *p = (inline_subfield *)xmalloc(sizeof(*p));
    
    if (p)
    {
	memset(p, 0, sizeof(*p));
	p->name = (char *) xmalloc(SZ_SFNAME+1);
	*(p->name) = '\0';
	p->parent = parent;
    }
    return p;
}

#if 0
static void inline_destroy_subfield(inline_subfield *p)
{
    if (p)
    {
	if (p->name) xfree(p->name);
	if (p->data) xfree(p->data);
	if (p->parent) p->parent->next = p->next;
	xfree(p);
    }
}
#endif

static void inline_destroy_subfield_recursive(inline_subfield *p)
{
    if (p)
    {
	inline_destroy_subfield_recursive(p->next);
	if (p->name) xfree(p->name);
	if (p->data) xfree(p->data);
	if (p->parent)
	    p->parent->next = 0;
	xfree(p);
    }
}
int inline_parse(inline_field *pif, const char *tag, const char *s)
{
    inline_field *pf = pif;
    char *p = (char *)s;
    
    if (!pf)
	return -1;
	
    if (pf->name[0] == '\0')
    {
	if ((sscanf(p, "%3s", pf->name)) != 1)
	    return -2;

	p += SZ_FNAME;

	if (!memcmp(pf->name, "00", 2))
	{
	    pf->list = inline_mk_subfield(0);
	    pf->list->data = xstrdup(p);
	}
	else
	{
	    if ((sscanf(p, "%c%c", pf->ind1, pf->ind2)) != 2)
		return -3;
	}
    }
    else
    {
	inline_subfield *psf = inline_mk_subfield(0);
	
	sscanf(tag, "%1s", psf->name);
	psf->data = xstrdup(p);
	
	if (!pf->list)
	{
	    pf->list = psf;
	}
	else
	{
	    inline_subfield *last = pf->list;
	    while (last->next)
		last = last->next;
	    last->next = psf;
	}
    }
    return 0;
}
