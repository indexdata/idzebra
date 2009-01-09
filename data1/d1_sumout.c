/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <yaz/log.h>
#include <yaz/proto.h>
#include <idzebra/data1.h>

static int *f_integer(data1_node *c, ODR o)
{
    int *r;
    char intbuf[64];

    if (!c->child || c->child->which != DATA1N_data ||
	c->child->u.data.len > 63)
	return 0;
    r = (int *)odr_malloc(o, sizeof(*r));
    sprintf(intbuf, "%.*s", 63, c->child->u.data.data);
    *r = atoi(intbuf);
    return r;
}

static char *f_string(data1_node *c, ODR o)
{
    char *r;

    if (!c->child || c->child->which != DATA1N_data)
	return 0;
    r = (char *)odr_malloc(o, c->child->u.data.len+1);
    memcpy(r, c->child->u.data.data, c->child->u.data.len);
    r[c->child->u.data.len] = '\0';
    return r;
}

Z_BriefBib *data1_nodetosummary (data1_handle dh, data1_node *n,
				 int select, ODR o)
{
    Z_BriefBib *res = (Z_BriefBib *)odr_malloc(o, sizeof(*res));
    data1_node *c;

    assert(n->which == DATA1N_root);
    if (strcmp(n->u.root.type, "summary"))
    {
	yaz_log(YLOG_WARN, "Attempt to convert a non-summary record");
	return 0;
    }

    res->title = "[UNKNOWN]";
    res->author = 0;
    res->callNumber = 0;
    res->recordType = 0;
    res->bibliographicLevel = 0;
    res->num_format = 0;
    res->format = 0;
    res->publicationPlace = 0;
    res->publicationDate = 0;
    res->targetSystemKey = 0;
    res->satisfyingElement = 0;
    res->rank = 0;
    res->documentId = 0;
    res->abstract = 0;
    res->otherInfo = 0;

    for (c = n->child; c; c = c->next)
    {
	if (c->which != DATA1N_tag || !c->u.tag.element)
	{
	    yaz_log(YLOG_WARN, "Malformed element in Summary record");
	    return 0;
	}
	if (select && !c->u.tag.node_selected)
	    continue;
	switch (c->u.tag.element->tag->value.numeric)
	{
	    case 0: res->title = f_string(c, o); break;
	    case 1: res->author = f_string(c, o); break;
	    case 2: res->callNumber = f_string(c, o); break;
	    case 3: res->recordType = f_string(c, o); break;
	    case 4: res->bibliographicLevel = f_string(c, o); break;
	    case 5: abort();   /* TODO */
	    case 10: res->publicationPlace = f_string(c, o); break;
	    case 11: res->publicationDate = f_string(c, o); break;
	    case 12: res->targetSystemKey = f_string(c, o); break;
	    case 13: res->satisfyingElement = f_string(c, o); break;
	    case 14: res->rank = f_integer(c, o); break;
	    case 15: res->documentId = f_string(c, o); break;
	    case 16: res->abstract = f_string(c, o); break;
	    case 17: abort(); /* TODO */
	    default:
	        yaz_log(YLOG_WARN, "Unknown element in Summary record.");
	}
    }
    return res;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

