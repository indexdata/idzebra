/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: attribute.c,v $
 * Revision 1.11  1999-11-30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.10  1999/02/02 14:50:49  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.9  1998/05/20 10:12:14  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.8  1998/03/05 08:45:11  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.7  1997/10/29 12:05:01  adam
 * Server produces diagnostic "Unsupported Attribute Set" when appropriate.
 *
 * Revision 1.6  1997/09/17 12:19:11  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.5  1997/09/05 15:30:08  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.4  1996/10/29 14:06:48  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.3  1996/05/09 07:28:54  quinn
 * Work towards phrases and multiple registers
 *
 * Revision 1.2  1995/11/15  19:13:07  adam
 * Work on record management.
 *
 *
 * This interface is used by other modules (the Z-server in particular)
 * to normalize the attributes given in queries.
 */

#include <stdio.h>

#include <yaz/log.h>
#include <res.h>
#include <zebrautl.h>
#include "zserver.h"

static data1_att *getatt(data1_attset *p, int att)
{
    data1_att *a;
    data1_attset_child *c;

    /* scan local set */
    for (a = p->atts; a; a = a->next)
	if (a->value == att)
	    return a;
    /* scan included sets */
    for (c = p->children; c; c = c->next)
	if ((a = getatt(c->child, att)))
	    return a;
    return 0;
}

int att_getentbyatt(ZebraHandle zi, attent *res, oid_value set, int att)
{
    data1_att *r;
    data1_attset *p;

    if (!(p = data1_attset_search_id (zi->dh, set)))
    {
	zebraExplain_loadAttsets (zi->dh, zi->res);
	p = data1_attset_search_id (zi->dh, set);
    }
    if (!p)
	return -2;
    if (!(r = getatt(p, att)))
	return -1;
    res->attset_ordinal = r->parent->reference;
    res->local_attributes = r->locals;
    return 0;
}
