#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <yaz/yaz-util.h>
#include "inline.h"

static void inline_destroy_subfield_recursive(inline_subfield *p);

static inline_field *inline_mk_field(void)
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
inline_field *inline_parse(const char *s)
{
    inline_field *pf = inline_mk_field();
    char *p = (char *)s;
    
    if (!pf)
	return 0;

    if ((sscanf(p, "%3s", pf->name)) != 1)
	return 0;

    p += SZ_FNAME;

    if (!memcmp(pf->name, "00", 2))
    {
	pf->list = inline_mk_subfield(0);
	pf->list->data = xstrdup(p);
    }
    else
    {
	if ((sscanf(p, "%c%c", pf->ind1, pf->ind2)) == 2)
	{
	    char *pdup;
	    inline_subfield *parent = 0;
	    
	    p += 2*SZ_IND;
    
	    if (!strlen(p) || *p != '$')
	    {
		return pf;
	    }
	    
	    pdup = p = xstrdup(p);
	    
	    for (p=strtok(p, "$"); p; p = strtok(NULL, "$"))
	    {
		inline_subfield *psf = inline_mk_subfield(parent);
		
		if (!psf)
		    break;
		    
		if (!parent)
		    pf->list = psf;
		else
		    parent->next = psf;
		parent = psf;	
		sscanf(p, "%1s", psf->name);
		p += SZ_SFNAME;
		psf->data = (char *) xstrdup(p);
	    }
	    
	    xfree(pdup);
	}
    }
    return pf;
}
