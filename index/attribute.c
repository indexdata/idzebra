/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: attribute.c,v $
 * Revision 1.8  1998-03-05 08:45:11  adam
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

#include <log.h>
#include <res.h>
#include <zebrautl.h>
#include "zserver.h"

static void att_loadset(void *p, const char *n, const char *name)
{
    data1_attset *cnew;
    ZebraHandle zi = p;

    if (!(cnew = data1_read_attset(zi->dh, (char*) name)))
    {
	logf(LOG_WARN|LOG_ERRNO, "%s", name);
	return;
    }
    cnew->next = zi->registered_sets;
    zi->registered_sets = cnew;
}

static void load_atts(ZebraHandle zi)
{
    res_trav(zi->res, "attset", zi, att_loadset);
}

static data1_att *getatt(data1_attset *p, int att)
{
    data1_att *a;

    for (; p; p = p->next)
    {
	/* scan local set */
	for (a = p->atts; a; a = a->next)
	    if (a->value == att)
		return a;
	/* scan included sets */
	if (p->children && (a = getatt(p->children, att)))
	    return a;
    }
    return 0;
}

int att_getentbyatt(ZebraHandle zi, attent *res, oid_value set, int att)
{
    data1_att *r;
    data1_attset *p;

    if (!zi->registered_sets)
	load_atts(zi);
    for (p = zi->registered_sets; p; p = p->next)
	if (p->reference == set)
	    break;
    if (!p)
	return -2;
    if (!(r = getatt(p, att)))
	return -1;
    res->attset_ordinal = r->parent->ordinal;
    res->local_attributes = r->locals;
    return 0;
}
