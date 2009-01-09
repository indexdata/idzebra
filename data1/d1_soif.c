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


/*
 * This module generates SOIF (Simple Object Interchange Format) records
 * from d1-nodes. nested elements are flattened out, depth first, by
 * concatenating the tag names at each level.
 */

#include <yaz/wrbuf.h>
#include <idzebra/data1.h>

static int nodetoelement(data1_node *n, int select, char *prefix, WRBUF b)
{
    data1_node *c;
    char tmp[1024];

    for (c = n->child; c; c = c->next)
    {
	char *tag;

	if (c->which == DATA1N_tag)
	{
	    if (select && !c->u.tag.node_selected)
		continue;
	    if (c->u.tag.element && c->u.tag.element->tag)
		tag = c->u.tag.element->tag->names->name; /* first name */
	    else
	    tag = c->u.tag.tag; /* local string tag */

	    if (*prefix)
		sprintf(tmp, "%s-%s", prefix, tag);
	    else
		strcpy(tmp, tag);

	    if (nodetoelement(c, select, tmp, b) < 0)
		return 0;
	}
	else if (c->which == DATA1N_data)
	{
	    char *p = c->u.data.data;
	    int l = c->u.data.len;

	    wrbuf_write(b, prefix, strlen(prefix));

	    sprintf(tmp, "{%d}:\t", l);
	    wrbuf_write(b, tmp, strlen(tmp));
	    wrbuf_write(b, p, l);
	    wrbuf_putc(b, '\n');
	}
    }
    return 0;
}

char *data1_nodetosoif (data1_handle dh, data1_node *n, int select, int *len)
{
    WRBUF b = data1_get_wrbuf (dh);
    char buf[128];

    wrbuf_rewind(b);
    
    if (n->which != DATA1N_root)
	return 0;
    sprintf(buf, "@%s{\n", n->u.root.type);
    wrbuf_write(b, buf, strlen(buf));
    if (nodetoelement(n, select, "", b))
	return 0;
    wrbuf_write(b, "}\n", 2);
    *len = wrbuf_len(b);
    return wrbuf_buf(b);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

