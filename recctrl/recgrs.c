/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recgrs.c,v $
 * Revision 1.6  1997-09-04 13:54:40  adam
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
#include <unistd.h>

#include <log.h>
#include <oid.h>

#include <recctrl.h>
#include <charmap.h>
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

static void dumpkeys_word(data1_node *n, struct recExtractCtrl *p,
    data1_att *att)
{
    char *b = n->u.data.data;
    int remain;
    char **map = 0;

    remain = n->u.data.len - (b - n->u.data.data);
    if (remain > 0)
	map = (*p->map_chrs_input)(&b, remain);

    while (map)
    {
	RecWord wrd;
	char buf[GRS_MAX_WORD+1];
	int i, remain;

	/* Skip spaces */
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = n->u.data.len - (b - n->u.data.data);
	    if (remain > 0)
		map = (*p->map_chrs_input)(&b, remain);
	    else
		map = 0;
	}
	if (!map)
	    break;
	i = 0;
	while (map && *map && **map != *CHR_SPACE)
	{
	    char *cp = *map;

	    while (i < GRS_MAX_WORD && *cp)
		buf[i++] = *(cp++);
	    remain = n->u.data.len - (b - n->u.data.data);
	    if (remain > 0)
		map = (*p->map_chrs_input)(&b, remain);
	    else
		map = 0;
	}
	if (!i)
	    return;
	buf[i] = '\0';
	(*p->init)(&wrd);      /* set defaults */
	wrd.which = Word_String;
	wrd.seqno = seqno++;
	wrd.u.string = buf;
	wrd.attrSet = att->parent->ordinal;
	wrd.attrUse = att->locals->local;
	(*p->add)(&wrd);
    }
}

static void dumpkeys_phrase(data1_node *n, struct recExtractCtrl *p,
    data1_att *att)
{
    char *b = n->u.data.data;
    char buf[GRS_MAX_WORD+1], **map = 0;
    RecWord wrd;
    int i = 0, remain;

    remain = n->u.data.len - (b - n->u.data.data);
    if (remain > 0)
	map = (*p->map_chrs_input)(&b, remain);

    while (remain > 0 && i < GRS_MAX_WORD)
    {
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = n->u.data.len - (b - n->u.data.data);
	    if (remain > 0)
		map = (*p->map_chrs_input)(&b, remain);
	    else
		map = 0;
	}
	if (!map)
	    break;

	if (i && i < GRS_MAX_WORD)
	    buf[i++] = *CHR_SPACE;
	while (map && *map && **map != *CHR_SPACE)
	{
	    char *cp = *map;

	    if (i >= GRS_MAX_WORD)
		break;
	    while (i < GRS_MAX_WORD && *cp)
		buf[i++] = *(cp++);
	    remain = n->u.data.len - (b - n->u.data.data);
	    if (remain > 0)
		map = (*p->map_chrs_input)(&b, remain);
	    else
		map = 0;
	}
    }
    if (!i)
	return;
    buf[i] = '\0';
    (*p->init)(&wrd);
    wrd.which = Word_Phrase;
    wrd.seqno = seqno++;
    wrd.u.string = buf;
    wrd.attrSet = att->parent->ordinal;
    wrd.attrUse = att->locals->local;
    (*p->add)(&wrd);
}

