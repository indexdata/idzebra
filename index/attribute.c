/* $Id: attribute.c,v 1.21 2006-02-22 08:42:16 adam Exp $
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

#include <yaz/log.h>
#include <idzebra/res.h>
#include <idzebra/util.h>
#include "index.h"

static data1_att *getatt(data1_attset *p, int att, const char *sattr)
{
    data1_att *a;
    data1_attset_child *c;

    /* scan local set */
    for (a = p->atts; a; a = a->next)
	if (sattr && !yaz_matchstr(sattr, a->name))
	    return a;
	else if (a->value == att)
	    return a;
    /* scan included sets */
    for (c = p->children; c; c = c->next)
	if ((a = getatt(c->child, att, sattr)))
	    return a;
    return 0;
}

int att_getentbyatt(ZebraHandle zi, attent *res, oid_value set, int att,
		const char *sattr)
{
    data1_att *r;
    data1_attset *p;

    if (!(p = data1_attset_search_id (zi->reg->dh, set)))
    {
	zebraExplain_loadAttsets (zi->reg->dh, zi->res);
	p = data1_attset_search_id (zi->reg->dh, set);
    }
    if (!p)   /* set undefined */
    {
	if (sattr)     
	    return -1; /* return bad string attribute */
	else
	    return -2; /* return bad set */
    }
    if (!(r = getatt(p, att, sattr)))
	return -1;
    res->attset_ordinal = r->parent->reference;
    res->local_attributes = r->locals;
    return 0;
}
