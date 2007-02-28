/* $Id: mod_dom.c,v 1.22 2007-02-28 13:16:24 marc Exp $
   Copyright (C) 1995-2007
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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

#include <yaz/diagbib1.h>
#include <yaz/tpath.h>
#include <yaz/snprintf.h>

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

/* DOM filter style indexing */
#define ZEBRA_DOM_NS "http://indexdata.com/zebra-2.0"
static const char *zebra_dom_ns = ZEBRA_DOM_NS;

/* DOM filter style indexing */
#define ZEBRA_PI_NAME "zebra-2.0"
static const char *zebra_pi_name = ZEBRA_PI_NAME;



struct convert_s {
    const char *stylesheet;
    xsltStylesheetPtr stylesheet_xsp;
    struct convert_s *next;
};

struct filter_extract {
    const char *name;
    struct convert_s *convert;
};

struct filter_store {
    struct convert_s *convert;
};

struct filter_retrieve {
    const char *name;
    const char *identifier;
    struct convert_s *convert;
    struct filter_retrieve *next;
};

#define DOM_INPUT_XMLREADER 1
#define DOM_INPUT_MARC 2
struct filter_input {
    const char *syntax;
    const char *name;
    struct convert_s *convert;
    int type;
    union {
        struct {
            const char *input_charset;
            yaz_marc_t handle;
            yaz_iconv_t iconv;
        } marc;
        struct {
            xmlTextReaderPtr reader;
            int split_level;
        } xmlreader;
    } u;
    struct filter_input *next;
};
  
struct filter_info {
    char *fname;
    char *full_name;
    const char *profile_path;
    ODR odr_record;
    ODR odr_config;
    xmlDocPtr doc_config;
    struct filter_extract *extract;
    struct filter_retrieve *retrieve_list;
    struct filter_input *input_list;
    struct filter_store *store;
};



#define XML_STRCMP(a,b)   strcmp((char*)a, b)
#define XML_STRLEN(a) strlen((char*)a)


#define FOR_EACH_ELEMENT(ptr) for (; ptr; ptr = ptr->next) if (ptr->type == XML_ELEMENT_NODE)

static void dom_log(int level, struct filter_info *tinfo, xmlNodePtr ptr,
                    const char *fmt, ...)
#ifdef __GNUC__
    __attribute__ ((format (printf, 4, 5)))
#endif
    ;

static void dom_log(int level, struct filter_info *tinfo, xmlNodePtr ptr,
                    const char *fmt, ...)
{
    va_list ap;
    char buf[4096];

    va_start(ap, fmt);
    yaz_vsnprintf(buf, sizeof(buf)-1, fmt, ap);
    if (ptr)
    {
        yaz_log(level, "%s:%ld: %s", tinfo->fname ? tinfo->fname : "none", 
                xmlGetLineNo(ptr), buf);
    }
    else
    {
        yaz_log(level, "%s: %s", tinfo->fname ? tinfo->fname : "none", buf);
    }
    va_end(ap);
}


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

