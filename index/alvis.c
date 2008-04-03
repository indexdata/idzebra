/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <yaz/diagbib1.h>
#include <yaz/tpath.h>
#include <yaz/oid_db.h>

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlreader.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#if YAZ_HAVE_EXSLT
#include <libexslt/exslt.h>
#endif

#include <idzebra/util.h>
#include <idzebra/recctrl.h>

struct filter_schema {
    const char *name;
    const char *identifier;
    const char *stylesheet;
    struct filter_schema *next;
    const char *default_schema;
    /* char default_schema; */
    xsltStylesheetPtr stylesheet_xsp;
};

struct filter_info {
    xmlDocPtr doc;
    char *fname;
    char *full_name;
    const char *profile_path;
    int split_level;
    const char *split_path;
    ODR odr;
    struct filter_schema *schemas;
    xmlTextReaderPtr reader;
};

#define ZEBRA_SCHEMA_XSLT_NS "http://indexdata.dk/zebra/xslt/1"

#define XML_STRCMP(a,b)   strcmp((char*)a, b)
#define XML_STRLEN(a) strlen((char*)a)

static const char *zebra_xslt_ns = ZEBRA_SCHEMA_XSLT_NS;

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

#define ENABLE_INPUT_CALLBACK 0

#if ENABLE_INPUT_CALLBACK
static int zebra_xmlInputMatchCallback (char const *filename)
{
    yaz_log(YLOG_LOG, "match %s", filename);
    return 0;
}

static void * zebra_xmlInputOpenCallback (char const *filename)
{
    return 0;
}

static int zebra_xmlInputReadCallback (void * context, char * buffer, int len)
{
    return 0;
}

static int zebra_xmlInputCloseCallback (void * context)
{
    return 0;
}
#endif

static void *filter_init(Res res, RecType recType)
{
    struct filter_info *tinfo = (struct filter_info *) xmalloc(sizeof(*tinfo));
    tinfo->reader = 0;
    tinfo->fname = 0;
    tinfo->full_name = 0;
    tinfo->profile_path = 0;
    tinfo->split_level = 0;
    tinfo->split_path = 0;
    tinfo->odr = odr_createmem(ODR_ENCODE);
    tinfo->doc = 0;
    tinfo->schemas = 0;

#if YAZ_HAVE_EXSLT
    exsltRegisterAll(); 
#endif

#if ENABLE_INPUT_CALLBACK
    xmlRegisterDefaultInputCallbacks();
    xmlRegisterInputCallbacks(zebra_xmlInputMatchCallback,
			      zebra_xmlInputOpenCallback,
			      zebra_xmlInputReadCallback,
			      zebra_xmlInputCloseCallback);
#endif
    return tinfo;
}

static int attr_content(struct _xmlAttr *attr, const char *name,
			const char **dst_content)
{
    if (!XML_STRCMP(attr->name, name) && attr->children 
        && attr->children->type == XML_TEXT_NODE)
    {
	*dst_content = (const char *)(attr->children->content);
	return 1;
    }
    return 0;
}

static void destroy_schemas(struct filter_info *tinfo)
{
    struct filter_schema *schema = tinfo->schemas;
    while (schema)
    {
	struct filter_schema *schema_next = schema->next;
	if (schema->stylesheet_xsp)
	    xsltFreeStylesheet(schema->stylesheet_xsp);
	xfree(schema);
	schema = schema_next;
    }
    tinfo->schemas = 0;
    xfree(tinfo->fname);
    if (tinfo->doc)
	xmlFreeDoc(tinfo->doc);    
    tinfo->doc = 0;
}

