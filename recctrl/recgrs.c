/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recgrs.c,v $
 * Revision 1.18  1998-03-05 08:41:31  adam
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
#ifndef WINDOWS
#include <unistd.h>
#endif

#include <log.h>
#include <oid.h>

#include <recctrl.h>
#include "grsread.h"

#define GRS_MAX_WORD 512

static int seqno = 0;

static data1_node *read_grs_type (struct grs_read_info *p, const char *type)
{
    static struct {
        char *type;
        data1_node *(*func)(struct grs_read_info *p);
    } tab[] = {
        { "sgml",  grs_read_sgml },
        { "regx",  grs_read_regx },
        { "marc",  grs_read_marc },
        { NULL, NULL }
    };
    const char *cp = strchr (type, '.');
    int i;

    if (cp == NULL || cp == type)
    {
        cp = strlen(type) + type;
        *p->type = 0;
    }
    else
        strcpy (p->type, cp+1);
    for (i=0; tab[i].type; i++)
    {
        if (!memcmp (type, tab[i].type, cp-type))
            return (tab[i].func)(p);
    }
    return NULL;
}

static void grs_init(void)
{
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

	if (n->which == DATA1N_data)
	{
	    data1_node *par = get_parent_tag(p->dh, n);
	    data1_termlist *tlist = 0;
	    data1_datatype dtype = DATA1K_string;

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

	    assert(par);

	    /*
	     * cycle up towards the root until we find a tag with an att..
	     * this has the effect of indexing locally defined tags with
	     * the attribute of their ancestor in the record.
	     */

	    while (!par->u.tag.element)
		if (!par->parent || !(par=get_parent_tag(p->dh, par->parent)))
		    break;
	    if (!par || !(tlist = par->u.tag.element->termlists))
		continue;
	    if (par->u.tag.element->tag)
		dtype = par->u.tag.element->tag->kind;
	    for (; tlist; tlist = tlist->next)
	    {
		if (p->flagShowRecords)
		{
		    printf("%*sIdx: [%s]", (level + 1) * 4, "",
			   tlist->structure);
		    printf("%s:%s [%d]\n",
			   tlist->att->parent->name,
			   tlist->att->name, tlist->att->value);
		}
		else
		{
		    wrd.reg_type = *tlist->structure;
		    wrd.seqno = seqno;
		    wrd.string = n->u.data.data;
		    wrd.length = n->u.data.len;
		    wrd.attrSet = tlist->att->parent->ordinal;
		    wrd.attrUse = tlist->att->locals->local;
		    (*p->add)(&wrd);
		    seqno = wrd.seqno;
		}
	    }
	}
	if (p->flagShowRecords && n->which == DATA1N_root)
	{
	    printf("%*s-------------\n\n", level * 4, "");
	}
    }
    return 0;
}