static void *filter_init(Res res, RecType recType)
{
    struct filter_info *tinfo = (struct filter_info *) xmalloc(sizeof(*tinfo));
    tinfo->fname = 0;
    tinfo->full_name = 0;
    tinfo->profile_path = 0;
    tinfo->odr_record = odr_createmem(ODR_ENCODE);
    tinfo->odr_config = odr_createmem(ODR_ENCODE);
    tinfo->extract = 0;
    tinfo->retrieve_list = 0;
    tinfo->input_list = 0;
    tinfo->store = 0;
    tinfo->doc_config = 0;

#if YAZ_HAVE_EXSLT
    exsltRegisterAll(); 
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

static void destroy_xsp(struct convert_s *c)
{
    while(c)
    {
        if (c->stylesheet_xsp)
            xsltFreeStylesheet(c->stylesheet_xsp);
        c = c->next;
    }
}

static void destroy_dom(struct filter_info *tinfo)
{
    if (tinfo->extract)
    {
        destroy_xsp(tinfo->extract->convert);
        tinfo->extract = 0;
    }
    if (tinfo->store)
    {
        destroy_xsp(tinfo->store->convert);
        tinfo->store = 0;
    }
    if (tinfo->input_list)
    {
        struct filter_input *i_ptr;
        for (i_ptr = tinfo->input_list; i_ptr; i_ptr = i_ptr->next)
        {
            switch(i_ptr->type)
            {
            case DOM_INPUT_XMLREADER:
                if (i_ptr->u.xmlreader.reader)
                    xmlFreeTextReader(i_ptr->u.xmlreader.reader);
                break;
            case DOM_INPUT_MARC:
                yaz_iconv_close(i_ptr->u.marc.iconv);
                yaz_marc_destroy(i_ptr->u.marc.handle);
                break;
            }
            destroy_xsp(i_ptr->convert);
        }
        tinfo->input_list = 0;
    }
    if (tinfo->retrieve_list)
    {
        struct filter_retrieve *r_ptr;
        for (r_ptr = tinfo->retrieve_list; r_ptr; r_ptr = r_ptr->next)
            destroy_xsp(r_ptr->convert);
        tinfo->retrieve_list = 0;
    }

    if (tinfo->doc_config)
    {
        xmlFreeDoc(tinfo->doc_config);
        tinfo->doc_config = 0;
    }
    odr_reset(tinfo->odr_config);
}

static ZEBRA_RES parse_convert(struct filter_info *tinfo, xmlNodePtr ptr,
                               struct convert_s **l)
{
    *l = 0;
    FOR_EACH_ELEMENT(ptr) {
        if (!XML_STRCMP(ptr->name, "xslt"))
        {
            struct _xmlAttr *attr;
            struct convert_s *p 
                = odr_malloc(tinfo->odr_config, sizeof(*p));
            
            p->next = 0;
            p->stylesheet = 0;
            p->stylesheet_xsp = 0;
            
            for (attr = ptr->properties; attr; attr = attr->next)
                if (attr_content(attr, "stylesheet", &p->stylesheet))
                    ;
                else
                {
                    dom_log(YLOG_WARN, tinfo, ptr,
                            "bad attribute @%s", attr->name);
                }
            if (p->stylesheet)
            {
                char tmp_xslt_full_name[1024];
                if (!yaz_filepath_resolve(p->stylesheet, 
                                          tinfo->profile_path,
                                          NULL, 
                                          tmp_xslt_full_name))
                {
                    dom_log(YLOG_WARN, tinfo, 0,
                            "stylesheet %s not found in "
                            "path %s",
                            p->stylesheet, 
                            tinfo->profile_path);
                    return ZEBRA_FAIL;
                }
                
                p->stylesheet_xsp
                    = xsltParseStylesheetFile((const xmlChar*) 
                                              tmp_xslt_full_name);
                if (!p->stylesheet_xsp)
                {
                    dom_log(YLOG_WARN, tinfo, 0,
                            "could not parse xslt stylesheet %s",
                            tmp_xslt_full_name);
                    return ZEBRA_FAIL;
                }
                }
                else
                {
                    dom_log(YLOG_WARN, tinfo, ptr,
                            "missing attribute 'stylesheet' ");
                    return ZEBRA_FAIL;
                }
                *l = p;
                l = &p->next;
        }
        else
        {
            dom_log(YLOG_WARN, tinfo, ptr,
                    "bad element '%s', expected <xslt>", ptr->name);
            return ZEBRA_FAIL;
        }
    }
    return ZEBRA_OK;
}

static ZEBRA_RES perform_convert(struct filter_info *tinfo, 
                                 struct convert_s *convert,
                                 const char **params,
                                 xmlDocPtr *doc,
                                 xsltStylesheetPtr *last_xsp)
{
    for (; convert; convert = convert->next)
    {
        xmlDocPtr res_doc = xsltApplyStylesheet(convert->stylesheet_xsp,
                                                *doc, params);
        if (last_xsp)
            *last_xsp = convert->stylesheet_xsp;
        xmlFreeDoc(*doc);
        *doc = res_doc;
    }
    return ZEBRA_OK;
}

static struct filter_input *new_input(struct filter_info *tinfo, int type)
{
    struct filter_input *p;
    struct filter_input **np = &tinfo->input_list;
    for (;*np; np = &(*np)->next)
        ;
    p = *np = odr_malloc(tinfo->odr_config, sizeof(*p));
    p->next = 0;
    p->syntax = 0;
    p->name = 0;
    p->convert = 0;
    p->type = type;
    return p;
}

static ZEBRA_RES parse_input(struct filter_info *tinfo, xmlNodePtr ptr,
                             const char *syntax, const char *name)
{
    FOR_EACH_ELEMENT(ptr) {
        if (!XML_STRCMP(ptr->name, "marc"))
        {
            yaz_iconv_t iconv = 0;
            const char *input_charset = "marc-8";
            struct _xmlAttr *attr;
            
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (attr_content(attr, "inputcharset", &input_charset))
                    ;
                else
                {
                    dom_log(YLOG_WARN, tinfo, ptr,
                            "bad attribute @%s, expected @inputcharset",
                            attr->name);
                }
            }
            iconv = yaz_iconv_open("utf-8", input_charset);
            if (!iconv)
            {
                dom_log(YLOG_WARN, tinfo, ptr, 
                        "unsupported @charset '%s'", input_charset);
                return ZEBRA_FAIL;
            }
            else
            {
                struct filter_input *p 
                    = new_input(tinfo, DOM_INPUT_MARC);
                p->u.marc.handle = yaz_marc_create();
                p->u.marc.iconv = iconv;
                
                yaz_marc_iconv(p->u.marc.handle, p->u.marc.iconv);
                
                ptr = ptr->next;
                
                parse_convert(tinfo, ptr, &p->convert);
            }
            break;

        }
        else if (!XML_STRCMP(ptr->name, "xmlreader"))
        {
            struct filter_input *p 
                = new_input(tinfo, DOM_INPUT_XMLREADER);
            struct _xmlAttr *attr;
            const char *level_str = 0;

            p->u.xmlreader.split_level = 0;
            p->u.xmlreader.reader = 0;

            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (attr_content(attr, "level", &level_str))
                    ;
                else
                {
                    dom_log(YLOG_WARN, tinfo, ptr,
                            "bad attribute @%s, expected @level",
                            attr->name);
                }
            }
            if (level_str)
                p->u.xmlreader.split_level = atoi(level_str);
                
            ptr = ptr->next;

            parse_convert(tinfo, ptr, &p->convert);
            break;
        }
        else
        {
            dom_log(YLOG_WARN, tinfo, ptr,
                    "bad element <%s>, expected <marc>|<xmlreader>",
                    ptr->name);
            return ZEBRA_FAIL;
        }
    }
    return ZEBRA_OK;
}

