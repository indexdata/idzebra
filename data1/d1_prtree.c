/* $Id: d1_prtree.c,v 1.8 2006-05-10 08:13:18 adam Exp $
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

#include <yaz/log.h>
#include <idzebra/data1.h>

static void pr_string (FILE *out, const char *str, int len)
{
    int i;
    for (i = 0; i<len; i++)
    {
	int c = str[i];
	if (c < 32 || c >126)
	    fprintf (out, "\\x%02x", c & 255);
	else
	    fputc (c, out);
    }
}

static void pr_tree (data1_handle dh, data1_node *n, FILE *out, int level)
{
    fprintf (out, "%*s", level, "");
    switch (n->which)
    {
    case DATA1N_root:
        fprintf (out, "root abstract syntax=%s\n", n->u.root.type);
        break;
    case DATA1N_tag:
	fprintf (out, "tag type=%s sel=%d\n", n->u.tag.tag,
                 n->u.tag.node_selected);
        if (n->u.tag.attributes)
        {
            data1_xattr *xattr = n->u.tag.attributes;
            fprintf (out, "%*s attr", level, "");
            for (; xattr; xattr = xattr->next)
                fprintf (out, " %s=%s ", xattr->name, xattr->value);
            fprintf (out, "\n");
        }
	break;
    case DATA1N_data:
    case DATA1N_comment:
        if (n->which == DATA1N_data)
            fprintf (out, "data type=");
        else
            fprintf (out, "comment type=");
	switch (n->u.data.what)
	{
	case DATA1I_inctxt:
	    fprintf (out, "inctxt\n");
	    break;
	case DATA1I_incbin:
	    fprintf (out, "incbin\n");
	    break;
	case DATA1I_text:
	    fprintf (out, "text '");
	    pr_string (out, n->u.data.data, n->u.data.len);
	    fprintf (out, "'\n");
	    break;
	case DATA1I_num:
	    fprintf (out, "num '");
	    pr_string (out, n->u.data.data, n->u.data.len);
	    fprintf (out, "'\n");
	    break;
	case DATA1I_oid:
	    fprintf (out, "oid '");
	    pr_string (out, n->u.data.data, n->u.data.len);
	    fprintf (out, "'\n");
	    break;
	case DATA1I_xmltext:
	    fprintf (out, "xml text '");
	    pr_string (out, n->u.data.data, n->u.data.len);
	    fprintf (out, "'\n");
            break;
	default:
	    fprintf (out, "unknown(%d)\n", n->u.data.what);
	    break;
	}
	break;
    case DATA1N_preprocess:
	fprintf (out, "preprocess target=%s\n", n->u.preprocess.target);
        if (n->u.preprocess.attributes)
        {
            data1_xattr *xattr = n->u.preprocess.attributes;
            fprintf (out, "%*s attr", level, "");
            for (; xattr; xattr = xattr->next)
                fprintf (out, " %s=%s ", xattr->name, xattr->value);
            fprintf (out, "\n");
        }
	break;
    case DATA1N_variant:
	fprintf (out, "variant\n");
#if 0
	if (n->u.variant.type->name)
	    fprintf (out, " class=%s type=%d value=%s\n",
		     n->u.variant.type->name, n->u.variant.type->type,
		     n->u.variant.value);
#endif
	break;
    default:
	fprintf (out, "unknown(%d)\n", n->which);
    }
    if (n->child)
	pr_tree (dh, n->child, out, level+4);
    if (n->next)
	pr_tree (dh, n->next, out, level);
    else
    {
	if (n->parent && n->parent->last_child != n)
	    fprintf(out, "%*sWARNING: last_child=%p != %p\n", level, "",
		    n->parent->last_child, n);
    }
}


void data1_pr_tree (data1_handle dh, data1_node *n, FILE *out)
{
    pr_tree (dh, n, out, 0);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

