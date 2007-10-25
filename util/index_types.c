/* $Id: index_types.c,v 1.2 2007-10-25 19:25:00 adam Exp $
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
   along with Zebra; see the file LICENSE.zebra.  If not, write to the
   Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.
*/

/** 
    \file
    \brief Implementation of Zebra's index types system
*/

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "index_types.h"
#include <yaz/icu_I18N.h>
#include <yaz/match_glob.h>
#include <yaz/xmalloc.h>
#include <yaz/wrbuf.h>
#include <yaz/log.h>

struct zebra_index_types_s {
#if YAZ_HAVE_XML2
    zebra_index_type_t rules;
    xmlDocPtr doc;
#endif
};

#if YAZ_HAVE_XML2
struct zebra_index_type_s {
    const xmlNode *ptr;
    const char *id;
    const char *locale;
    const char *position;
    const char *alwaysmatches;
    const char *firstinfield;
    int sort_flag;
    int index_flag;
    int staticrank_flag;
    int simple_chain;
#if HAVE_ICU
    struct icu_chain *chain;
#endif
    zebra_index_type_t next;
    WRBUF simple_buf;
    size_t simple_off;
};

static void index_type_destroy(zebra_index_type_t t);

zebra_index_type_t parse_index_type(const xmlNode *ptr)
{
    struct _xmlAttr *attr;
    struct zebra_index_type_s *rule;
    
    rule = xmalloc(sizeof(*rule)); 
    rule->next = 0;
#if HAVE_ICU
    rule->chain = 0;
#endif
    rule->ptr = ptr;
    rule->locale = 0;
    rule->id = 0;
    rule->position = 0;
    rule->alwaysmatches = 0;
    rule->firstinfield = 0;
    rule->sort_flag = 0;
    rule->index_flag = 1;
    rule->staticrank_flag = 0;
    rule->simple_chain = 0;
    rule->simple_buf = wrbuf_alloc();
    for (attr = ptr->properties; attr; attr = attr->next)
    {
        if (attr->children && attr->children->type == XML_TEXT_NODE)
        {
            if (!strcmp((const char *) attr->name, "id"))
                rule->id = (const char *) attr->children->content;
            else if (!strcmp((const char *) attr->name, "locale"))
                rule->locale = (const char *) attr->children->content;
            else if (!strcmp((const char *) attr->name, "position"))
                rule->position = (const char *) attr->children->content;
            else if (!strcmp((const char *) attr->name, "alwaysmatches"))
                rule->alwaysmatches = (const char *) attr->children->content;
            else if (!strcmp((const char *) attr->name, "firstinfield"))
                rule->firstinfield = (const char *) attr->children->content;
            else if (!strcmp((const char *) attr->name, "index"))
            {
                const char *v = (const char *) attr->children->content;
                if (v)
                    rule->index_flag = *v == '1';
            }
            else if (!strcmp((const char *) attr->name, "sort"))
            {
                const char *v = (const char *) attr->children->content;
                if (v)
                    rule->sort_flag = *v == '1';
            }
            else if (!strcmp((const char *) attr->name, "staticrank"))
            {
                const char *v = (const char *) attr->children->content;
                if (v)
                    rule->staticrank_flag = *v == '1';
            }
            else
            {
                yaz_log(YLOG_WARN, "Unsupport attribute '%s' for indextype",
                        attr->name);
                index_type_destroy(rule);
                return 0;
            }
        }
    }
    ptr = ptr->children;
    while (ptr && ptr->type != XML_ELEMENT_NODE)
        ptr = ptr->next;
    if (!ptr)
    {
        yaz_log(YLOG_WARN, "Missing rules for indexrule");
        index_type_destroy(rule);
        rule = 0;
    }
    else if (!strcmp((const char *) ptr->name, "icu_chain"))
    {
#if HAVE_ICU
        UErrorCode status;
        rule->chain = icu_chain_xml_config(ptr,
                                           rule->locale,
                                           rule->sort_flag,
                                           &status);
        if (!rule->chain)
        {
            index_type_destroy(rule);
            rule = 0;
        }
#else
        yaz_log(YLOG_WARN, "ICU unsupported (must be part of YAZ)");
        xfree(rule);
        rule = 0;
#endif
    }
    else if (!strcmp((const char *) ptr->name, "simple"))
    {
        rule->simple_chain = 1;
    }
    else 
    {
        yaz_log(YLOG_WARN, "Unsupported mapping %s for indexrule",  ptr->name);
        index_type_destroy(rule);
        rule = 0;
    }
    return rule;
}
/* YAZ_HAVE_XML2 */
#endif

