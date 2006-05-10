/* $Id: d1_absyn.c,v 1.24 2006-05-10 08:13:17 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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
#include <stdlib.h>
#include <string.h>

#include <yaz/log.h>
#include <yaz/oid.h>
#include <idzebra/data1.h>
#include <zebra_xpath.h>
#include <d1_absyn.h>

#define D1_MAX_NESTING  128

struct data1_hash_table {
    NMEM nmem;
    int size;
    struct data1_hash_entry **ar;
};

struct data1_hash_entry {
    void *clientData;
    char *str;
    struct data1_hash_entry *next;
};

unsigned data1_hash_calc(struct data1_hash_table *ht, const char *str)
{
    unsigned v = 0;
    assert(str);
    while (*str)
    {
	if (*str >= 'a' && *str <= 'z')
	    v = v*65509 + *str -'a'+10;
	else if (*str >= 'A' && *str <= 'Z')
	    v = v*65509 + *str -'A'+10;
	else if (*str >= '0' && *str <= '9')
	    v = v*65509 + *str -'0';
	str++;
    }
    return v % ht->size;
}

struct data1_hash_table *data1_hash_open(int size, NMEM nmem)
{
    int i;
    struct data1_hash_table *ht = nmem_malloc(nmem, sizeof(*ht));
    ht->nmem = nmem;
    ht->size = size;
    if (ht->size <= 0)
	ht->size = 29;
    ht->ar = nmem_malloc(nmem, sizeof(*ht->ar) * ht->size);
    for (i = 0; i<ht->size; i++)
	ht->ar[i] = 0;
    return ht;
}

void data1_hash_insert(struct data1_hash_table *ht, const char *str,
		       void *clientData, int copy)
{
    char *dstr = copy ? nmem_strdup(ht->nmem, str) : (char*) str;
    if (strchr(str, '?') || strchr(str, '.'))
    {
	int i;
	for (i = 0; i<ht->size; i++)
	{
	    struct data1_hash_entry **he = &ht->ar[i];
	    for (; *he && strcmp(str, (*he)->str); he = &(*he)->next)
		;
	    if (!*he)
	    {
		*he = nmem_malloc(ht->nmem, sizeof(**he));
		(*he)->str = dstr;
		(*he)->next = 0;
	    }
	    (*he)->clientData = clientData;
	}
    }
    else
    {
	struct data1_hash_entry **he = &ht->ar[data1_hash_calc(ht, str)];
	for (; *he && strcmp(str, (*he)->str); he = &(*he)->next)
	    ;
	if (!*he)
	{
	    *he = nmem_malloc(ht->nmem, sizeof(**he));
	    (*he)->str = dstr;
	    (*he)->next = 0;
	}
	(*he)->clientData = clientData;
    }
}

void *data1_hash_lookup(struct data1_hash_table *ht, const char *str)
{
    struct data1_hash_entry **he = &ht->ar[data1_hash_calc(ht, str)];
    
    for (; *he && yaz_matchstr(str, (*he)->str); he = &(*he)->next)
	;
    if (*he)
	return (*he)->clientData;
    return 0;
}

struct data1_systag {
    char *name;
    char *value;
    struct data1_systag *next;
};

struct data1_absyn_cache_info 
{
    char *name;
    data1_absyn *absyn;
    data1_absyn_cache next;
};

struct data1_attset_cache_info 
{
    char *name;
    data1_attset *attset;
    data1_attset_cache next;
};

data1_element *data1_mk_element(data1_handle dh)
{
    data1_element *e = nmem_malloc(data1_nmem_get(dh), sizeof(*e));
    e->name = 0;
    e->tag = 0;
    e->termlists = 0;
    e->next = e->children = 0;
    e->sub_name = 0;
    e->hash = 0;
    return e;
}

data1_absyn *data1_absyn_search (data1_handle dh, const char *name)
{
    data1_absyn_cache p = *data1_absyn_cache_get (dh);

    while (p)
    {
	if (!yaz_matchstr (name, p->name))
	    return p->absyn;
	p = p->next;
    }
    return NULL;
}
/* *ostrich*
   We need to destroy DFAs, in xp_element (xelm) definitions 
   pop, 2002-12-13
*/