static ZEBRA_RES create_schemas(struct filter_info *tinfo, const char *fname)
{
    char tmp_full_name[1024];
    xmlNodePtr ptr;
    tinfo->fname = xstrdup(fname);
    
    if (yaz_filepath_resolve(tinfo->fname, tinfo->profile_path, 
                             NULL, tmp_full_name))
        tinfo->full_name = xstrdup(tmp_full_name);
    else
        tinfo->full_name = xstrdup(tinfo->fname);
    
    yaz_log(YLOG_LOG, "alvis filter: loading config file %s", tinfo->full_name);
    
    tinfo->doc = xmlParseFile(tinfo->full_name);
    
    if (!tinfo->doc)
    {
        yaz_log(YLOG_WARN, "alvis filter: could not parse config file %s", 
                tinfo->full_name);
        
	return ZEBRA_FAIL;
    }
    
    ptr = xmlDocGetRootElement(tinfo->doc);
    if (!ptr || ptr->type != XML_ELEMENT_NODE 
        || XML_STRCMP(ptr->name, "schemaInfo"))
    {
        yaz_log(YLOG_WARN, 
                "alvis filter:  config file %s :" 
                " expected root element <schemaInfo>", 
                tinfo->full_name);  
        return ZEBRA_FAIL;
    }

    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
	if (ptr->type != XML_ELEMENT_NODE)
	    continue;
	if (!XML_STRCMP(ptr->name, "schema"))
	{  
	    struct _xmlAttr *attr;
	    struct filter_schema *schema = xmalloc(sizeof(*schema));
	    schema->name = 0;
	    schema->identifier = 0;
	    schema->stylesheet = 0;
	    schema->default_schema = 0;
	    schema->next = tinfo->schemas;
	    schema->stylesheet_xsp = 0;
	    tinfo->schemas = schema;
	    for (attr = ptr->properties; attr; attr = attr->next)
	    {
		attr_content(attr, "identifier", &schema->identifier);
		attr_content(attr, "name", &schema->name);
		attr_content(attr, "stylesheet", &schema->stylesheet);
		attr_content(attr, "default", &schema->default_schema);
	    }
            /*yaz_log(YLOG_LOG, "XSLT add %s %s %s", 
              schema->name, schema->identifier, schema->stylesheet); */

            /* find requested schema */

	    if (schema->stylesheet)
            {
                char tmp_xslt_full_name[1024];
                if (!yaz_filepath_resolve(schema->stylesheet, tinfo->profile_path, 
                                          NULL, tmp_xslt_full_name)) 
                {
                    yaz_log(YLOG_WARN, 
                            "alvis filter: stylesheet %s not found in path %s",
                            schema->stylesheet, tinfo->profile_path);
                    return ZEBRA_FAIL;
                }
                schema->stylesheet_xsp 
                    = xsltParseStylesheetFile((const xmlChar*) tmp_xslt_full_name);
                if (!schema->stylesheet_xsp)
                {
                    yaz_log(YLOG_WARN, 
                            "alvis filter: could not parse xslt stylesheet %s", 
                            tmp_xslt_full_name);
                    return ZEBRA_FAIL;
                }
            }
	}
	else if (!XML_STRCMP(ptr->name, "split"))
	{
	    struct _xmlAttr *attr;
	    for (attr = ptr->properties; attr; attr = attr->next)
	    {
                const char *split_level_str = 0;
		attr_content(attr, "level", &split_level_str);
                tinfo->split_level = 
                    split_level_str ? atoi(split_level_str) : 0;
	    }
	}
	else
	{
	    yaz_log(YLOG_WARN, "Bad element %s in %s", ptr->name, fname);
	    return ZEBRA_FAIL;
	}
    }
    return ZEBRA_OK;
}

static struct filter_schema *lookup_schema(struct filter_info *tinfo,
					   const char *est)
{
    struct filter_schema *schema;

    for (schema = tinfo->schemas; schema; schema = schema->next)
    { 
        /* find requested schema */
	if (est) 
	{    
	    if (schema->identifier && !strcmp(schema->identifier, est))
                return schema;
            
	    if (schema->name && !strcmp(schema->name, est))
		return schema;
	} 
        /* or return default schema if defined */
        else if (schema->default_schema)
	    return schema;
    }

    /* return first schema if no default schema defined */
    if (tinfo->schemas)
        return tinfo->schemas;
    
    return 0;
}