static ZEBRA_RES parse_dom(struct filter_info *tinfo, const char *fname)
{
    char tmp_full_name[1024];
    xmlNodePtr ptr;
    xmlDocPtr doc;

    tinfo->fname = odr_strdup(tinfo->odr_config, fname);
    
    if (yaz_filepath_resolve(tinfo->fname, tinfo->profile_path, 
                             NULL, tmp_full_name))
        tinfo->full_name = odr_strdup(tinfo->odr_config, tmp_full_name);
    else
        tinfo->full_name = odr_strdup(tinfo->odr_config, tinfo->fname);
    
    yaz_log(YLOG_LOG, "%s dom filter: "
            "loading config file %s", tinfo->fname, tinfo->full_name);

    doc = xmlParseFile(tinfo->full_name);
    if (!doc)
    {
        yaz_log(YLOG_WARN, "%s: dom filter: "
                "failed to parse config file %s",
                tinfo->fname, tinfo->full_name);
        return ZEBRA_FAIL;
    }
    /* save because we store ptrs to the content */ 
    tinfo->doc_config = doc;
    
    ptr = xmlDocGetRootElement(doc);
    if (!ptr || ptr->type != XML_ELEMENT_NODE 
        || XML_STRCMP(ptr->name, "dom"))
    {
        dom_log(YLOG_WARN, tinfo, ptr,
                "bad root element <%s>, expected root element <dom>", 
                ptr->name);  
        return ZEBRA_FAIL;
    }

    ptr = ptr->children;
    FOR_EACH_ELEMENT(ptr) {
        if (!XML_STRCMP(ptr->name, "extract"))
        {
            /*
              <extract name="index">
              <xslt stylesheet="first.xsl"/>
              <xslt stylesheet="second.xsl"/>
              </extract>
            */
            struct _xmlAttr *attr;
            struct filter_extract *f =
                odr_malloc(tinfo->odr_config, sizeof(*f));
            
            tinfo->extract = f;
            f->name = 0;
            f->convert = 0;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (attr_content(attr, "name", &f->name))
                    ;
                else
                {
                    dom_log(YLOG_WARN, tinfo, ptr,
                            "bad attribute @%s, expected @name",
                            attr->name);
                }
            }
            parse_convert(tinfo, ptr->children, &f->convert);
        }
        else if (!XML_STRCMP(ptr->name, "retrieve"))
        {  
            /* 
               <retrieve name="F">
               <xslt stylesheet="some.xsl"/>
               <xslt stylesheet="some.xsl"/>
               </retrieve>
            */
            struct _xmlAttr *attr;
            struct filter_retrieve **fp = &tinfo->retrieve_list;
            struct filter_retrieve *f =
                odr_malloc(tinfo->odr_config, sizeof(*f));
            
            while (*fp)
                fp = &(*fp)->next;

            *fp = f;
            f->name = 0;
            f->identifier = 0;
            f->convert = 0;
            f->next = 0;

            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (attr_content(attr, "identifier", 
                                 &f->identifier))
                    ;
                else if (attr_content(attr, "name", &f->name))
                    ;
                else
                {
                    dom_log(YLOG_WARN, tinfo, ptr,
                            "bad attribute @%s,  expected @identifier|@name",
                            attr->name);
                }
            }
            parse_convert(tinfo, ptr->children, &f->convert);
        }
        else if (!XML_STRCMP(ptr->name, "store"))
        {
            /*
              <store name="F">
              <xslt stylesheet="some.xsl"/>
              <xslt stylesheet="some.xsl"/>
              </retrieve>
            */
            struct filter_store *f =
                odr_malloc(tinfo->odr_config, sizeof(*f));
            
            tinfo->store = f;
            f->convert = 0;
            parse_convert(tinfo, ptr->children, &f->convert);
        }
        else if (!XML_STRCMP(ptr->name, "input"))
        {
            /*
              <input syntax="xml">
              <xmlreader level="1"/>
              </input>
              <input syntax="usmarc">
              <marc inputcharset="marc-8"/>
              </input>
            */
            struct _xmlAttr *attr;
            const char  *syntax = 0;
            const char *name = 0;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (attr_content(attr, "syntax", &syntax))
                    ;
                else if (attr_content(attr, "name", &name))
                    ;
                else
                {
                    dom_log(YLOG_WARN, tinfo, ptr,
                            "bad attribute @%s,  expected @syntax|@name",
                            attr->name);
                }
            }
            parse_input(tinfo, ptr->children, syntax, name);
        }
        else
        {
            dom_log(YLOG_WARN, tinfo, ptr,
                    "bad element <%s>, "
                    "expected <extract>|<input>|<retrieve>|<store>",
                    ptr->name);
            return ZEBRA_FAIL;
        }
    }
    return ZEBRA_OK;
}