void data1_absyn_destroy (data1_handle dh)
{
    data1_absyn_cache p = *data1_absyn_cache_get (dh);
    
    while (p)
    {
        data1_absyn *abs = p->absyn;
	if (abs)
	{
	    data1_xpelement *xpe = abs->xp_elements;
	    while (xpe) {
		yaz_log (YLOG_DEBUG,"Destroy xp element %s",xpe->xpath_expr);
		if (xpe->dfa) {  dfa_delete (&xpe->dfa); }
		xpe = xpe->next;
	    } 
	}
        p = p->next;
    }
}


void data1_absyn_trav (data1_handle dh, void *handle,
		       void (*fh)(data1_handle dh, void *h, data1_absyn *a))
{
    data1_absyn_cache p = *data1_absyn_cache_get (dh);

    while (p)
    {
	(*fh)(dh, handle, p->absyn);
	p = p->next;
    }
}

data1_absyn *data1_absyn_add (data1_handle dh, const char *name)
{
    char fname[512];
    NMEM mem = data1_nmem_get (dh);

    data1_absyn_cache p = (data1_absyn_cache)nmem_malloc (mem, sizeof(*p));
    data1_absyn_cache *pp = data1_absyn_cache_get (dh);

    sprintf(fname, "%s.abs", name);
    p->absyn = data1_read_absyn (dh, fname, 0);
    p->name = nmem_strdup (mem, name);
    p->next = *pp;
    *pp = p;
    return p->absyn;
}

data1_absyn *data1_get_absyn (data1_handle dh, const char *name)
{
    data1_absyn *absyn;

    if (!(absyn = data1_absyn_search (dh, name)))
	absyn = data1_absyn_add (dh, name);
    return absyn;
}

data1_attset *data1_attset_search_name (data1_handle dh, const char *name)
{
    data1_attset_cache p = *data1_attset_cache_get (dh);

    while (p)
    {
	if (!yaz_matchstr (name, p->name))
	    return p->attset;
	p = p->next;
    }
    return NULL;
}

data1_attset *data1_attset_search_id (data1_handle dh, int id)
{
    data1_attset_cache p = *data1_attset_cache_get (dh);

    while (p)
    {
	if (id == p->attset->reference)
	    return p->attset;
	p = p->next;
    }
    return NULL;
}

data1_attset *data1_attset_add (data1_handle dh, const char *name)
{
    char fname[512], aname[512];
    NMEM mem = data1_nmem_get (dh);
    data1_attset *attset;

    strcpy (aname, name);
    sprintf(fname, "%s.att", name);
    attset = data1_read_attset (dh, fname);
    if (!attset)
    {
	char *cp;
	attset = data1_read_attset (dh, name);
	if (attset && (cp = strrchr (aname, '.')))
	    *cp = '\0';
    }
    if (!attset)
	yaz_log (YLOG_WARN|YLOG_ERRNO, "Couldn't load attribute set %s", name);
    else
    {
	data1_attset_cache p = (data1_attset_cache)
	    nmem_malloc (mem, sizeof(*p));
	data1_attset_cache *pp = data1_attset_cache_get (dh);
	
	attset->name = p->name = nmem_strdup (mem, aname);
	p->attset = attset;
	p->next = *pp;
	*pp = p;
    }
    return attset;
}

data1_attset *data1_get_attset (data1_handle dh, const char *name)
{
    data1_attset *attset;

    if (!(attset = data1_attset_search_name (dh, name)))
	attset = data1_attset_add (dh, name);
    return attset;
}

data1_esetname *data1_getesetbyname(data1_handle dh, data1_absyn *a,
				    const char *name)
{
    data1_esetname *r;

    for (r = a->esetnames; r; r = r->next)
	if (!data1_matchstr(r->name, name))
	    return r;
    return 0;
}

/* we have multiple versions of data1_getelementbyname */
#define DATA1_GETELEMENTBYTAGNAME_VERSION 1

#if DATA1_GETELEMENTBYTAGNAME_VERSION==0
/* straight linear search */
data1_element *data1_getelementbytagname (data1_handle dh, data1_absyn *abs,
					  data1_element *parent,
					  const char *tagname)
{
    data1_element *r;

    /* It's now possible to have a data1 tree with no abstract syntax */
    if ( !abs )
        return 0;

    if (!parent)
        r = abs->main_elements;
    else
	r = parent->children;

    for (; r; r = r->next)
    {
	data1_name *n;

	for (n = r->tag->names; n; n = n->next)
	    if (!data1_matchstr(tagname, n->name))
		return r;
    }
    return 0;
}
#endif