static ZEBRA_RES filter_config(void *clientData, Res res, const char *args)
{
    struct filter_info *tinfo = clientData;
    if (!args || !*args)
    {
        yaz_log(YLOG_WARN, "alvis filter: need config file");
        return ZEBRA_FAIL;
    }

    if (tinfo->fname && !strcmp(args, tinfo->fname))
	return ZEBRA_OK;
    
    tinfo->profile_path = res_get(res, "profilePath");
    yaz_log(YLOG_LOG, "alvis filter: profilePath %s", tinfo->profile_path);

    destroy_schemas(tinfo);
    return create_schemas(tinfo, args);
}

static void filter_destroy(void *clientData)
{
    struct filter_info *tinfo = clientData;
    destroy_schemas(tinfo);
    xfree(tinfo->full_name);
    if (tinfo->reader)
	xmlFreeTextReader(tinfo->reader);
    odr_destroy(tinfo->odr);
    xfree(tinfo);
}

static int ioread_ex(void *context, char *buffer, int len)
{
    struct recExtractCtrl *p = context;
    return p->stream->readf(p->stream, buffer, len);
}

static int ioclose_ex(void *context)
{
    return 0;
}

static void index_cdata(struct filter_info *tinfo, struct recExtractCtrl *ctrl,
			xmlNodePtr ptr,	RecWord *recWord)
{
    for(; ptr; ptr = ptr->next)
    {
	index_cdata(tinfo, ctrl, ptr->children, recWord);
	if (ptr->type != XML_TEXT_NODE)
	    continue;
	recWord->term_buf = (const char *)ptr->content;
	recWord->term_len = XML_STRLEN(ptr->content);
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
	    XML_STRCMP(ptr->ns->href, zebra_xslt_ns))
	    continue;
	if (!XML_STRCMP(ptr->name, "index"))
	{
	    const char *name_str = 0;
	    const char *type_str = 0;
	    const char *xpath_str = 0;
	    struct _xmlAttr *attr;
	    for (attr = ptr->properties; attr; attr = attr->next)
	    {
		attr_content(attr, "name", &name_str);
		attr_content(attr, "xpath", &xpath_str);
		attr_content(attr, "type", &type_str);
	    }
	    if (name_str)
	    {
		const char *prev_type = recWord->index_type; /* save default type */

		if (type_str && *type_str)
		    recWord->index_type = (const char *) type_str; /* type was given */
		recWord->index_name = name_str;
		index_cdata(tinfo, ctrl, ptr->children, recWord);

		recWord->index_type = prev_type;     /* restore it again */
	    }
	}
    }
}

static void index_record(struct filter_info *tinfo,struct recExtractCtrl *ctrl,
			 xmlNodePtr ptr, RecWord *recWord)
{
    const char *type_str = "update";

    if (ptr && ptr->type == XML_ELEMENT_NODE && ptr->ns &&
	!XML_STRCMP(ptr->ns->href, zebra_xslt_ns)
	&& !XML_STRCMP(ptr->name, "record"))
    {
	const char *id_str = 0;
	const char *rank_str = 0;
	struct _xmlAttr *attr;
	for (attr = ptr->properties; attr; attr = attr->next)
	{
	    attr_content(attr, "type", &type_str);
	    attr_content(attr, "id", &id_str);
	    attr_content(attr, "rank", &rank_str);
	}
	if (id_str)
	    sscanf(id_str, "%255s", ctrl->match_criteria);

	if (rank_str)
	    ctrl->staticrank = atozint(rank_str);
	ptr = ptr->children;
    }

    if (!strcmp("update", type_str))
        index_node(tinfo, ctrl, ptr, recWord);
    else if (!strcmp("delete", type_str))
         yaz_log(YLOG_WARN, "alvis filter delete: to be implemented");
    else
         yaz_log(YLOG_WARN, "alvis filter: unknown record type '%s'", 
                 type_str);
}
    
