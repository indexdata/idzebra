/*
 * Copyright (C) 1994-2001, Index Data
 * All rights reserved.
 *
 * $Log: recgrs.c,v $
 * Revision 1.41  2001-05-22 21:01:47  adam
 * Removed print of data1 tree on stdout so that inetd works again.
 *
 * Revision 1.40  2001/03/29 21:31:31  adam
 * Fixed "record begin" for Tcl filter.
 *
 * Revision 1.39  2000/12/05 19:09:15  adam
 * Fixed problem where indexer could crash if abstract syntax was undefined.
 *
 * Revision 1.38  2000/12/05 14:44:58  adam
 * Fixed minor bug that could cause zmbol to break it data were emitted
 * with not parent tags.
 *
 * Revision 1.37  2000/12/05 12:22:53  adam
 * Termlist source implemented (so that we can index values of XML/SGML
 * attributes).
 *
 * Revision 1.36  2000/12/05 10:01:44  adam
 * Fixed bug regarding user-defined attribute sets.
 *
 * Revision 1.35  2000/11/29 15:21:31  adam
 * Fixed problem with passwd db.
 *
 * Revision 1.34  2000/02/25 13:24:49  adam
 * Fixed bug regarding pointer conversion that showed up on OSF V5.
 *
 * Revision 1.33  1999/11/30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.32  1999/09/07 07:19:21  adam
 * Work on character mapping. Implemented replace rules.
 *
 * Revision 1.31  1999/07/14 10:56:43  adam
 * Fixed potential memory leak.
 *
 * Revision 1.30  1999/07/06 12:26:41  adam
 * Retrieval handler obeys schema and handles XML transfer syntax.
 *
 * Revision 1.29  1999/05/26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.28  1999/05/21 12:00:17  adam
 * Better diagnostics for extraction process.
 *
 * Revision 1.27  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.26  1999/03/02 16:15:44  quinn
 * Added "tagsysno" and "tagrank" directives to zebra.cfg.
 *
 * Revision 1.25  1999/02/18 15:01:26  adam
 * Minor changes.
 *
 * Revision 1.24  1999/02/02 14:51:28  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.23  1998/10/18 07:51:10  adam
 * Changed one logf call.
 *
 * Revision 1.22  1998/10/16 08:14:37  adam
 * Updated record control system.
 *
 * Revision 1.21  1998/07/01 09:16:10  adam
 * Element localno only added when it's greater than 0.
 *
 * Revision 1.20  1998/05/20 10:12:26  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.19  1998/03/11 11:19:05  adam
 * Changed the way sequence numbers are generated.
 *
 * Revision 1.18  1998/03/05 08:41:31  adam
 * Minor changes.
 *
 * Revision 1.17  1998/02/10 12:03:06  adam
 * Implemented Sort.
 *
 * Revision 1.16  1998/01/29 13:38:17  adam
 * Fixed problem with mapping to record with unknown schema.
 *
 * Revision 1.15  1998/01/26 10:37:57  adam
 * Better diagnostics.
 *
 * Revision 1.14  1997/11/06 11:41:01  adam
 * Implemented "begin variant" for the sgml.regx filter.
 *
 * Revision 1.13  1997/10/31 12:35:44  adam
 * Added a few log statements.
 *
 * Revision 1.12  1997/10/29 12:02:22  adam
 * Using oid_ent_to_oid used instead of the non thread-safe oid_getoidbyent.
 *
 * Revision 1.11  1997/10/27 14:34:00  adam
 * Work on generic character mapping depending on "structure" field
 * in abstract syntax file.
 *
 * Revision 1.10  1997/09/18 08:59:21  adam
 * Extra generic handle for the character mapping routines.
 *
 * Revision 1.9  1997/09/17 12:19:21  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.8  1997/09/09 13:38:14  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.7  1997/09/05 15:30:10  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.6  1997/09/04 13:54:40  adam
 * Added MARC filter - type grs.marc.<syntax> where syntax refers
 * to abstract syntax. New method tellf in retrieve/extract method.
 *
 * Revision 1.5  1997/07/15 16:29:03  adam
 * Initialized dummy variable to keep checker gcc happy.
 *
 * Revision 1.4  1997/04/30 08:56:08  quinn
 * null
 *
 * Revision 1.2  1996/10/11  16:06:43  quinn
 * Revision 1.3  1997/02/24 10:41:50  adam
 * Cleanup of code and commented out the "end element-end-record" code.
 *
 * Revision 1.2  1996/10/11 16:06:43  quinn
 * Fixed arguments to nodetogr
 *
 * Revision 1.1  1996/10/11  10:57:25  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 * Revision 1.29  1996/10/08 10:30:21  quinn
 * Fixed type mismatch
 *
 * Revision 1.28  1996/10/07  16:06:40  quinn
 * Added SOIF support
 *
 * Revision 1.27  1996/06/11  10:54:12  quinn
 * Relevance work
 *
 * Revision 1.26  1996/06/06  12:08:45  quinn
 * Added showRecord function
 *
 * Revision 1.25  1996/06/04  14:18:53  quinn
 * Charmap work
 *
 * Revision 1.24  1996/06/04  13:27:54  quinn
 * More work on charmapping
 *
 * Revision 1.23  1996/06/04  10:19:01  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.22  1996/06/03  10:15:27  quinn
 * Various character-mapping.
 *
 * Revision 1.21  1996/05/31  13:27:24  quinn
 * Character-conversion in phrases, too.
 *
 * Revision 1.19  1996/05/16  15:31:14  quinn
 * a7
 *
 * Revision 1.18  1996/05/09  07:28:56  quinn
 * Work towards phrases and multiple registers
 *
 * Revision 1.17  1996/05/01  13:46:37  adam
 * First work on multiple records in one file.
 * New option, -offset, to the "unread" command in the filter module.
 *
 * Revision 1.16  1996/01/17  14:57:54  adam
 * Prototype changed for reader functions in extract/retrieve. File
 *  is identified by 'void *' instead of 'int.
 *
 * Revision 1.15  1996/01/08  19:15:47  adam
 * New input filter that works!
 *
 * Revision 1.14  1995/12/15  12:36:11  adam
 * Retrieval calls data1_read_regx when subType is specified.
 *
 * Revision 1.13  1995/12/15  12:24:43  quinn
 * *** empty log message ***
 *
 * Revision 1.12  1995/12/15  12:20:28  quinn
 * *** empty log message ***
 *
 * Revision 1.11  1995/12/15  12:07:57  quinn
 * Changed extraction strategy.
 *
 * Revision 1.10  1995/12/14  11:10:48  quinn
 * Explain work
 *
 * Revision 1.9  1995/12/13  17:14:05  quinn
 * *** empty log message ***
 *
 * Revision 1.8  1995/12/13  15:33:18  quinn
 * *** empty log message ***
 *
 * Revision 1.7  1995/12/13  13:45:39  quinn
 * Changed data1 to use nmem.
 *
 * Revision 1.6  1995/12/04  14:22:30  adam
 * Extra arg to recType_byName.
 * Started work on new regular expression parsed input to
 * structured records.
 *
 * Revision 1.5  1995/11/28  14:18:37  quinn
 * Set output_format.
 *
 * Revision 1.4  1995/11/21  13:14:49  quinn
 * Fixed end-of-data-field problem (maybe).
 *
 * Revision 1.3  1995/11/15  19:13:09  adam
 * Work on record management.
 *
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
        if (!memcmp (type, gh->type->type, cp-type))
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
    struct grs_handler *gh = (struct grs_handler *) malloc (sizeof(*gh));
    gh->next = h->handlers;
    h->handlers = gh;
    gh->initFlag = 0;
    gh->clientData = 0;
    gh->type = t;
}

static void *grs_init(RecType recType)
{
    struct grs_handlers *h = (struct grs_handlers *) malloc (sizeof(*h));
    h->handlers = 0;

    grs_add_handler (h, recTypeGrs_sgml);
    grs_add_handler (h, recTypeGrs_regx);
#if HAVE_TCL_H
    grs_add_handler (h, recTypeGrs_tcl);
#endif
    grs_add_handler (h, recTypeGrs_marc);
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
	free (gh);
	gh = gh_next;
    }
    free (h);
}

static void index_tag (data1_node *par, data1_node *n,
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
	char xattr[512];
	/* consider source */
	wrd->string = 0;
	
	if (!strcmp (tlist->source, "data") && n->which == DATA1N_data)
	{
	    wrd->string = n->u.data.data;
	    wrd->length = n->u.data.len;
	}
	else if (sscanf (tlist->source, "attr(%511[^)])", xattr) == 1 &&
	    n->which == DATA1N_tag)
	{
	    data1_xattr *p = n->u.tag.attributes;
	    while (p && strcmp (p->name, xattr))
		p = p->next;
	    if (p)
	    {
		wrd->string = p->value;
		wrd->length = strlen(p->value);
	    }
	}
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
		printf (" data=\"");
		for (i = 0; i<wrd->length && i < 8; i++)
		    fputc (wrd->string[i], stdout);
		fputc ('"', stdout);
		if (wrd->length > 8)
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