#if DATA1_GETELEMENTBYTAGNAME_VERSION==1
/* using hash search */
data1_element *data1_getelementbytagname (data1_handle dh, data1_absyn *abs,
					  data1_element *parent,
					  const char *tagname)
{
    data1_element *r;
    struct data1_hash_table *ht;

    /* It's now possible to have a data1 tree with no abstract syntax */
    if ( !abs )
        return 0;

    if (!parent)
        r = abs->main_elements;
    else
	r = parent->children;

    if (!r)
	return 0;

    ht = r->hash;
    if (!ht)
    {
	ht = r->hash = data1_hash_open(29, data1_nmem_get(dh));
	for (; r; r = r->next)
	{
	    data1_name *n;
	    
	    for (n = r->tag->names; n; n = n->next)
		data1_hash_insert(ht, n->name, r, 0);
	}
    }
    return data1_hash_lookup(ht, tagname);
}
#endif

data1_element *data1_getelementbyname (data1_handle dh, data1_absyn *absyn,
				       const char *name)
{
    data1_element *r;

    /* It's now possible to have a data1 tree with no abstract syntax */
    if ( !absyn )
        return 0;
    for (r = absyn->main_elements; r; r = r->next)
	if (!data1_matchstr(r->name, name))
	    return r;
    return 0;
}


void fix_element_ref (data1_handle dh, data1_absyn *absyn, data1_element *e)
{
    /* It's now possible to have a data1 tree with no abstract syntax */
    if ( !absyn )
        return;

    for (; e; e = e->next)
    {
	if (!e->sub_name)
	{
	    if (e->children)
		fix_element_ref (dh, absyn, e->children);
	}
	else
	{
	    data1_sub_elements *sub_e = absyn->sub_elements;
	    while (sub_e && strcmp (e->sub_name, sub_e->name))
		sub_e = sub_e->next;
	    if (sub_e)
		e->children = sub_e->elements;
	    else
		yaz_log (YLOG_WARN, "Unresolved reference to sub-elements %s",
		      e->sub_name);
	}
    }
}
/* *ostrich*

   New function, a bit dummy now... I've seen it in zrpn.c... We should build
   more clever regexps...


      //a    ->    ^a/.*$
      //a/b  ->    ^b/a/.*$
      /a     ->    ^a/$
      /a/b   ->    ^b/a/$

      /      ->    none

   pop, 2002-12-13

   Now [] predicates are supported

   pop, 2003-01-17

 */

static const char * mk_xpath_regexp (data1_handle dh, const char *expr) 
{
    const char *p = expr;
    int abs = 1;
    int e = 0;
    char *stack[32];
    char *res_p, *res = 0;
    size_t res_size = 1;
    
    if (*p != '/')
	return ("");
    p++;
    if (*p == '/') 
    { 
	abs =0;
	p++;
    }
    while (*p)
    {
	int is_predicate = 0;
	char *s;
	int i, j;
        for (i = 0; *p && !strchr("/",*p); i++, p++)
	    ;
	res_size += (i+3); /* we'll add / between later .. */
        stack[e] = (char *) nmem_malloc(data1_nmem_get(dh), i+1);
	s = stack[e];
	for (j = 0; j < i; j++)
	{
	    const char *pp = p-i+j;
	    if (*pp == '[')
		is_predicate=1;
	    else if (*pp == ']')
		is_predicate=0;
	    else 
	    {
		if (!is_predicate) {
		    if (*pp == '*') 
			*s++ = '.';
		    *s++ = *pp;
		}
	    }
	}
	*s = 0;
        e++;
        if (*p)
	    p++;
    }
    res_p = res = nmem_malloc(data1_nmem_get(dh), res_size + 10);

    if (stack[e-1][0] == '@')  /* path/@attr spec (leaf is attribute) */
	strcpy(res_p, "/");
    else
	strcpy(res_p, "[^@]*/");  /* path .. (index all cdata below it) */
    res_p = res_p + strlen(res_p);
    while (--e >= 0) {
	sprintf(res_p, "%s/", stack[e]);
	res_p += strlen(stack[e]) + 1;
    }
    if (!abs)
    {
	sprintf(res_p, ".*"); 
	res_p += 2;
    }
    sprintf (res_p, "$");
    res_p++;
    yaz_log(YLOG_DEBUG, "Got regexp: %s", res);
    return res;
}

