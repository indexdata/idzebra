/* $Id: xslt.c,v 1.3 2005-04-28 13:33:20 adam Exp $
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
#include <ctype.h>

#include <yaz/diagbib1.h>
#include <libxml/xmlreader.h>
#include <libxslt/transform.h>

#include <idzebra/util.h>
#include <idzebra/recctrl.h>

struct filter_info {
    xsltStylesheetPtr stylesheet_xsp;
    xmlTextReaderPtr reader;
    char *fname;
    int split_depth;
    ODR odr;
};

#define ZEBRA_INDEX_NS "http://indexdata.dk/zebra/indexing/1"
#define ZEBRA_SCHEMA_IDENTITY_NS "http://indexdata.dk/zebra/identity/1"
static const char *zebra_index_ns = ZEBRA_INDEX_NS;

static void set_param_str(const char **params, const char *name,
			  const char *value, ODR odr)
{
    char *quoted = odr_malloc(odr, 3 + strlen(value));
    sprintf(quoted, "'%s'", value);
    while (*params)
	params++;
    params[0] = name;
    params[1] = quoted;
    params[2] = 0;
}

static void set_param_int(const char **params, const char *name,
			  zint value, ODR odr)
{
    char *quoted = odr_malloc(odr, 30); /* 25 digits enough for 2^64 */
    while (*params)
	params++;
    sprintf(quoted, "'" ZINT_FORMAT "'", value);
    params[0] = name;
    params[1] = quoted;
    params[2] = 0;
}


static void *filter_init (Res res, RecType recType)
{
    struct filter_info *tinfo = (struct filter_info *) xmalloc(sizeof(*tinfo));
    tinfo->stylesheet_xsp = 0;
    tinfo->reader = 0;
    tinfo->fname = 0;
    tinfo->split_depth = 1;
    tinfo->odr = odr_createmem(ODR_ENCODE);
    return tinfo;
}

static void filter_config(void *clientData, Res res, const char *args)
{
    struct filter_info *tinfo = clientData;
    if (!args || !*args)
	args = "default.xsl";
    if (!tinfo->fname || strcmp(args, tinfo->fname))
    {
	/* different filename so must reread stylesheet */
	xfree(tinfo->fname);
	tinfo->fname = xstrdup(args);
	if (tinfo->stylesheet_xsp)
	    xsltFreeStylesheet(tinfo->stylesheet_xsp);
	tinfo->stylesheet_xsp =
	    xsltParseStylesheetFile((const xmlChar*) tinfo->fname);
    }
}

static void filter_destroy(void *clientData)
{
    struct filter_info *tinfo = clientData;
    if (tinfo->stylesheet_xsp)
	xsltFreeStylesheet(tinfo->stylesheet_xsp);
    xfree(tinfo->fname);
    odr_destroy(tinfo->odr);
    xfree(tinfo);
}

static int ioread_ex(void *context, char *buffer, int len)
{
    struct recExtractCtrl *p = context;
    return (*p->readf)(p->fh, buffer, len);
}

static int ioclose_ex(void *context)
{
    return 0;
}

static void index_field(struct filter_info *tinfo, struct recExtractCtrl *ctrl,
			xmlNodePtr ptr,	RecWord *recWord)
{
    for(; ptr; ptr = ptr->next)
    {
	index_field(tinfo, ctrl, ptr->children, recWord);
	if (ptr->type != XML_TEXT_NODE)
	    continue;
	recWord->term_buf = ptr->content;
	recWord->term_len = strlen(ptr->content);
	(*ctrl->tokenAdd)(recWord);
    }
}

static void index_node(struct filter_info *tinfo,  struct recExtractCtrl *ctrl,
		       xmlNodePtr ptr, RecWord *recWord)
{
    for(; ptr; ptr = ptr->next)
    {
	index_node(tinfo, ctrl, ptr->children, recWord);
	if (ptr->type != XML_ELEMENT_NODE || !ptr->ns ||
	    strcmp(ptr->ns->href, zebra_index_ns))
	    continue;
	if (!strcmp(ptr->name, "index"))
	{
	    char *field_str = 0;
	    const char *xpath_str = 0;
	    struct _xmlAttr *attr;
	    for (attr = ptr->properties; attr; attr = attr->next)
	    {
		if (!strcmp(attr->name, "field") 
		    && attr->children && attr->children->type == XML_TEXT_NODE)
		    field_str = attr->children->content;
		if (!strcmp(attr->name, "xpath") 
		    && attr->children && attr->children->type == XML_TEXT_NODE)
		    xpath_str = attr->children->content;
	    }
	    if (field_str)
	    {
		recWord->attrStr = field_str;
		index_field(tinfo, ctrl, ptr->children, recWord);
	    }
	}
    }
}

