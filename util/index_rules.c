/* $Id: index_rules.c,v 1.2 2007-10-24 13:55:55 adam Exp $
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

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "index_rules.h"
#include <yaz/match_glob.h>
#include <yaz/xmalloc.h>
#include <yaz/wrbuf.h>
#include <yaz/log.h>

struct zebra_index_rules_s {
#if YAZ_HAVE_XML2
    struct zebra_index_rule *rules;
    xmlDocPtr doc;
#endif
};

#if YAZ_HAVE_XML2
struct zebra_index_rule {
    const xmlNode *ptr;
    const char *id;
    const char *locale;
    const char *position;
    const char *alwaysmatches;
    const char *firstinfield;
    const char *sort;
    struct zebra_index_rule *next;
};

struct zebra_index_rule *parse_index_rule(const xmlNode *ptr)
{
    struct _xmlAttr *attr;
    struct zebra_index_rule *rule;
    
    rule = xmalloc(sizeof(*rule)); 
    rule->next = 0;
    rule->ptr = ptr;
    rule->locale = 0;
    rule->id = 0;
    rule->position = 0;
    rule->alwaysmatches = 0;
    rule->firstinfield = 0;
    rule->sort = 0;
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
            else if (!strcmp((const char *) attr->name, "sort"))
                rule->sort = (const char *) attr->children->content;
            else
            {
                yaz_log(YLOG_WARN, "Unsupport attribute '%s' for indexrule",
                        attr->name);
                xfree(rule);
                return 0;
            }
        }
    }
    return rule;
}
/* YAZ_HAVE_XML2 */
#endif

zebra_index_rules_t zebra_index_rules_create(const char *fname)
{
    xmlDocPtr doc = xmlParseFile(fname);
    if (!doc)
        return 0;
    return zebra_index_rules_create_doc(doc);
}

zebra_index_rules_t zebra_index_rules_create_doc(xmlDocPtr doc)
{
#if YAZ_HAVE_XML2
    zebra_index_rules_t r = xmalloc(sizeof(*r));
    struct zebra_index_rule **rp = &r->rules;
    const xmlNode *top = xmlDocGetRootElement(doc);
    
    r->doc = doc;
    *rp = 0;
    if (top && top->type == XML_ELEMENT_NODE
        && !strcmp((const char *) top->name, "indexrules"))
    {
        const xmlNode *ptr = top->children;
        for (; ptr; ptr = ptr->next)
        {
            if (ptr->type == XML_ELEMENT_NODE
                && !strcmp((const char *) ptr->name, "indexrule"))
            {
                *rp = parse_index_rule(ptr);
                if (!*rp)
                {
                    zebra_index_rules_destroy(r);
                    return 0;
                }
                rp = &(*rp)->next;
            }
        }
    }
    else
    {
        zebra_index_rules_destroy(r);
        r = 0;
    }
    return r;
#else
    yaz_log(YLOG_WARN, "Cannot read index rules %s because YAZ is without XML "
            "support", fname);
    return 0;
/* YAZ_HAVE_XML2 */
#endif
}

void zebra_index_rules_destroy(zebra_index_rules_t r)
{
#if YAZ_HAVE_XML2
    struct zebra_index_rule *rule;
    while (r->rules)
    {
        rule = r->rules;
        r->rules = rule->next;
        xfree(rule);
    }
    xmlFreeDoc(r->doc);

#endif
    xfree(r);
}

const char *zebra_index_rule_lookup_str(zebra_index_rules_t r, const char *id)
{
#if YAZ_HAVE_XML2

    struct zebra_index_rule *rule = r->rules;
        
    while (rule && !yaz_match_glob(rule->id, id))
        rule = rule->next;
    if (rule)
        return rule->id;
#endif
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

