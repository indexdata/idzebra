/* $Id: xslt.c,v 1.1 2005-04-28 08:20:40 adam Exp $
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
};

static const char *zebra_index_ns = "http://indexdata.dk/zebra/indexing/1";

static void *filter_init (Res res, RecType recType)
{
    struct filter_info *tinfo = (struct filter_info *) xmalloc(sizeof(*tinfo));
    tinfo->stylesheet_xsp = 0;
    tinfo->reader = 0;
    tinfo->fname = 0;
    tinfo->split_depth = 1;
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
    static const char *params[] = {
	"schema", "'http://indexdata.dk/zebra/indexing/1'",
	0
    };
    struct filter_info *tinfo = clientData;
    RecWord recWord;
    int ret;

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
    static const char *params[] = {
	"schema", "'F'",
	0
    };
    struct filter_info *tinfo = clientData;
    xmlDocPtr resDoc;
    xmlDocPtr doc;

    if (p->comp)
    {
	const char *esn;
	char *esn_quoted;
	if (p->comp->which != Z_RecordComp_simple
	    || p->comp->u.simple->which != Z_ElementSetNames_generic)
	{
	    p->diagnostic = YAZ_BIB1_PRESENT_COMP_SPEC_PARAMETER_UNSUPP;
	    return 0;
	}
	esn = p->comp->u.simple->u.generic;
	esn_quoted = odr_malloc(p->odr, 3 + strlen(esn));
	sprintf(esn_quoted, "'%s'", esn);
	params[1] = esn_quoted;
    }
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
    resDoc = xsltApplyStylesheet(tinfo->stylesheet_xsp,
				 doc, params);
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
    xmlFreeDoc(doc);
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
