/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: symtab.c,v $
 * Revision 1.1  1995-09-06 16:11:18  adam
 * Option: only one word key per file.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <alexutil.h>
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
    struct strtab *p = xmalloc (sizeof (*p));
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
    e = xmalloc (sizeof(*e));
    e->name = xmalloc (strlen(name)+1);
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
            free (e->name);
            free (e);
        }
    free (t);
}