/* *ostrich*

   added arg xpelement... when called from xelm context, it's 1, saying
   that ! means xpath, not element name as attribute name...

   pop, 2002-12-13
 */
static int parse_termlists (data1_handle dh, data1_termlist ***tpp,
			    char *cp, const char *file, int lineno,
			    const char *element_name, data1_absyn *res,
			    int xpelement)
{
    data1_termlist **tp = *tpp;
    while(1)
    {
	char attname[512], structure[512];
	char *source;
	int r, i;
	int level = 0;
	structure[0] = '\0';
	for (i = 0; cp[i] && i<sizeof(attname)-1; i++)
	    if (strchr(":,", cp[i]))
		break;
	    else
		attname[i] = cp[i];
	if (i == 0)
	{
	    if (*cp)
		yaz_log(YLOG_WARN,
			"%s:%d: Syntax error in termlistspec '%s'",
			file, lineno, cp);
	    break;
	}
	attname[i] = '\0';
	r = 1;
	cp += i;
	if (*cp == ':')
	    cp++;

	for (i = 0; cp[i] && i<sizeof(structure)-1; i++)
	    if (level == 0 && strchr(",", cp[i]))
		break;
	    else
	    {
		structure[i] = cp[i];
		if (cp[i] == '(')
		    level++;
		else if (cp[i] == ')')
		    level--;
	    }
	structure[i] = '\0';
	if (i)
	    r = 2;
	cp += i;
	if (*cp)
	    cp++;  /* skip , */

	*tp = (data1_termlist *)
	    nmem_malloc(data1_nmem_get(dh), sizeof(**tp));
	(*tp)->next = 0;
        
#if NATTR
	(*tp)->index_name = nmem_strdup(data1_nmem_get(dh), element_name);
	if (*attname == '!' && xpelement)
	    (*tp)->index_name = 0;
#else
	if (!xpelement) {
            if (*attname == '!')
                strcpy(attname, element_name);
	}
	if (!((*tp)->att = data1_getattbyname(dh, res->attset, attname))) 
	{
            if ((!xpelement) || (*attname != '!')) {
                yaz_log(YLOG_WARN,
                        "%s:%d: Couldn't find att '%s' in attset",
                        file, lineno, attname);
                return -1;
            } else {
                (*tp)->att = 0;
            }
	}
#endif   
	if (r == 2 && (source = strchr(structure, ':')))
	    *source++ = '\0';   /* cut off structure .. */
	else
	    source = "data";    /* ok: default is leaf data */
	(*tp)->source = (char *)
	    nmem_strdup (data1_nmem_get (dh), source);
	
	if (r < 2) /* is the structure qualified? */
	    (*tp)->structure = "w";
	else 
	    (*tp)->structure = (char *)
		nmem_strdup (data1_nmem_get (dh), structure);
	tp = &(*tp)->next;
    }

    *tpp = tp;
    return 0;
}

/* quinn
 * Converts a 'melm' field[$subfield] pattern to a simple xpath
 */
static int melm2xpath(char *melm, char *buf)
{
    char *dollar;
    char *field = melm;
    char *subfield;
    char *fieldtype;
    if ((dollar = strchr(melm, '$'))) {
	*dollar = '\0';
	subfield = ++dollar;
    } else
	subfield = "";
    if (field[0] == '0' && field[1] == '0')
	fieldtype = "controlfield";
    else
	fieldtype = "datafield";
    sprintf(buf, "/*/%s[@tag=\"%s\"]", fieldtype, field);
    if (*subfield) 
	sprintf(buf + strlen(buf), "/subfield[@code=\"%s\"]", subfield);
    else if (field[0] != '0' || field[1] != '0')
	strcat(buf, "/subfield");
    yaz_log(YLOG_DEBUG, "Created xpath: '%s'", buf);
    return 0;
}

