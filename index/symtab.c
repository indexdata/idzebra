/* $Id: symtab.c,v 1.7.2.1 2006-08-14 10:38:59 adam Exp $
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "index.h"

struct strentry {
    char *name;
    void *info;
    struct strentry *next;
};

#define STR_HASH 401

struct strtab {
    struct strentry *ar[STR_HASH];
};

struct strtab *strtab_mk (void)
{
    int i;
    struct strtab *p = (struct strtab *) xmalloc (sizeof (*p));
    for (i=0; i<STR_HASH; i++)
        p->ar[i] = NULL;
    return p;
}

int strtab_src (struct strtab *t, const char *name, void ***infop)
{
    unsigned hash = 0;
    int i;
    struct strentry *e;

    for (i=0; name[i]; i++)
        hash += hash*65519 + name[i];
    hash = hash % STR_HASH;
    for (e = t->ar[hash]; e; e = e->next)
        if (!strcmp(e->name, name))
        {
            *infop = &e->info;
            return 1;
        }
    e = (struct strentry *) xmalloc (sizeof(*e));
    e->name = (char *) xmalloc (strlen(name)+1);
    strcpy (e->name, name);
    e->next = t->ar[hash];
    t->ar[hash] = e;
    *infop = &e->info;
    return 0;
}

void strtab_del (struct strtab *t,
                 void (*func)(const char *name, void *info, void *data),
                 void *data)
{
    int i;
    struct strentry *e, *e1;

    for (i = 0; i<STR_HASH; i++)
        for (e = t->ar[i]; e; e = e1)
        {
            e1 = e->next;
            (*func)(e->name, e->info, data);
            xfree (e->name);
            xfree (e);
        }
    xfree (t);
}
