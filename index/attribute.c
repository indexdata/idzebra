/* $Id: attribute.c,v 1.15.2.1 2006-08-14 10:38:57 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

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

#include <yaz/log.h>
#include <res.h>
#include <zebrautl.h>
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
    if (!p)
	return -2;
    if (!(r = getatt(p, att, sattr)))
	return -1;
    res->attset_ordinal = r->parent->reference;
    res->local_attributes = r->locals;
    return 0;
}