const char *data1_systag_lookup(data1_absyn *absyn, const char *tag,
                                const char *default_value)
{
    struct data1_systag *p = absyn->systags;
    for (; p; p = p->next)
        if (!strcmp(p->name, tag))
            return p->value;
    return default_value;
}

#define l_isspace(c) ((c) == '\t' || (c) == ' ' || (c) == '\n' || (c) == '\r')

int read_absyn_line(FILE *f, int *lineno, char *line, int len,
		    char *argv[], int num)
{
    char *p;
    int argc;
    int quoted = 0;
    
    while ((p = fgets(line, len, f)))
    {
	(*lineno)++;
	while (*p && l_isspace(*p))
	    p++;
	if (*p && *p != '#')
	    break;
    }
    if (!p)
	return 0;
    
    for (argc = 0; *p ; argc++)
    {
	if (*p == '#')  /* trailing comment */
	    break;
	argv[argc] = p;
	while (*p && !(l_isspace(*p) && !quoted)) {
  	  if (*p =='"') quoted = 1 - quoted;
  	  if (*p =='[') quoted = 1;
  	  if (*p ==']') quoted = 0;
	  p++;
	}
	if (*p)
	{
	    *(p++) = '\0';
	    while (*p && l_isspace(*p))
		p++;
	}
    }
    return argc;
}

data1_marctab *data1_absyn_getmarctab(data1_handle dh, data1_absyn *absyn)
{
    return absyn->marc;
}

YAZ_EXPORT data1_element *data1_absyn_getelements(data1_handle dh,
						  data1_absyn *absyn)
{
    return absyn->main_elements;
}