static struct filter_retrieve *lookup_retrieve(struct filter_info *tinfo,
                                               const char *est)
{
    struct filter_retrieve *f = tinfo->retrieve_list;

    /* return first schema if no est is provided */
    if (!est)
        return f;
    for (; f; f = f->next)
    { 
        /* find requested schema */
        if (est) 
        {    
            if (f->identifier && !strcmp(f->identifier, est))
                return f;
            if (f->name && !strcmp(f->name, est))
                return f;
        } 
    }
    return 0;
}

static ZEBRA_RES filter_config(void *clientData, Res res, const char *args)
{
    struct filter_info *tinfo = clientData;
    if (!args || !*args)
    {
        yaz_log(YLOG_WARN, "dom filter: need config file");
        return ZEBRA_FAIL;
    }

    if (tinfo->fname && !strcmp(args, tinfo->fname))
	return ZEBRA_OK;
    
    tinfo->profile_path = res_get(res, "profilePath");

    destroy_dom(tinfo);
    return parse_dom(tinfo, args);
}

static void filter_destroy(void *clientData)
{
    struct filter_info *tinfo = clientData;
    destroy_dom(tinfo);
    odr_destroy(tinfo->odr_config);
    odr_destroy(tinfo->odr_record);
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


/* DOM filter style indexing */
static int attr_content_xml(struct _xmlAttr *attr, const char *name,
                            xmlChar **dst_content)
{
    if (0 == XML_STRCMP(attr->name, name) && attr->children 
        && attr->children->type == XML_TEXT_NODE)
    {
        *dst_content = (attr->children->content);
        return 1;
    }
    return 0;
}


/* DOM filter style indexing */
static void index_value_of(struct filter_info *tinfo, 
                           struct recExtractCtrl *extctr,
                           RecWord* recword, 
                           xmlNodePtr node, 
                           xmlChar * index_p)
{
    xmlChar *text = xmlNodeGetContent(node);
    size_t text_len = strlen((const char *)text);

    /*dom_log(YLOG_DEBUG, tinfo, node, "Indexing: '%s' '%s'", index_p, text);*/

    /* if there is no text, we do not need to proceed */
    if (text_len)
    {            
        xmlChar *look = index_p;
        xmlChar *bval;
        xmlChar *eval;

        xmlChar index[256];
        xmlChar type[256];

        /* assingning text to be indexed */
        recword->term_buf = (const char *)text;
        recword->term_len = text_len;

        /* parsing all index name/type pairs */
        /* may not start with ' ' or ':' */
        while (*look && ' ' != *look && ':' != *look)
        {
            /* setting name and type to zero */
            *index = '\0';
            *type = '\0';
    
            /* parsing one index name */
            bval = look;
            while (*look && ':' != *look && ' ' != *look)
            {
                look++;
            }
            eval = look;
            strncpy((char *)index, (const char *)bval, eval - bval);
            index[eval - bval] = '\0';
    
    
            /* parsing one index type, if existing */
            if (':' == *look)
            {
                look++;
      
                bval = look;
                while (*look && ' ' != *look)
                {
                    look++;
                }
                eval = look;
                strncpy((char *)type, (const char *)bval, eval - bval);
                type[eval - bval] = '\0';
            }

            /* actually indexing the text given */
            dom_log(YLOG_DEBUG, tinfo, 0, 
                    "INDEX '%s:%s' '%s'", 
                    index ? (const char *) index : "null",
                    type ? (const char *) type : "null", 
                    text ? (const char *) text : "null");

            recword->index_name = (const char *)index;
            if (type && *type)
                recword->index_type = *type;
            (extctr->tokenAdd)(recword);

            /* eat whitespaces */
            if (*look && ' ' == *look && *(look+1))
            {
                look++;
            } 
        }
    }
    
    xmlFree(text); 
}


/* DOM filter style indexing */
static void set_record_info(struct filter_info *tinfo, 
                            struct recExtractCtrl *extctr, 
                            xmlChar * id_p, 
                            xmlChar * rank_p, 
                            xmlChar * type_p)
{
    dom_log(YLOG_DEBUG, tinfo, 0,
            "RECORD id=%s rank=%s type=%s", 
            id_p ? (const char *) id_p : "null",
            rank_p ? (const char *) rank_p : "null",
            type_p ? (const char *) type_p : "null");
    
    if (id_p)
        sscanf((const char *)id_p, "%255s", extctr->match_criteria);

    if (rank_p)
        extctr->staticrank = atozint((const char *)rank_p);

    /*     if (!strcmp("update", type_str)) */
    /*         index_node(tinfo, ctrl, ptr, recword); */
    /*     else if (!strcmp("delete", type_str)) */
    /*         dom_log(YLOG_WARN, tinfo, ptr, "dom filter delete: to be implemented"); */
    /*     else */
    /*         dom_log(YLOG_WARN, tinfo, ptr, "dom filter: unknown record type '%s'",  */
    /*                 type_str); */

}


/* DOM filter style indexing */
static void process_xml_element_zebra_node(struct filter_info *tinfo, 
                                           struct recExtractCtrl *extctr, 
                                           RecWord* recword, 
                                           xmlNodePtr node)
{
    if (node->type == XML_ELEMENT_NODE && node->ns && node->ns->href
        && 0 == XML_STRCMP(node->ns->href, zebra_dom_ns))
    {
         if (0 == XML_STRCMP(node->name, "index"))
         {
            xmlChar *index_p = 0;

            struct _xmlAttr *attr;      
            for (attr = node->properties; attr; attr = attr->next)
            {
                if (attr_content_xml(attr, "name", &index_p))
                {
                    index_value_of(tinfo, extctr, recword,node, index_p);
                }  
                else
                {
                    dom_log(YLOG_WARN, tinfo, node,
                            "bad attribute @%s, expected @name",
                            attr->name);
                }
            }
        }
        else if (0 == XML_STRCMP(node->name, "record"))
        {
            xmlChar *id_p = 0;
            xmlChar *rank_p = 0;
            xmlChar *type_p = 0;

            struct _xmlAttr *attr;
            for (attr = node->properties; attr; attr = attr->next)
            {
                if (attr_content_xml(attr, "id", &id_p))
                    ;
                else if (attr_content_xml(attr, "rank", &rank_p))
                    ;
                else if (attr_content_xml(attr, "type", &type_p))
                    ;
                else
                {
                    dom_log(YLOG_WARN, tinfo, node,
                            "bad attribute @%s, expected @id|@rank|@type",
                            attr->name);
                }

                if (type_p && 0 != strcmp("update", (const char *)type_p))
                {
                    dom_log(YLOG_WARN, tinfo, node,
                            "attribute @%s, only implemented '@type='update'",
                            attr->name);
                }
            }
            set_record_info(tinfo, extctr, id_p, rank_p, type_p);
        } 
        else
        {
            dom_log(YLOG_WARN, tinfo, node,
                    "bad element <%s>,"
                    " expected <record>|<index> in namespace '%s'",
                    node->name, zebra_dom_ns);
        }
    }
}


/* DOM filter style indexing */
static void process_xml_pi_node(struct filter_info *tinfo, 
                                struct recExtractCtrl *extctr, 
                                xmlNodePtr node,
                                xmlChar **index_pp)
{
    /* if right PI name, continue parsing PI */
    if (0 == strcmp(zebra_pi_name, (const char *)node->name))
    {
        xmlChar *pi_p =  node->content;
        xmlChar *look = pi_p;
    
        xmlChar *bval;
        xmlChar *eval;

        /* parsing PI record instructions */
        if (0 == strncmp((const char *)look, "record", 6))
        {
            xmlChar id[256];
            xmlChar rank[256];
            xmlChar type[256];

            *id = '\0';
            *rank = '\0';
            *type = '\0';
      
            look += 6;
      
            /* eat whitespace */
            while (*look && ' ' == *look && *(look+1))
                look++;

            /* parse possible id */
            if (*look && 0 == strncmp((const char *)look, "id=", 3))
            {
                look += 3;
                bval = look;
                while (*look && ' ' != *look)
                    look++;
                eval = look;
                strncpy((char *)id, (const char *)bval, eval - bval);
                id[eval - bval] = '\0';
            }
      
            /* eat whitespace */
            while (*look && ' ' == *look && *(look+1))
                look++;
      
            /* parse possible rank */
            if (*look && 0 == strncmp((const char *)look, "rank=", 5))
            {
                look += 6;
                bval = look;
                while (*look && ' ' != *look)
                    look++;
                eval = look;
                strncpy((char *)rank, (const char *)bval, eval - bval);
                rank[eval - bval] = '\0';
            }

            /* eat whitespace */
            while (*look && ' ' == *look && *(look+1))
                look++;

            if (look && '\0' != *look)
            {
                dom_log(YLOG_WARN, tinfo, node,
                        "content '%s', can not parse '%s'",
                        pi_p, look);
            }
            else 
                set_record_info(tinfo, extctr, id, rank, 0);

        } 
        /* parsing index instruction */
        else if (0 == strncmp((const char *)look, "index", 5))
        {
            look += 5;
      
            /* eat whitespace */
            while (*look && ' ' == *look && *(look+1))
                look++;

            /* export index instructions to outside */
            *index_pp = look;
        } 
        else 
        {
            dom_log(YLOG_WARN, tinfo, node,
                    "content '%s', can not parse '%s'",
                    pi_p, look);
        }
    }
}

/* DOM filter style indexing */
static void process_xml_element_node(struct filter_info *tinfo, 
                                     struct recExtractCtrl *extctr, 
                                     RecWord* recword, 
                                     xmlNodePtr node)
{
    /* remember indexing instruction from PI to next element node */
    xmlChar *index_p = 0;

    /* check if we are an element node in the special zebra namespace 
       and either set record data or index value-of node content*/
    process_xml_element_zebra_node(tinfo, extctr, recword, node);
  
    /* loop through kid nodes */
    for (node = node->children; node; node = node->next)
    {
        /* check and set PI record and index index instructions */
        if (node->type == XML_PI_NODE)
        {
            process_xml_pi_node(tinfo, extctr, node, &index_p);
        }
        else if (node->type == XML_ELEMENT_NODE)
        {
            /* if there was a PI index instruction before this element */
            if (index_p)
            {
                index_value_of(tinfo, extctr, recword, node, index_p);
                index_p = 0;
            }
            process_xml_element_node(tinfo, extctr, recword,node);
        }
        else
            continue;
    }
}


/* DOM filter style indexing */
static void extract_dom_doc_node(struct filter_info *tinfo, 
                                 struct recExtractCtrl *extctr, 
                                 xmlDocPtr doc)
{
    xmlChar *buf_out;
    int len_out;

    /* only need to do the initialization once, reuse recword for all terms */
    RecWord recword;
    (*extctr->init)(extctr, &recword);

    if (extctr->flagShowRecords)
    {
        xmlDocDumpMemory(doc, &buf_out, &len_out);
        fwrite(buf_out, len_out, 1, stdout);
        xmlFree(buf_out);
    }

    process_xml_element_node(tinfo, extctr, &recword, (xmlNodePtr)doc);
}




static int convert_extract_doc(struct filter_info *tinfo, 
                               struct filter_input *input,
                               struct recExtractCtrl *p, 
                               xmlDocPtr doc)

{
    xmlChar *buf_out;
    int len_out;
    const char *params[10];
    xsltStylesheetPtr last_xsp = 0;
    xmlDocPtr store_doc = 0;

    params[0] = 0;
    set_param_str(params, "schema", zebra_dom_ns, tinfo->odr_record);

    /* input conversion */
    perform_convert(tinfo, input->convert, params, &doc, 0);

    if (tinfo->store)
    {
        /* store conversion */
        store_doc = xmlCopyDoc(doc, 1);
        perform_convert(tinfo, tinfo->store->convert,
                        params, &store_doc, &last_xsp);
    }
    
    if (last_xsp)
        xsltSaveResultToString(&buf_out, &len_out, 
                               store_doc ? store_doc : doc, last_xsp);
    else
        xmlDocDumpMemory(store_doc ? store_doc : doc, &buf_out, &len_out);
    if (p->flagShowRecords)
	fwrite(buf_out, len_out, 1, stdout);
    (*p->setStoreData)(p, buf_out, len_out);
    xmlFree(buf_out);

    if (store_doc)
        xmlFreeDoc(store_doc);

    /* extract conversion */
    perform_convert(tinfo, tinfo->extract->convert, params, &doc, 0);

    /* finally, do the indexing */
    if (doc)
    {
        extract_dom_doc_node(tinfo, p, doc);
        /* extract_doc_alvis(tinfo, p, doc); */
	xmlFreeDoc(doc);
    }

    return RECCTRL_EXTRACT_OK;
}

static int extract_xml_split(struct filter_info *tinfo,
                             struct filter_input *input,
                             struct recExtractCtrl *p)
{
    int ret;

    if (p->first_record)
    {
        if (input->u.xmlreader.reader)
            xmlFreeTextReader(input->u.xmlreader.reader);
        input->u.xmlreader.reader = xmlReaderForIO(ioread_ex, ioclose_ex,
                                                   p /* I/O handler */,
                                                   0 /* URL */, 
                                                   0 /* encoding */,
                                                   XML_PARSE_XINCLUDE|
                                                   XML_PARSE_NOENT);
    }
    if (!input->u.xmlreader.reader)
	return RECCTRL_EXTRACT_ERROR_GENERIC;

    ret = xmlTextReaderRead(input->u.xmlreader.reader);
    while (ret == 1)
    {
        int type = xmlTextReaderNodeType(input->u.xmlreader.reader);
        int depth = xmlTextReaderDepth(input->u.xmlreader.reader);
        if (type == XML_READER_TYPE_ELEMENT && 
            input->u.xmlreader.split_level == depth)
        {
            xmlNodePtr ptr
                = xmlTextReaderExpand(input->u.xmlreader.reader);
            if (ptr)
            {
                xmlNodePtr ptr2 = xmlCopyNode(ptr, 1);
                xmlDocPtr doc = xmlNewDoc((const xmlChar*) "1.0");
                
                xmlDocSetRootElement(doc, ptr2);
                
                return convert_extract_doc(tinfo, input, p, doc);
            }
            else
            {
                xmlFreeTextReader(input->u.xmlreader.reader);
                input->u.xmlreader.reader = 0;
                return RECCTRL_EXTRACT_ERROR_GENERIC;
            }
        }
        ret = xmlTextReaderRead(input->u.xmlreader.reader);
    }
    xmlFreeTextReader(input->u.xmlreader.reader);
    input->u.xmlreader.reader = 0;
    return RECCTRL_EXTRACT_EOF;
}

static int extract_xml_full(struct filter_info *tinfo, 
                            struct filter_input *input,
                            struct recExtractCtrl *p)
{
    if (p->first_record) /* only one record per stream */
    {
        xmlDocPtr doc = xmlReadIO(ioread_ex, ioclose_ex, 
                                  p /* I/O handler */,
                                  0 /* URL */,
                                  0 /* encoding */,
                                  XML_PARSE_XINCLUDE|XML_PARSE_NOENT);
        if (!doc)
        {
            return RECCTRL_EXTRACT_ERROR_GENERIC;
        }
        return convert_extract_doc(tinfo, input, p, doc);
    }
    else
        return RECCTRL_EXTRACT_EOF;
}

static int extract_iso2709(struct filter_info *tinfo,
                           struct filter_input *input,
                           struct recExtractCtrl *p)
{
    char buf[100000];
    int record_length;
    int read_bytes, r;

    if (p->stream->readf(p->stream, buf, 5) != 5)
        return RECCTRL_EXTRACT_EOF;
    while (*buf < '0' || *buf > '9')
    {
        int i;

        dom_log(YLOG_WARN, tinfo, 0,
                "MARC: Skipping bad byte %d (0x%02X)",
                *buf & 0xff, *buf & 0xff);
        for (i = 0; i<4; i++)
            buf[i] = buf[i+1];

        if (p->stream->readf(p->stream, buf+4, 1) != 1)
            return RECCTRL_EXTRACT_EOF;
    }
    record_length = atoi_n (buf, 5);
    if (record_length < 25)
    {
        dom_log(YLOG_WARN, tinfo, 0,
                "MARC record length < 25, is %d",  record_length);
        return RECCTRL_EXTRACT_ERROR_GENERIC;
    }
    read_bytes = p->stream->readf(p->stream, buf+5, record_length-5);
    if (read_bytes < record_length-5)
    {
        dom_log(YLOG_WARN, tinfo, 0,
                "couldn't read whole MARC record");
        return RECCTRL_EXTRACT_ERROR_GENERIC;
    }
    r = yaz_marc_read_iso2709(input->u.marc.handle,  buf, record_length);
    if (r < record_length)
    {
        dom_log (YLOG_WARN, tinfo, 0,
                 "parsing of MARC record failed r=%d length=%d",
                 r, record_length);
        return RECCTRL_EXTRACT_ERROR_GENERIC;
    }
    else
    {
        xmlDocPtr rdoc;
        xmlNode *root_ptr;
        yaz_marc_write_xml(input->u.marc.handle, &root_ptr, 0, 0, 0);
        rdoc = xmlNewDoc((const xmlChar*) "1.0");
        xmlDocSetRootElement(rdoc, root_ptr);
        return convert_extract_doc(tinfo, input, p, rdoc);        
    }
    return RECCTRL_EXTRACT_OK;
}

static int filter_extract(void *clientData, struct recExtractCtrl *p)
{
    struct filter_info *tinfo = clientData;
    struct filter_input *input = tinfo->input_list;

    if (!input)
        return RECCTRL_EXTRACT_ERROR_GENERIC;

    odr_reset(tinfo->odr_record);
    switch(input->type)
    {
    case DOM_INPUT_XMLREADER:
        if (input->u.xmlreader.split_level == 0)
            return extract_xml_full(tinfo, input, p);
        else
            return extract_xml_split(tinfo, input, p);
        break;
    case DOM_INPUT_MARC:
        return extract_iso2709(tinfo, input, p);
    }
    return RECCTRL_EXTRACT_ERROR_GENERIC;
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
    /* const char *esn = zebra_dom_ns; */
    const char *esn = 0;
    const char *params[32];
    struct filter_info *tinfo = clientData;
    xmlDocPtr doc;
    struct filter_retrieve *retrieve;
    xsltStylesheetPtr last_xsp = 0;

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
    retrieve = lookup_retrieve(tinfo, esn);
    if (!retrieve)
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
        if (retrieve->name)
            set_param_str(params, "schema", retrieve->name, p->odr);
        else if (retrieve->identifier)
            set_param_str(params, "schema", retrieve->identifier, p->odr);
        else
            set_param_str(params, "schema", "", p->odr);

    if (p->score >= 0)
	set_param_int(params, "score", p->score, p->odr);
    set_param_int(params, "size", p->recordSize, p->odr);

    doc = xmlReadIO(ioread_ret, ioclose_ret, p /* I/O handler */,
		    0 /* URL */,
		    0 /* encoding */,
		    XML_PARSE_XINCLUDE|XML_PARSE_NOENT);
    if (!doc)
    {
        p->diagnostic = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
        return 0;
    }

    /* retrieve conversion */
    perform_convert(tinfo, retrieve->convert, params, &doc, &last_xsp);
    if (!doc)
    {
        p->diagnostic = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }
    else if (p->input_format == VAL_NONE || p->input_format == VAL_TEXT_XML)
    {
        xmlChar *buf_out;
        int len_out;

        if (last_xsp)
            xsltSaveResultToString(&buf_out, &len_out, doc, last_xsp);
        else
            xmlDocDumpMemory(doc, &buf_out, &len_out);            

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

        if (last_xsp)
            xsltSaveResultToString(&buf_out, &len_out, doc, last_xsp);
        else
            xmlDocDumpMemory(doc, &buf_out, &len_out);            
        
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
    xmlFreeDoc(doc);
    return 0;
}

static struct recType filter_type = {
    0,
    "dom",
    filter_init,
    filter_config,
    filter_destroy,
    filter_extract,
    filter_retrieve
};

RecType
#ifdef IDZEBRA_STATIC_DOM
idzebra_filter_dom
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

