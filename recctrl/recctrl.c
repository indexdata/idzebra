/* $Id: recctrl.c,v 1.6 2002-08-02 19:26:56 adam Exp $
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/


#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <zebrautl.h>
#include "rectext.h"
#include "recgrs.h"

struct recTypeEntry {
    RecType recType;
    struct recTypeEntry *next;
    int init_flag;
    void *clientData;
};

struct recTypes {
    data1_handle dh;
    struct recTypeEntry *entries;
};

RecTypes recTypes_init (data1_handle dh)
{
    RecTypes p = (RecTypes) nmem_malloc (data1_nmem_get (dh), sizeof(*p));

    p->dh = dh;
    p->entries = 0;
    return p;
}

void recTypes_destroy (RecTypes rts)
{
    struct recTypeEntry *rte;

    for (rte = rts->entries; rte; rte = rte->next)
	if (rte->init_flag)
	    (*(rte->recType)->destroy)(rte->clientData);
}

void recTypes_add_handler (RecTypes rts, RecType rt)
{
    struct recTypeEntry *rte;

    rte = (struct recTypeEntry *)
	nmem_malloc (data1_nmem_get (rts->dh), sizeof(*rte));

    rte->recType = rt;
    rte->init_flag = 0;
    rte->clientData = 0;
    rte->next = rts->entries;
    rts->entries = rte;
}

RecType recType_byName (RecTypes rts, const char *name, char *subType,
			void **clientDataP)
{
    struct recTypeEntry *rte;
    char *p;
    char tmpname[256];

    strcpy (tmpname, name);
    if ((p = strchr (tmpname, '.')))
    {
        *p = '\0';
        strcpy (subType, p+1);
    }
    else
        *subType = '\0';
    for (rte = rts->entries; rte; rte = rte->next)
	if (!strcmp (rte->recType->name, tmpname))
	{
	    if (!rte->init_flag)
	    {
		rte->init_flag = 1;
		rte->clientData =
		    (*(rte->recType)->init)(rte->recType);
	    }
	    *clientDataP = rte->clientData;
	    return rte->recType;
	}
    return 0;
}

void recTypes_default_handlers (RecTypes rts)
{
    recTypes_add_handler (rts, recTypeGrs);
    recTypes_add_handler (rts, recTypeText);
}