static int extract_doc(struct filter_info *tinfo, struct recExtractCtrl *p,
		       xmlDocPtr doc)
{
    RecWord recWord;
    const char *params[10];
    xmlChar *buf_out;
    int len_out;

    struct filter_schema *schema = lookup_schema(tinfo, zebra_xslt_ns);

    params[0] = 0;
    set_param_str(params, "schema", zebra_xslt_ns, tinfo->odr);

    (*p->init)(p, &recWord);

    if (schema && schema->stylesheet_xsp)
    {
	xmlNodePtr root_ptr;
	xmlDocPtr resDoc = 
	    xsltApplyStylesheet(schema->stylesheet_xsp,
				doc, params);
	if (p->flagShowRecords)
	{
	    xmlDocDumpMemory(resDoc, &buf_out, &len_out);
	    fwrite(buf_out, len_out, 1, stdout);
	    xmlFree(buf_out);
	}
	root_ptr = xmlDocGetRootElement(resDoc);
	if (root_ptr)
	    index_record(tinfo, p, root_ptr, &recWord);
	else
	{
	    yaz_log(YLOG_WARN, "No root for index XML record."
		    " split_level=%d stylesheet=%s",
		    tinfo->split_level, schema->stylesheet);
	}
	xmlFreeDoc(resDoc);
    }
    xmlDocDumpMemory(doc, &buf_out, &len_out);
    if (p->flagShowRecords)
	fwrite(buf_out, len_out, 1, stdout);
    if (p->setStoreData)
        (*p->setStoreData)(p, buf_out, len_out);
    xmlFree(buf_out);
    
    xmlFreeDoc(doc);
    return RECCTRL_EXTRACT_OK;
}

static int extract_split(struct filter_info *tinfo, struct recExtractCtrl *p)
{
    int ret;

    if (p->first_record)
    {
	if (tinfo->reader)
	    xmlFreeTextReader(tinfo->reader);
	tinfo->reader = xmlReaderForIO(ioread_ex, ioclose_ex,
				       p /* I/O handler */,
				       0 /* URL */, 
				       0 /* encoding */,
				       XML_PARSE_XINCLUDE
                                       | XML_PARSE_NOENT
                                       | XML_PARSE_NONET);
    }
    if (!tinfo->reader)
	return RECCTRL_EXTRACT_ERROR_GENERIC;

    ret = xmlTextReaderRead(tinfo->reader);
    while (ret == 1)
    {
	int type = xmlTextReaderNodeType(tinfo->reader);
	int depth = xmlTextReaderDepth(tinfo->reader);
	if (type == XML_READER_TYPE_ELEMENT && tinfo->split_level == depth)
	{
	    xmlNodePtr ptr = xmlTextReaderExpand(tinfo->reader);
            if (ptr)
            {
                xmlNodePtr ptr2 = xmlCopyNode(ptr, 1);
                xmlDocPtr doc = xmlNewDoc((const xmlChar*) "1.0");
                
                xmlDocSetRootElement(doc, ptr2);
                
                return extract_doc(tinfo, p, doc);
            }
            else
            {
                xmlFreeTextReader(tinfo->reader);
                tinfo->reader = 0;
                return RECCTRL_EXTRACT_ERROR_GENERIC;
            }
	}
	ret = xmlTextReaderRead(tinfo->reader);
    }
    xmlFreeTextReader(tinfo->reader);
    tinfo->reader = 0;
    return RECCTRL_EXTRACT_EOF;
}

static int extract_full(struct filter_info *tinfo, struct recExtractCtrl *p)
{
    if (p->first_record) /* only one record per stream */
    {
       xmlDocPtr doc = xmlReadIO(ioread_ex, ioclose_ex, p /* I/O handler */,
                                 0 /* URL */,
                                 0 /* encoding */,
                                 XML_PARSE_XINCLUDE
                                 | XML_PARSE_NOENT
                                 | XML_PARSE_NONET);
       if (!doc)
           return RECCTRL_EXTRACT_ERROR_GENERIC;
       /* else {
           xmlNodePtr root = xmlDocGetRootElement(doc);
            if (!root)
                return RECCTRL_EXTRACT_ERROR_GENERIC;
                } */
       
       return extract_doc(tinfo, p, doc);
    }
    else
       return RECCTRL_EXTRACT_EOF;
}

static int filter_extract(void *clientData, struct recExtractCtrl *p)
{
    struct filter_info *tinfo = clientData;

    odr_reset(tinfo->odr);
    if (tinfo->split_level == 0 || p->setStoreData == 0)
        return extract_full(tinfo, p);
    else
        return extract_split(tinfo, p);
}