data1_absyn *data1_read_absyn (data1_handle dh, const char *file,
                               int file_must_exist)
{
    data1_sub_elements *cur_elements = NULL;
    data1_xpelement *cur_xpelement = NULL;

    data1_absyn *res = 0;
    FILE *f;
    data1_element **ppl[D1_MAX_NESTING];
    data1_esetname **esetpp;
    data1_maptab **maptabp;
    data1_marctab **marcp;
    data1_termlist *all = 0;
    data1_attset_child **attset_childp;
    data1_tagset **tagset_childp;
    struct data1_systag **systagsp;
    int level = 0;
    int lineno = 0;
    int argc;
    char *argv[50], line[512];

    if (!(f = data1_path_fopen(dh, file, "r")))
    {
	yaz_log(YLOG_WARN|YLOG_ERRNO, "Couldn't open %s", file);
        if (file_must_exist)
            return 0;
    }
    
    res = (data1_absyn *) nmem_malloc(data1_nmem_get(dh), sizeof(*res));
    res->name = 0;
    res->reference = VAL_NONE;
    res->tagset = 0;
    res->encoding = 0;
    res->enable_xpath_indexing = (f ? 0 : 1);
    res->systags = 0;
    systagsp = &res->systags;
    tagset_childp = &res->tagset;

    res->attset = data1_empty_attset (dh);
    attset_childp =  &res->attset->children;

    res->varset = 0;
    res->esetnames = 0;
    esetpp = &res->esetnames;
    res->maptabs = 0;
    maptabp = &res->maptabs;
    res->marc = 0;
    marcp = &res->marc;
    res->sub_elements = NULL;
    res->main_elements = NULL;
    res->xp_elements = NULL;
    
    while (f && (argc = read_absyn_line(f, &lineno, line, 512, argv, 50)))
    {
	char *cmd = *argv;
	if (!strcmp(cmd, "elm") || !strcmp(cmd, "element"))
	{
	    data1_element *new_element;
	    int i;
	    char *p, *sub_p, *path, *name, *termlists;
	    int type, value;
	    data1_termlist **tp;

	    if (argc < 4)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to elm", file, lineno);
		continue;
	    }
	    path = argv[1];
	    name = argv[2];
	    termlists = argv[3];

	    if (!cur_elements)
	    {
                cur_elements = (data1_sub_elements *)
		    nmem_malloc(data1_nmem_get(dh), sizeof(*cur_elements));
	        cur_elements->next = res->sub_elements;
		cur_elements->elements = NULL;
		cur_elements->name = "main";
		res->sub_elements = cur_elements;
		
		level = 0;
    		ppl[level] = &cur_elements->elements;
            }
	    p = path;
	    for (i = 1;; i++)
	    {
		char *e;

		if ((e = strchr(p, '/')))
		    p = e+1;
		else
		    break;
	    }
	    if (i > level+1)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad level increase", file, lineno);
		fclose(f);
		return 0;
	    }
	    level = i;
	    new_element = *ppl[level-1] = data1_mk_element(dh);
	    
	    tp = &new_element->termlists;
	    ppl[level-1] = &new_element->next;
	    ppl[level] = &new_element->children;
	    
	    /* consider subtree (if any) ... */
	    if ((sub_p = strchr (p, ':')) && sub_p[1])
	    {
		*sub_p++ = '\0';
		new_element->sub_name =
		    nmem_strdup (data1_nmem_get(dh), sub_p);		
	    }
	    /* well-defined tag */
	    if (sscanf(p, "(%d,%d)", &type, &value) == 2)
	    {
		if (!res->tagset)
		{
		    yaz_log(YLOG_WARN, "%s:%d: No tagset loaded", file, lineno);
		    fclose(f);
		    return 0;
		}
		if (!(new_element->tag = data1_gettagbynum (dh, res->tagset,
							    type, value)))
		{
		    yaz_log(YLOG_WARN, "%s:%d: Couldn't find tag %s in tagset",
			 file, lineno, p);
		    fclose(f);
		    return 0;
		}
	    }
	    /* private tag */
	    else if (*p)
	    {
		data1_tag *nt =
		    new_element->tag = (data1_tag *)
		    nmem_malloc(data1_nmem_get (dh),
				sizeof(*new_element->tag));
		nt->which = DATA1T_string;
		nt->value.string = nmem_strdup(data1_nmem_get (dh), p);
		nt->names = (data1_name *)
		    nmem_malloc(data1_nmem_get(dh), 
				sizeof(*new_element->tag->names));
		nt->names->name = nt->value.string;
		nt->names->next = 0;
		nt->kind = DATA1K_string;
		nt->next = 0;
		nt->tagset = 0;
	    }
	    else
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad element", file, lineno);
		fclose(f);
		return 0;
	    }
	    /* parse termList definitions */
	    p = termlists;
	    if (*p != '-')
	    {
		assert (res->attset);
		
		if (parse_termlists (dh, &tp, p, file, lineno, name, res, 0))
		{
		    fclose (f);
		    return 0;
		}
	        *tp = all; /* append any ALL entries to the list */
	    }
	    new_element->name = nmem_strdup(data1_nmem_get (dh), name);
	}
	/* *ostrich*
	   New code to support xelm directive
	   for each xelm a dfa is built. xelms are stored in res->xp_elements
           
	   maybe we should use a simple sscanf instead of dfa?
           
	   pop, 2002-12-13

	   Now [] predicates are supported. regexps and xpath structure is
	   a bit redundant, however it's comfortable later...

	   pop, 2003-01-17
	*/

	else if (!strcmp(cmd, "xelm") || !strcmp(cmd, "melm")) {

	    int i;
	    char *p, *xpath_expr, *termlists;
	    const char *regexp;
	    struct DFA *dfa = dfa = dfa_init();
	    data1_termlist **tp;
	    char melm_xpath[128];
            
	    if (argc < 3)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to xelm", file, lineno);
		continue;
	    }

	    if (!strcmp(cmd, "melm")) {
		if (melm2xpath(argv[1], melm_xpath) < 0)
		    continue;
		xpath_expr = melm_xpath;
	    } else {
		xpath_expr = argv[1];
	    }
	    termlists = argv[2];
	    regexp = mk_xpath_regexp(dh, xpath_expr);
	    i = dfa_parse (dfa, &regexp);
	    if (i || *regexp) {
                yaz_log(YLOG_WARN, "%s:%d: Bad xpath to xelm", file, lineno);
                dfa_delete (&dfa);
                continue;
	    }
            
	    if (!cur_xpelement)
	    {
                cur_xpelement = (data1_xpelement *)
		    nmem_malloc(data1_nmem_get(dh), sizeof(*cur_xpelement));
		res->xp_elements = cur_xpelement;
            } else {
                cur_xpelement->next = (data1_xpelement *)
                    nmem_malloc(data1_nmem_get(dh), sizeof(*cur_xpelement));
                cur_xpelement = cur_xpelement->next;
	    }
	    cur_xpelement->next = NULL;
	    cur_xpelement->xpath_expr = nmem_strdup(data1_nmem_get (dh), 
						    xpath_expr); 
	    
	    dfa_mkstate (dfa);
	    cur_xpelement->dfa = dfa;