static int filter_extract(void *clientData, struct recExtractCtrl *p)
{
    const char *params[10];
    struct filter_info *tinfo = clientData;
    RecWord recWord;
    int ret;

    params[0] = 0;

    odr_reset(tinfo->odr);
    set_param_str(params, "schema", ZEBRA_INDEX_NS, tinfo->odr);

    if (p->first_record)
    {
	if (tinfo->reader)
	    xmlFreeTextReader(tinfo->reader);
	tinfo->reader = xmlReaderForIO(ioread_ex, ioclose_ex,
				       p /* I/O handler */,
				       0 /* URL */, 
				       0 /* encoding */,
				       XML_PARSE_XINCLUDE);
    }
    if (!tinfo->reader)
	return RECCTRL_EXTRACT_ERROR_GENERIC;

    if (!tinfo->stylesheet_xsp)
	return RECCTRL_EXTRACT_ERROR_GENERIC;

    (*p->init)(p, &recWord);
    recWord.reg_type = 'w';

    ret = xmlTextReaderRead(tinfo->reader);
    while (ret == 1) {
	int type = xmlTextReaderNodeType(tinfo->reader);
	int depth = xmlTextReaderDepth(tinfo->reader);
	if (tinfo->split_depth == 0 ||
	    (type == XML_READER_TYPE_ELEMENT && tinfo->split_depth == depth))
	{
	    xmlChar *buf_out;
	    int len_out;

	    xmlNodePtr ptr = xmlTextReaderExpand(tinfo->reader);
	    xmlNodePtr ptr2 = xmlCopyNode(ptr, 1);
	    xmlDocPtr doc = xmlNewDoc("1.0");

	    xmlDocSetRootElement(doc, ptr2);
	    
	    if (tinfo->stylesheet_xsp)
	    {
		xmlDocPtr resDoc = 
		    xsltApplyStylesheet(tinfo->stylesheet_xsp,
					doc, params);
		if (p->flagShowRecords)
		{
		    xmlDocDumpMemory(resDoc, &buf_out, &len_out);
		    fwrite(buf_out, len_out, 1, stdout);
		    xmlFree(buf_out);
		}
		index_node(tinfo, p, xmlDocGetRootElement(resDoc), &recWord);
		xmlFreeDoc(resDoc);
	    }
	    xmlDocDumpMemory(doc, &buf_out, &len_out);
	    if (p->flagShowRecords)
		fwrite(buf_out, len_out, 1, stdout);
	    (*p->setStoreData)(p, buf_out, len_out);
	    xmlFree(buf_out);

	    xmlFreeDoc(doc);
	    return RECCTRL_EXTRACT_OK;
	}
	ret = xmlTextReaderRead(tinfo->reader);
    }
    xmlFreeTextReader(tinfo->reader);
    tinfo->reader = 0;
    return RECCTRL_EXTRACT_EOF;
}

static int ioread_ret(void *context, char *buffer, int len)
{
    struct recRetrieveCtrl *p = context;
    return (*p->readf)(p->fh, buffer, len);
}

static int ioclose_ret(void *context)
{
    return 0;
}

static int filter_retrieve (void *clientData, struct recRetrieveCtrl *p)
{
    const char *esn = ZEBRA_SCHEMA_IDENTITY_NS;
    const char *params[10];
    struct filter_info *tinfo = clientData;
    xmlDocPtr resDoc;
    xmlDocPtr doc;

    if (p->comp)
    {
	if (p->comp->which != Z_RecordComp_simple
	    || p->comp->u.simple->which != Z_ElementSetNames_generic)
	{
	    p->diagnostic = YAZ_BIB1_PRESENT_COMP_SPEC_PARAMETER_UNSUPP;
	    return 0;
	}
	esn = p->comp->u.simple->u.generic;
    }
    
    params[0] = 0;
    set_param_str(params, "schema", esn, p->odr);
    if (p->fname)
	set_param_str(params, "filename", p->fname, p->odr);
    if (p->score >= 0)
	set_param_int(params, "score", p->score, p->odr);
    set_param_int(params, "size", p->recordSize, p->odr);
    
    if (!tinfo->stylesheet_xsp)
    {
	p->diagnostic = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
	return 0;
    }
    doc = xmlReadIO(ioread_ret, ioclose_ret, p /* I/O handler */,
		    0 /* URL */,
		    0 /* encoding */,
		    XML_PARSE_XINCLUDE);
    if (!doc)
    {
	p->diagnostic = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
	return 0;
    }

    if (!strcmp(esn, ZEBRA_SCHEMA_IDENTITY_NS))
	resDoc = doc;
    else
    {
	resDoc = xsltApplyStylesheet(tinfo->stylesheet_xsp,
				     doc, params);
	xmlFreeDoc(doc);
    }
    if (!resDoc)
    {
	p->diagnostic = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }
    else if (p->input_format == VAL_NONE || p->input_format == VAL_TEXT_XML)
    {
	xmlChar *buf_out;
	int len_out;
	xmlDocDumpMemory(resDoc, &buf_out, &len_out);

	p->output_format = VAL_TEXT_XML;
	p->rec_len = len_out;
	p->rec_buf = odr_malloc(p->odr, p->rec_len);
	memcpy(p->rec_buf, buf_out, p->rec_len);
	
	xmlFree(buf_out);
    }
    else if (p->output_format == VAL_SUTRS)
    {
	xmlChar *buf_out;
	int len_out;
	xmlDocDumpMemory(resDoc, &buf_out, &len_out);

	p->output_format = VAL_SUTRS;
	p->rec_len = len_out;
	p->rec_buf = odr_malloc(p->odr, p->rec_len);
	memcpy(p->rec_buf, buf_out, p->rec_len);
	
	xmlFree(buf_out);
    }
    else
    {
	p->diagnostic = YAZ_BIB1_RECORD_SYNTAX_UNSUPP;
    }
    xmlFreeDoc(resDoc);
    return 0;
}

static struct recType filter_type = {
    0,
    "xslt",
    filter_init,
    filter_config,
    filter_destroy,
    filter_extract,
    filter_retrieve
};

RecType
#ifdef IDZEBRA_STATIC_XSLT
idzebra_filter_xslt
#else
idzebra_filter
#endif

[] = {
    &filter_type,
    0,
};