static int grs_extract(struct recExtractCtrl *p)
{
    data1_node *n;
    NMEM mem;
    struct grs_read_info gri;
    seqno = 0;

    mem = nmem_create (); 
    gri.readf = p->readf;
    gri.seekf = p->seekf;
    gri.tellf = p->tellf;
    gri.endf = p->endf;
    gri.fh = p->fh;
    gri.offset = p->offset;
    gri.mem = mem;
    gri.dh = p->dh;

    n = read_grs_type (&gri, p->subType);
    if (!n)
        return -1;
    if (dumpkeys(n, p, 0) < 0)
    {
	data1_free_tree(p->dh, n);
	return -2;
    }
    data1_free_tree(p->dh, n);
    nmem_destroy(mem);
    return 0;
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
		    switch (p->which)
		    {
			case Z_ElementSpec_elementSetName:
			    if (!(eset =
				  data1_getesetbyname(dh,
						      n->u.root.absyn,
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
	    else
		return 26; /* fix */
    }
    if (espec)
    {
        logf (LOG_LOG, "Element: Espec-1 match");
	return data1_doespec1(dh, n, espec);
    }
    else
    {
	logf (LOG_DEBUG, "Element: all match");
	return -1;
    }
}

static int grs_retrieve(struct recRetrieveCtrl *p)
{
    data1_node *node = 0, *onode = 0;
    data1_node *dnew;
    data1_maptab *map;
    int res, selected = 0;
    NMEM mem;
    struct grs_read_info gri;
    
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
    node = read_grs_type (&gri, p->subType);
    if (!node)
    {
	p->diagnostic = 14;
        nmem_destroy (mem);
	return 0;
    }
    logf (LOG_DEBUG, "grs_retrieve: size");
    if ((dnew = data1_insert_taggeddata(p->dh, node, node,
				       "size", mem)))
    {
	dnew->u.data.what = DATA1I_text;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->recordSize);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    logf (LOG_DEBUG, "grs_retrieve: score");
    if (p->score >= 0 && (dnew =
			  data1_insert_taggeddata(p->dh, node,
						  node, "rank",
						  mem)))
    {
	dnew->u.data.what = DATA1I_num;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->score);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    logf (LOG_DEBUG, "grs_retrieve: localControlNumber");
    if ((dnew = data1_insert_taggeddata(p->dh, node, node,
				       "localControlNumber", mem)))
    {
	dnew->u.data.what = DATA1I_text;
	dnew->u.data.data = dnew->lbuf;
	sprintf(dnew->u.data.data, "%d", p->localno);
	dnew->u.data.len = strlen(dnew->u.data.data);
    }

    logf (LOG_DEBUG, "grs_retrieve: schemaIdentifier");
    if (p->input_format == VAL_GRS1 && node->u.root.absyn &&
	node->u.root.absyn->reference != VAL_NONE)
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
		dnew->u.data.data = nmem_malloc(mem, p - tmp);
		memcpy(dnew->u.data.data, tmp, p - tmp);
		dnew->u.data.len = p - tmp;
	    }
	}
    }

    logf (LOG_DEBUG, "grs_retrieve: schema mapping");
    /*
     * Does the requested format match a known schema-mapping? (this reflects
     * the overlap of schema and formatting which is inherent in the MARC
     * family)
     * NOTE: This should look at the schema-specification in the compspec
     * as well.
     */
    for (map = node->u.root.absyn->maptabs; map; map = map->next)
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

    logf (LOG_DEBUG, "grs_retrieve: transfer syntax mapping");
    switch (p->output_format = (p->input_format != VAL_NONE ?
	p->input_format : VAL_SUTRS))
    {
	data1_marctab *marctab;
        int dummy;

	case VAL_GRS1:
	    dummy = 0;
	    if (!(p->rec_buf = data1_nodetogr(p->dh, node, selected,
					      p->odr, &dummy)))
		p->diagnostic = 238; /* not available in requested syntax */
	    else
		p->rec_len = -1;
	    break;
	case VAL_EXPLAIN:
	    if (!(p->rec_buf = data1_nodetoexplain(p->dh, node, selected,
						   p->odr)))
		p->diagnostic = 238;
	    else
		p->rec_len = -1;
	    break;
	case VAL_SUMMARY:
	    if (!(p->rec_buf = data1_nodetosummary(p->dh, node, selected,
						   p->odr)))
		p->diagnostic = 238;
	    else
		p->rec_len = -1;
	    break;
	case VAL_SUTRS:
	    if (!(p->rec_buf = data1_nodetobuf(p->dh, node, selected,
		(int*)&p->rec_len)))
		p->diagnostic = 238;
	    break;
	case VAL_SOIF:
	    if (!(p->rec_buf = data1_nodetosoif(p->dh, node, selected,
						(int*)&p->rec_len)))
		p->diagnostic = 238;
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
						selected,
						(int*)&p->rec_len)))
	    {
		p->diagnostic = 238;
		break;
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
    grs_extract,
    grs_retrieve
};

RecType recTypeGrs = &grs_type;
