/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: attribute.c,v $
 * Revision 1.4  1996-10-29 14:06:48  adam
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
#include <d1_attset.h>
#include <zebrautl.h>
#include "attribute.h"

static int initialized = 0;

static data1_attset *registered_sets = 0;

static void att_loadset(const char *n, const char *name)
{
    data1_attset *new;

    if (!(new = data1_read_attset((char*) name)))
    {
	logf(LOG_WARN|LOG_ERRNO, "%s", name);
	return;
    }
    new->next = registered_sets;
    registered_sets = new;
    return;
}

static void load_atts()
{
    res_trav(common_resource, "attset", att_loadset);
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

attent *att_getentbyatt(oid_value set, int att)
{
    static attent res;
    data1_att *r;
    data1_attset *p;

    if (!initialized)
    {
	initialized = 1;
	load_atts();
    }
    for (p = registered_sets; p; p = p->next)
	if (p->reference == set && (r = getatt(p, att)))
	    break;;
    if (!p)
	return 0;
    res.attset_ordinal = r->parent->ordinal;
    res.local_attributes = r->locals;
    return &res;
}