static int dumpkeys(data1_node *n, struct recExtractCtrl *p, int level)
{
    RecWord wrd;
    (*p->init)(p, &wrd);      /* set defaults */
    for (; n; n = n->next)
    {
	if (p->flagShowRecords) /* display element description to user */
	{
	    if (n->which == DATA1N_root)
	    {
		printf("%*s", level * 4, "");
		printf("Record type: '%s'\n", n->u.root.absyn->name);
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

	if (n->child)
	    if (dumpkeys(n->child, p, level + 1) < 0)
		return -1;

	if (n->which == DATA1N_tag)
	{
	    index_tag (n, n, p, level, &wrd);
	}

	if (n->which == DATA1N_data)
	{
	    data1_node *par = get_parent_tag(p->dh, n);

	    if (p->flagShowRecords)
	    {
		printf("%*s", level * 4, "");
		printf("Data: ");
		if (n->u.data.len > 32)
		    printf("'%.24s ... %.6s'\n", n->u.data.data,
			   n->u.data.data + n->u.data.len-6);
		else if (n->u.data.len > 0)
		    printf("'%.*s'\n", n->u.data.len, n->u.data.data);
		else
		    printf("NULL\n");
	    }

	    if (par)
		index_tag (par, n, p, level, &wrd);
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

    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_SCHEMA;
    oe.value = n->u.root.absyn->reference;

    if ((oid_ent_to_oid (&oe, oidtmp)))
	(*p->schemaAdd)(p, oidtmp);

    return dumpkeys(n, p, 0);
}

static int grs_extract_sub(struct grs_handlers *h, struct recExtractCtrl *p,
			   NMEM mem)
{
    data1_node *n;
    struct grs_read_info gri;
    oident oe;
    int oidtmp[OID_SIZE];

    gri.readf = p->readf;
    gri.seekf = p->seekf;
    gri.tellf = p->tellf;
    gri.endf = p->endf;
    gri.fh = p->fh;
    gri.offset = p->offset;
    gri.mem = mem;
    gri.dh = p->dh;

    if (read_grs_type (h, &gri, p->subType, &n))
	return RECCTRL_EXTRACT_ERROR;
    if (!n)
        return RECCTRL_EXTRACT_EOF;
    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_SCHEMA;
    if (!n->u.root.absyn)
        return RECCTRL_EXTRACT_ERROR;
    oe.value = n->u.root.absyn->reference;
    if ((oid_ent_to_oid (&oe, oidtmp)))
	(*p->schemaAdd)(p, oidtmp);

    if (dumpkeys(n, p, 0) < 0)
    {
	data1_free_tree(p->dh, n);
	return RECCTRL_EXTRACT_ERROR;
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
	    logf(LOG_LOG, "Unknown esetname '%s'", c->u.simple->u.generic);
	    return 25; /* invalid esetname */
	}
	logf(LOG_DEBUG, "Esetname '%s' in simple compspec",
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
			logf(LOG_LOG, "Unknown esetname '%s'",
			     p->u.elementSetName);
			return 25; /* invalid esetname */
		    }
		    logf(LOG_DEBUG, "Esetname '%s' in complex compspec",
			 p->u.elementSetName);
		    espec = eset->spec;
		    break;
		case Z_ElementSpec_externalSpec:
		    if (p->u.externalSpec->which == Z_External_espec1)
		    {
			logf(LOG_DEBUG, "Got Espec-1");
			espec = p->u.externalSpec-> u.espec1;
		    }
		    else
		    {
			logf(LOG_LOG, "Unknown external espec.");
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
        logf (LOG_DEBUG, "Element: Espec-1 match");
	return data1_doespec1(dh, n, espec);
    }
    else
    {
	logf (LOG_DEBUG, "Element: all match");
	return -1;
    }
}

static int grs_retrieve(void *clientData, struct recRetrieveCtrl *p)
{
    data1_node *node = 0, *onode = 0;
    data1_node *dnew;
    data1_maptab *map;
    int res, selected = 0;
    NMEM mem;
    struct grs_read_info gri;
    char *tagname;
    struct grs_handlers *h = (struct grs_handlers *) clientData;
    int requested_schema = VAL_NONE;
    
    mem = nmem_create();
    gri.readf = p->readf;
    gri.seekf = p->seekf;
    gri.tellf = p->tellf;
    gri.endf = NULL;
    gri.fh = p->fh;
    gri.offset = 0;
    gri.mem = mem;
    gri.dh = p->dh;

    logf (LOG_DEBUG, "grs_retrieve");
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
#if 0
    data1_pr_tree (p->dh, node, stdout);
#endif
    logf (LOG_DEBUG, "grs_retrieve: size");
    if ((dnew = data1_insert_taggeddata(p->dh, node, node,
				       "size", mem)))
    {
	dnew->u.data.what = DATA1I_text;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->recordSize);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    tagname = res_get_def(p->res, "tagrank", "rank");
    if (strcmp(tagname, "0") && p->score >= 0 &&
	(dnew = data1_insert_taggeddata(p->dh, node, node, tagname, mem)))
    {
        logf (LOG_DEBUG, "grs_retrieve: %s", tagname);
	dnew->u.data.what = DATA1I_num;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->score);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    tagname = res_get_def(p->res, "tagsysno", "localControlNumber");
    if (strcmp(tagname, "0") && p->localno > 0 &&
   	 (dnew = data1_insert_taggeddata(p->dh, node, node, tagname, mem)))
    {
        logf (LOG_DEBUG, "grs_retrieve: %s", tagname);
	dnew->u.data.what = DATA1I_text;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->localno);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    if (p->comp && p->comp->which == Z_RecordComp_complex &&
	p->comp->u.complex->generic &&
	p->comp->u.complex->generic->schema)
    {
	oident *oe = oid_getentbyoid (p->comp->u.complex->generic->schema);
	if (oe)
	    requested_schema = oe->value;
    }

    /* If schema has been specified, map if possible, then check that
     * we got the right one 
     */
    if (requested_schema != VAL_NONE)
    {
	logf (LOG_DEBUG, "grs_retrieve: schema mapping");
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
    logf (LOG_DEBUG, "grs_retrieve: syntax mapping");
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
    logf (LOG_DEBUG, "grs_retrieve: schemaIdentifier");
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
	    *(p++) = '\0';
		
	    if ((dnew = data1_insert_taggeddata(dh, node, node,
						"schemaIdentifier", mem)))
	    {
		dnew->u.data.what = DATA1I_oid;
		dnew->u.data.data = (char *) nmem_malloc(mem, p - tmp);
		memcpy(dnew->u.data.data, tmp, p - tmp);
		dnew->u.data.len = p - tmp;
	    }
	}
    }

    logf (LOG_DEBUG, "grs_retrieve: element spec");
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
    logf (LOG_DEBUG, "grs_retrieve: transfer syntax mapping");
    switch (p->output_format = (p->input_format != VAL_NONE ?
				p->input_format : VAL_SUTRS))
    {
	data1_marctab *marctab;
        int dummy;
	
    case VAL_TEXT_XML:
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