#ifdef ENHANCED_XELM 
            cur_xpelement->xpath_len =
                zebra_parse_xpath_str(xpath_expr, 
                                      cur_xpelement->xpath, XPATH_STEP_COUNT,
                                      data1_nmem_get(dh));
            
	    /*
	    dump_xp_steps(cur_xpelement->xpath,cur_xpelement->xpath_len);
	    */
#endif
	    cur_xpelement->termlists = 0;
	    tp = &cur_xpelement->termlists;
            
	    /* parse termList definitions */
	    p = termlists;
	    if (*p != '-')
	    {
		assert (res->attset);
		
		if (parse_termlists (dh, &tp, p, file, lineno,
                                     xpath_expr, res, 1))
		{
		    fclose (f);
		    return 0;
		}
	        *tp = all; /* append any ALL entries to the list */
	    }
	}
 	else if (!strcmp(cmd, "section"))
	{
	    char *name;
	    
	    if (argc < 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to section",
                        file, lineno);
		continue;
	    }
	    name = argv[1];
	    
            cur_elements = (data1_sub_elements *)
		nmem_malloc(data1_nmem_get(dh), sizeof(*cur_elements));
	    cur_elements->next = res->sub_elements;
	    cur_elements->elements = NULL;
	    cur_elements->name = nmem_strdup (data1_nmem_get(dh), name);
	    res->sub_elements = cur_elements;
	    
	    level = 0;
    	    ppl[level] = &cur_elements->elements;
	}
        else if (!strcmp(cmd, "xpath"))
        {
            if (argc != 2)
            {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to 'xpath' directive",
		     file, lineno);
		continue;
            }
            if (!strcmp(argv[1], "enable"))
                res->enable_xpath_indexing = 1;
            else if (!strcmp (argv[1], "disable"))
                res->enable_xpath_indexing = 0;
            else
            {
		yaz_log(YLOG_WARN, "%s:%d: Expecting disable/enable "
                        "after 'xpath' directive", file, lineno);
            }
        }
	else if (!strcmp(cmd, "all"))
	{
	    data1_termlist **tp = &all;
	    if (all)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Too many 'all' directives - ignored",
		     file, lineno);
		continue;
	    }
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to 'all' directive",
		     file, lineno);
		continue;
	    }
	    if (parse_termlists (dh, &tp, argv[1], file, lineno, 0, res, 0))
	    {
		fclose (f);
		return 0;
	    }
	}
	else if (!strcmp(cmd, "name"))
	{
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to name directive",
		     file, lineno);
		continue;
	    }
	    res->name = nmem_strdup(data1_nmem_get(dh), argv[1]);
	}
	else if (!strcmp(cmd, "reference"))
	{
	    char *name;
	    
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to reference",
		     file, lineno);
		continue;
	    }
	    name = argv[1];
	    if ((res->reference = oid_getvalbyname(name)) == VAL_NONE)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Unknown tagset ref '%s'", 
		     file, lineno, name);
		continue;
	    }
	}
	else if (!strcmp(cmd, "attset"))
	{
#if NATTR
	    yaz_log(YLOG_WARN, "%s:%d: attset obsolete", file, lineno);
#else
	    char *name;
	    data1_attset *attset;
	    
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to attset",
		     file, lineno);
		continue;
	    }
	    name = argv[1];
	    if (!(attset = data1_get_attset (dh, name)))
	    {
		yaz_log(YLOG_WARN, "%s:%d: Couldn't find attset  %s",
		     file, lineno, name);
		continue;
	    }
	    *attset_childp = (data1_attset_child *)
		nmem_malloc (data1_nmem_get(dh), sizeof(**attset_childp));
	    (*attset_childp)->child = attset;
	    (*attset_childp)->next = 0;
	    attset_childp = &(*attset_childp)->next;
