/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recctrl.c,v $
 * Revision 1.5  1999-05-26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.4  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.3  1998/10/16 08:14:36  adam
 * Updated record control system.
 *
 * Revision 1.2  1996/10/29 14:03:16  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.1  1996/10/11 10:57:24  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 * Revision 1.5  1996/06/04 10:18:59  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.4  1995/12/04  17:59:24  adam
 * More work on regular expression conversion.
 *
 * Revision 1.3  1995/12/04  14:22:30  adam
 * Extra arg to recType_byName.
 * Started work on new regular expression parsed input to
 * structured records.
 *
 * Revision 1.2  1995/11/15  14:46:19  adam
 * Started work on better record management system.
 *
 * Revision 1.1  1995/09/27  12:22:28  adam
 * More work on extract in record control.
 * Field name is not in isam keys but in prefix in dictionary words.
 *
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