static int dumpkeys(data1_node *n, struct recExtractCtrl *p, int level)
{
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
	    data1_node *par = get_parent_tag(n);
	    data1_termlist *tlist = 0;

	    if (p->flagShowRecords)
	    {
		printf("%*s", level * 4, "");
		printf("Data: ");
		if (n->u.data.len > 20)
		    printf("'%.20s...'\n", n->u.data.data);
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
		if (!par->parent || !(par = get_parent_tag(par->parent)))
		    break;
	    if (!par)
		tlist = 0;
	    else if (par->u.tag.element->termlists)
		tlist = par->u.tag.element->termlists;
	    else
		continue;

	    for (; tlist; tlist = tlist->next)
	    {
		if (p->flagShowRecords)
		{
		    printf("%*sIdx: [", (level + 1) * 4, "");
		    switch (tlist->structure)
		    {
			case DATA1S_word: printf("w"); break;
			case DATA1S_phrase: printf("p"); break;
			default: printf("?"); break;
		    }
		    printf("] ");
		    printf("%s:%s [%d]\n", tlist->att->parent->name,
			tlist->att->name, tlist->att->value);
		}
		else switch (tlist->structure)
		{
		    case DATA1S_word:
			dumpkeys_word(n, p, tlist->att); break;
		    case DATA1S_phrase:
			dumpkeys_phrase(n, p, tlist->att); break;
		    default:
			logf(LOG_FATAL, "Bad structure type in dumpkeys");
			abort();
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
    NMEM mem = nmem_create();
    struct grs_read_info gri;
    seqno = 0;

    gri.readf = p->readf;
    gri.seekf = p->seekf;
    gri.tellf = p->tellf;
    gri.endf = p->endf;
    gri.fh = p->fh;
    gri.offset = p->offset;
    gri.mem = mem;

    n = read_grs_type (&gri, p->subType);
    if (!n)
        return -1;
    if (dumpkeys(n, p, 0) < 0)
    {
	data1_free_tree(n);
	return -2;
    }
    data1_free_tree(n);
    nmem_destroy(mem);
    return 0;
}

/*
 * Return: -1: Nothing done. 0: Ok. >0: Bib-1 diagnostic.
 */
static int process_comp(data1_node *n, Z_RecordComposition *c)
{
    data1_esetname *eset;
    Z_Espec1 *espec = 0;
    Z_ElementSpec *p;

    switch (c->which)
    {
	case Z_RecordComp_simple:
	    if (c->u.simple->which != Z_ElementSetNames_generic)
		return 26; /* only generic form supported. Fix this later */
	    if (!(eset = data1_getesetbyname(n->u.root.absyn,
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
			    if (!(eset = data1_getesetbyname(n->u.root.absyn,
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
	return data1_doespec1(n, espec);
    else
	return -1;
}

static int grs_retrieve(struct recRetrieveCtrl *p)
{
    data1_node *node = 0, *onode = 0;
    data1_node *new;
    data1_maptab *map;
    int res, selected = 0;
    NMEM mem = nmem_create();
    struct grs_read_info gri;
    
    gri.readf = p->readf;
    gri.seekf = p->seekf;
    gri.tellf = p->tellf;
    gri.endf = NULL;
    gri.fh = p->fh;
    gri.offset = 0;
    gri.mem = mem;

    node = read_grs_type (&gri, p->subType);
/* node = data1_read_record(p->readf, p->fh, mem); */
    if (!node)
    {
	p->diagnostic = 2;
	return 0;
    }
    if (p->score >= 0 && (new = data1_insert_taggeddata(node, node, "rank",
	mem)))
    {
	new->u.data.what = DATA1I_num;
	new->u.data.data = new->u.data.lbuf;
	sprintf(new->u.data.data, "%d", p->score);
	new->u.data.len = strlen(new->u.data.data);
    }
    if ((new = data1_insert_taggeddata(node, node, "localControlNumber", mem)))
    {
	new->u.data.what = DATA1I_text;
	new->u.data.data = new->u.data.lbuf;
	sprintf(new->u.data.data, "%d", p->localno);
	new->u.data.len = strlen(new->u.data.data);
    }
    if (p->input_format == VAL_GRS1 && node->u.root.absyn &&
	node->u.root.absyn->reference != VAL_NONE)
    {
	oident oe;
	Odr_oid *oid;

	oe.proto = PROTO_Z3950;
	oe.oclass = CLASS_SCHEMA;
	oe.value = node->u.root.absyn->reference;

	if ((oid = oid_getoidbyent(&oe)))
	{
	    char tmp[128];
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

	    if ((new = data1_insert_taggeddata(node, node, "schemaIdentifier",
		mem)))
	    {
		new->u.data.what = DATA1I_oid;
		new->u.data.data = nmem_malloc(mem, p - tmp);
		memcpy(new->u.data.data, tmp, p - tmp);
		new->u.data.len = p - tmp;
	    }
	}
    }

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
	    if (!(node = data1_map_record(onode, map, mem)))
	    {
		p->diagnostic = 14;
		return 0;
	    }

	    break;
	}

    if (p->comp && (res = process_comp(node, p->comp)) > 0)
    {
	p->diagnostic = res;
	if (onode)
	    data1_free_tree(onode);
	data1_free_tree(node);
	nmem_destroy(mem);
	return 0;
    }
    else if (p->comp && !res)
	selected = 1;

    switch (p->output_format = (p->input_format != VAL_NONE ?
	p->input_format : VAL_SUTRS))
    {
	data1_marctab *marctab;
        int dummy;

	case VAL_GRS1:
	    dummy = 0;
	    if (!(p->rec_buf = data1_nodetogr(node, selected, p->odr, &dummy)))
		p->diagnostic = 2; /* this should be better specified */
	    else
		p->rec_len = -1;
	    break;
	case VAL_EXPLAIN:
	    if (!(p->rec_buf = data1_nodetoexplain(node, selected, p->odr)))
		p->diagnostic = 2; /* this should be better specified */
	    else
		p->rec_len = -1;
	    break;
	case VAL_SUMMARY:
	    if (!(p->rec_buf = data1_nodetosummary(node, selected, p->odr)))
		p->diagnostic = 2;
	    else
		p->rec_len = -1;
	    break;
	case VAL_SUTRS:
	    if (!(p->rec_buf = data1_nodetobuf(node, selected,
		(int*)&p->rec_len)))
	    {
		p->diagnostic = 2;
		break;
	    }
	    break;
	case VAL_SOIF:
	    if (!(p->rec_buf = data1_nodetosoif(node, selected,
		(int*)&p->rec_len)))
	    {
		p->diagnostic = 2;
		break;
	    }
	    break;
	default:
	    for (marctab = node->u.root.absyn->marc; marctab;
		marctab = marctab->next)
		if (marctab->reference == p->input_format)
		    break;
	    if (!marctab)
	    {
		p->diagnostic = 227;
		break;
	    }
	    if (!(p->rec_buf = data1_nodetomarc(marctab, node, selected,
		(int*)&p->rec_len)))
	    {
		p->diagnostic = 2;
		break;
	    }
    }
    if (node)
	data1_free_tree(node);
    if (onode)
	data1_free_tree(onode);
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