zebra_index_types_t zebra_index_types_create(const char *fname)
{
    xmlDocPtr doc = xmlParseFile(fname);
    if (!doc)
        return 0;
    return zebra_index_types_create_doc(doc);
}

zebra_index_types_t zebra_index_types_create_doc(xmlDocPtr doc)
{
#if YAZ_HAVE_XML2
    zebra_index_types_t r = xmalloc(sizeof(*r));
    zebra_index_type_t *rp = &r->rules;
    const xmlNode *top = xmlDocGetRootElement(doc);
    
    r->doc = doc;
    *rp = 0;
    if (top && top->type == XML_ELEMENT_NODE
        && !strcmp((const char *) top->name, "indextypes"))
    {
        const xmlNode *ptr = top->children;
        for (; ptr; ptr = ptr->next)
        {
            if (ptr->type == XML_ELEMENT_NODE
                && !strcmp((const char *) ptr->name, "indextype"))
            {
                *rp = parse_index_type(ptr);
                if (!*rp)
                {
                    zebra_index_types_destroy(r);
                    return 0;
                }
                rp = &(*rp)->next;
            }
        }
    }
    else
    {
        zebra_index_types_destroy(r);
        r = 0;
    }
    return r;
#else
    yaz_log(YLOG_WARN, "XML unsupported. Cannot read index rules");
    return 0;
/* YAZ_HAVE_XML2 */
#endif
}

static void index_type_destroy(zebra_index_type_t t)
{
    if (t)
    {
#if HAVE_ICU
        if (t->chain)
            icu_chain_destroy(t->chain);
#endif
        wrbuf_destroy(t->simple_buf);
        xfree(t);
    }
}

void zebra_index_types_destroy(zebra_index_types_t r)
{
    if (r)
    {
#if YAZ_HAVE_XML2
        zebra_index_type_t rule;
        while (r->rules)
        {
            rule = r->rules;
            r->rules = rule->next;
            index_type_destroy(rule);
        }
        xmlFreeDoc(r->doc);
        
#endif
        xfree(r);
    }
}

zebra_index_type_t zebra_index_type_get(zebra_index_types_t types, 
                                        const char *id)
{
#if YAZ_HAVE_XML2
    zebra_index_type_t rule = types->rules;
        
    while (rule && !yaz_match_glob(rule->id, id))
        rule = rule->next;
    return rule;
#endif
    return 0;
}

const char *zebra_index_type_lookup_str(zebra_index_types_t types,
                                        const char *id)
{
    zebra_index_type_t t = zebra_index_type_get(types, id);
    if (t)
        return t->id;
    return 0;
}

int zebra_index_type_is_index(zebra_index_type_t type)
{
    return type->index_flag;
}

int zebra_index_type_is_sort(zebra_index_type_t type)
{
    return type->sort_flag;
}

int zebra_index_type_is_staticrank(zebra_index_type_t type)
{
    return type->staticrank_flag;
}

#define SE_CHARS ";,.()-/?<> \r\n\t"

int tokenize_simple(zebra_index_type_t type,
                    const char **result_buf, size_t *result_len)
{
    char *buf = wrbuf_buf(type->simple_buf);
    size_t len = wrbuf_len(type->simple_buf);
    size_t i = type->simple_off;
    size_t start;

    while (i < len && strchr(SE_CHARS, buf[i]))
        i++;
    start = i;
    while (i < len && !strchr(SE_CHARS, buf[i]))
    {
        if (buf[i] > 32 && buf[i] < 127)
            buf[i] = tolower(buf[i]);
        i++;
    }

    type->simple_off = i;
    if (start != i)
    {
        *result_buf = buf + start;
        *result_len = i - start;
        return 1;
    }
    return 0;
 }

int zebra_index_type_tokenize(zebra_index_type_t type,
                              const char *buf, size_t len,
                              const char **result_buf, size_t *result_len)
{
    if (type->simple_chain)
    {
        if (buf)
        {
            wrbuf_rewind(type->simple_buf);
            wrbuf_write(type->simple_buf, buf, len);
            type->simple_off = 0;
        }
        return tokenize_simple(type, result_buf, result_len);
    }
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

