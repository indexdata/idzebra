/* $Id: d1_attset.c,v 1.2 2002-10-22 13:19:50 adam Exp $
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
#include <stdlib.h>

#include <yaz/log.h>
#include <d1_attset.h>
#include <data1.h>

data1_att *data1_getattbyname(data1_handle dh, data1_attset *s, char *name)
{
    data1_att *r;
    data1_attset_child *c;
    
    /* scan local set */
    for (r = s->atts; r; r = r->next)
	if (!data1_matchstr(r->name, name))
	    return r;
    for (c = s->children; c; c = c->next)
    {
	assert (c->child);
	/* scan included sets */
	if ((r = data1_getattbyname (dh, c->child, name)))
	    return r;
    }
    return 0;
}

data1_attset *data1_empty_attset(data1_handle dh)
{
    NMEM mem = data1_nmem_get (dh);
    data1_attset *res = (data1_attset*) nmem_malloc(mem,sizeof(*res));

    res->name = 0;
    res->reference = VAL_NONE;
    res->atts = 0;
    res->children = 0;
    res->next = 0;
    return res;
}

data1_attset *data1_read_attset(data1_handle dh, const char *file)
{
    data1_attset *res = 0;
    data1_attset_child **childp;
    data1_att **attp;
    FILE *f;
    NMEM mem = data1_nmem_get (dh);
    int lineno = 0;
    int argc;
    char *argv[50], line[512];

    if (!(f = data1_path_fopen(dh, file, "r")))
	return NULL;
    res = data1_empty_attset (dh);

    childp = &res->children;
    attp = &res->atts;

    while ((argc = readconf_line(f, &lineno, line, 512, argv, 50)))
    {
	char *cmd = argv[0];
	if (!strcmp(cmd, "att"))
	{
	    int num;
	    char *name;
	    data1_att *t;
	    data1_local_attribute *locals;
	    
	    if (argc < 3)
	    {
		yaz_log(LOG_WARN, "%s:%d: Bad # of args to att", file, lineno);
		continue;
	    }
	    num = atoi (argv[1]);
	    name = argv[2];
	    
	    if (argc == 3) /* no local attributes given */
	    {
		locals = (data1_local_attribute *)
		    nmem_malloc(mem, sizeof(*locals));
		locals->local = num;
		locals->next = 0;
	    }
	    else /* parse the string "local{,local}" */
	    {
		char *p = argv[3];
		data1_local_attribute **ap = &locals;
		do
		{
		    *ap = (data1_local_attribute *)
			nmem_malloc(mem, sizeof(**ap));
		    (*ap)->local = atoi(p);
		    (*ap)->next = 0;
		    ap = &(*ap)->next;
		}
		while ((p = strchr(p, ',')) && *(++p));
	    }
	    t = *attp = (data1_att *)nmem_malloc(mem, sizeof(*t));
	    t->parent = res;
	    t->name = nmem_strdup(mem, name);
	    t->value = num;
	    t->locals = locals;
	    t->next = 0;
	    attp = &t->next;
	}
	else if (!strcmp(cmd, "name"))
	{
	    if (argc != 2)
	    {
		yaz_log(LOG_WARN, "%s:%d: Bad # of args to name", file, lineno);
		continue;
	    }
	}
	else if (!strcmp(cmd, "reference"))
	{
	    char *name;

	    if (argc != 2)
	    {
		yaz_log(LOG_WARN, "%s:%d: Bad # of args to reference",
			file, lineno);
		continue;
	    }
	    name = argv[1];
	    if ((res->reference = oid_getvalbyname(name)) == VAL_NONE)
	    {
		yaz_log(LOG_WARN, "%s:%d: Unknown reference oid '%s'",
			file, lineno, name);
		fclose(f);
		return 0;
	    }
	}
	else if (!strcmp(cmd, "ordinal"))
	{
	    yaz_log (LOG_WARN, "%s:%d: Directive ordinal ignored",
		     file, lineno);
	}
	else if (!strcmp(cmd, "include"))
	{
	    char *name;
	    data1_attset *attset;

	    if (argc != 2)
	    {
		yaz_log(LOG_WARN, "%s:%d: Bad # of args to include",
			file, lineno);
		continue;
	    }
	    name = argv[1];

	    if (!(attset = data1_get_attset (dh, name)))
	    {
		yaz_log(LOG_WARN, "%s:%d: Include of attset %s failed",
			file, lineno, name);
		continue;
		
	    }
	    *childp = (data1_attset_child *)
		nmem_malloc (mem, sizeof(**childp));
	    (*childp)->child = attset;
	    (*childp)->next = 0;
	    childp = &(*childp)->next;
	}
	else
	{
	    yaz_log(LOG_WARN, "%s:%d: Unknown directive '%s'",
		    file, lineno, cmd);
	}
    }
    fclose(f);
    return res;
}