static int ioread_ret(void *context, char *buffer, int len)
{
    struct recRetrieveCtrl *p = context;
    return p->stream->readf(p->stream, buffer, len);
}

static int ioclose_ret(void *context)
{
    return 0;
}

static int filter_retrieve (void *clientData, struct recRetrieveCtrl *p)
{
    /* const char *esn = zebra_xslt_ns; */
    const char *esn = 0;
    const char *params[32];
    struct filter_info *tinfo = clientData;
    xmlDocPtr resDoc;
    xmlDocPtr doc;
    struct filter_schema *schema;

    if (p->comp)
    {
	if (p->comp->which == Z_RecordComp_simple
	    && p->comp->u.simple->which == Z_ElementSetNames_generic)
	{
	    esn = p->comp->u.simple->u.generic;
	}
	else if (p->comp->which == Z_RecordComp_complex 
		 && p->comp->u.complex->generic->elementSpec
		 && p->comp->u.complex->generic->elementSpec->which ==
		 Z_ElementSpec_elementSetName)
	{
	    esn = p->comp->u.complex->generic->elementSpec->u.elementSetName;
	}
    }
    schema = lookup_schema(tinfo, esn);
    if (!schema)
    {
	p->diagnostic =
	    YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
	return 0;
    }

    params[0] = 0;
    set_param_int(params, "id", p->localno, p->odr);
    if (p->fname)
	set_param_str(params, "filename", p->fname, p->odr);
    if (p->staticrank >= 0)
	set_param_int(params, "rank", p->staticrank, p->odr);

    if (esn)
        set_param_str(params, "schema", esn, p->odr);
    else
        if (schema->name)
            set_param_str(params, "schema", schema->name, p->odr);
        else if (schema->identifier)
            set_param_str(params, "schema", schema->identifier, p->odr);
        else
            set_param_str(params, "schema", "", p->odr);

    if (p->score >= 0)
	set_param_int(params, "score", p->score, p->odr);
    set_param_int(params, "size", p->recordSize, p->odr);

    doc = xmlReadIO(ioread_ret, ioclose_ret, p /* I/O handler */,
		    0 /* URL */,
		    0 /* encoding */,
		    XML_PARSE_XINCLUDE | XML_PARSE_NOENT | XML_PARSE_NONET);
    if (!doc)
    {
	p->diagnostic = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
	return 0;
    }

    if (!schema->stylesheet_xsp)
	resDoc = doc;
    else
    {
	resDoc = xsltApplyStylesheet(schema->stylesheet_xsp,
				     doc, params);
	xmlFreeDoc(doc);
    }
    if (!resDoc)
    {
	p->diagnostic = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }
    else if (!p->input_format 
             || !oid_oidcmp(p->input_format, yaz_oid_recsyn_xml))
    {
	xmlChar *buf_out;
	int len_out;

        if (schema->stylesheet_xsp)
            xsltSaveResultToString(&buf_out, &len_out, resDoc,
                                   schema->stylesheet_xsp);	
        else
	    xmlDocDumpMemory(resDoc, &buf_out, &len_out);            

	p->output_format = yaz_oid_recsyn_xml;
	p->rec_len = len_out;
	p->rec_buf = odr_malloc(p->odr, p->rec_len);
	memcpy(p->rec_buf, buf_out, p->rec_len);
	xmlFree(buf_out);
    }
    else if (!oid_oidcmp(p->output_format, yaz_oid_recsyn_sutrs))
    {
	xmlChar *buf_out;
	int len_out;

        if (schema->stylesheet_xsp)
            xsltSaveResultToString(&buf_out, &len_out, resDoc,
                                   schema->stylesheet_xsp);
        else
	    xmlDocDumpMemory(resDoc, &buf_out, &len_out);            

	p->output_format = yaz_oid_recsyn_sutrs;
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
    "alvis",
    filter_init,
    filter_config,
    filter_destroy,
    filter_extract,
    filter_retrieve
};

RecType
#ifdef IDZEBRA_STATIC_ALVIS
idzebra_filter_alvis
#else
idzebra_filter
#endif

[] = {
    &filter_type,
    0,
};
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