#endif
	}
	else if (!strcmp(cmd, "tagset"))
	{
	    char *name;
	    int type = 0;
	    if (argc < 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args to tagset",
		     file, lineno);
		continue;
	    }
	    name = argv[1];
	    if (argc == 3)
		type = atoi(argv[2]);
	    *tagset_childp = data1_read_tagset (dh, name, type);
	    if (!(*tagset_childp))
	    {
		yaz_log(YLOG_WARN, "%s:%d: Couldn't load tagset %s",
		     file, lineno, name);
		continue;
	    }
	    tagset_childp = &(*tagset_childp)->next;
	}
	else if (!strcmp(cmd, "varset"))
	{
	    char *name;

	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args in varset",
		     file, lineno);
		continue;
	    }
	    name = argv[1];
	    if (!(res->varset = data1_read_varset (dh, name)))
	    {
		yaz_log(YLOG_WARN, "%s:%d: Couldn't load Varset %s",
		     file, lineno, name);
		continue;
	    }
	}
	else if (!strcmp(cmd, "esetname"))
	{
	    char *name, *fname;

	    if (argc != 3)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args in esetname",
		     file, lineno);
		continue;
	    }
	    name = argv[1];
	    fname = argv[2];
	    
	    *esetpp = (data1_esetname *)
		nmem_malloc(data1_nmem_get(dh), sizeof(**esetpp));
	    (*esetpp)->name = nmem_strdup(data1_nmem_get(dh), name);
	    (*esetpp)->next = 0;
	    if (*fname == '@')
		(*esetpp)->spec = 0;
	    else if (!((*esetpp)->spec = data1_read_espec1 (dh, fname)))
	    {
		yaz_log(YLOG_WARN, "%s:%d: Espec-1 read failed for %s",
		     file, lineno, fname);
		continue;
	    }
	    esetpp = &(*esetpp)->next;
	}
	else if (!strcmp(cmd, "maptab"))
	{
	    char *name;
	    
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # of args for maptab",
                     file, lineno);
		continue;
	    }
	    name = argv[1];
	    if (!(*maptabp = data1_read_maptab (dh, name)))
	    {
		yaz_log(YLOG_WARN, "%s:%d: Couldn't load maptab %s",
                     file, lineno, name);
		continue;
	    }
	    maptabp = &(*maptabp)->next;
	}
	else if (!strcmp(cmd, "marc"))
	{
	    char *name;
	    
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # or args for marc",
		     file, lineno);
		continue;
	    }
	    name = argv[1];
	    if (!(*marcp = data1_read_marctab (dh, name)))
	    {
		yaz_log(YLOG_WARN, "%s:%d: Couldn't read marctab %s",
                     file, lineno, name);
		continue;
	    }
	    marcp = &(*marcp)->next;
	}
	else if (!strcmp(cmd, "encoding"))
	{
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # or args for encoding",
		     file, lineno);
		continue;
	    }
            res->encoding = nmem_strdup (data1_nmem_get(dh), argv[1]);
	}
        else if (!strcmp(cmd, "systag"))
        {
            if (argc != 3)
            {
		yaz_log(YLOG_WARN, "%s:%d: Bad # or args for systag",
		     file, lineno);
		continue;
            }
            *systagsp = nmem_malloc (data1_nmem_get(dh), sizeof(**systagsp));

            (*systagsp)->name = nmem_strdup(data1_nmem_get(dh), argv[1]);
            (*systagsp)->value = nmem_strdup(data1_nmem_get(dh), argv[2]);
            systagsp = &(*systagsp)->next;
        }
	else
	{
	    yaz_log(YLOG_WARN, "%s:%d: Unknown directive '%s'", file, 
                    lineno, cmd);
	    continue;
	}
    }
    if (f)
        fclose(f);
    
    for (cur_elements = res->sub_elements; cur_elements;
	 cur_elements = cur_elements->next)
    {
	if (!strcmp (cur_elements->name, "main"))
	    res->main_elements = cur_elements->elements;
	fix_element_ref (dh, res, cur_elements->elements);
    }
    *systagsp = 0;
    yaz_log(YLOG_DEBUG, "%s: data1_read_absyn end", file);
    return res;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

