/* $Id: d1_varset.c,v 1.11 2007-04-16 08:44:31 adam Exp $
   Copyright (C) 1995-2007
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <string.h>
#include <stdlib.h>

#include <yaz/oid_db.h>
#include <yaz/log.h>
#include <d1_absyn.h>

data1_vartype *data1_getvartypebyct (data1_handle dh, data1_varset *set,
				     const char *zclass, const char *type)
{
    data1_varclass *c;
    data1_vartype *t;

    for (c = set->classes; c; c = c->next)
	if (!data1_matchstr(c->name, zclass))
	{
	    for (t = c->types; t; t = t->next)
		if (!data1_matchstr(t->name, type))
		    return t;
	    yaz_log(YLOG_WARN, "Unknown variant type %s in class %s",
		    type, zclass);
	    return 0;
	}
    yaz_log(YLOG_WARN, "Unknown variant class %s", zclass);
    return 0;
}

data1_vartype *data1_getvartypeby_absyn (data1_handle dh, data1_absyn *absyn,
					   char *zclass, char *type)
{
    return data1_getvartypebyct(dh, absyn->varset, zclass, type);
}

data1_varset *data1_read_varset (data1_handle dh, const char *file)
{
    NMEM mem = data1_nmem_get (dh);
    data1_varset *res = (data1_varset *)nmem_malloc(mem, sizeof(*res));
    data1_varclass **classp = &res->classes, *zclass = 0;
    data1_vartype **typep = 0;
    FILE *f;
    int lineno = 0;
    int argc;
    char *argv[50],line[512];

    res->name = 0;
    res->oid = 0;
    res->classes = 0;

    if (!(f = data1_path_fopen(dh, file, "r")))
    {
	yaz_log(YLOG_WARN|YLOG_ERRNO, "%s", file);
	return 0;
    }
    while ((argc = readconf_line(f, &lineno, line, 512, argv, 50)))
	if (!strcmp(argv[0], "class"))
	{
	    data1_varclass *r;
	    
	    if (argc != 3)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # or args to class",
			file, lineno);
		continue;
	    }
	    *classp = r = zclass = (data1_varclass *)
		nmem_malloc(mem, sizeof(*r));
	    r->set = res;
	    r->zclass = atoi(argv[1]);
	    r->name = nmem_strdup(mem, argv[2]);
	    r->types = 0;
	    typep = &r->types;
	    r->next = 0;
	    classp = &r->next;
	}
	else if (!strcmp(argv[0], "type"))
	{
	    data1_vartype *r;

	    if (!typep)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Directive class must precede type",
			file, lineno);
		continue;
	    }
	    if (argc != 4)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # or args to type",
			file, lineno);
		continue;
	    }
	    *typep = r = (data1_vartype *)nmem_malloc(mem, sizeof(*r));
	    r->name = nmem_strdup(mem, argv[2]);
	    r->zclass = zclass;
	    r->type = atoi(argv[1]);
	    if (!(r->datatype = data1_maptype (dh, argv[3])))
	    {
		yaz_log(YLOG_WARN, "%s:%d: Unknown datatype '%s'",
			file, lineno, argv[3]);
		fclose(f);
		return 0;
	    }
	    r->next = 0;
	    typep = &r->next;
	}
	else if (!strcmp(argv[0], "name"))
	{
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # args for name",
			file, lineno);
		continue;
	    }
	    res->name = nmem_strdup(mem, argv[1]);
	}
	else if (!strcmp(argv[0], "reference"))
	{
	    if (argc != 2)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Bad # args for reference",
			file, lineno);
		continue;
	    }
            res->oid = yaz_string_to_oid_nmem(yaz_oid_std(),
                                              CLASS_VARSET, argv[1], mem);
	    if (!res->oid)
	    {
		yaz_log(YLOG_WARN, "%s:%d: Unknown reference '%s'",
			file, lineno, argv[1]);
		continue;
	    }
	}
	else 
	    yaz_log(YLOG_WARN, "%s:%d: Unknown directive '%s'",
		    file, lineno, argv[0]);
    
    fclose(f);
    return res;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

