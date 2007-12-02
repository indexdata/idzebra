/* $Id: strmap.c,v 1.1 2007-12-02 11:30:28 adam Exp $
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

#include <stddef.h>
#include <string.h>
#include <zebra_strmap.h>
#include <yaz/nmem.h>

struct strmap_entry {
    char *name;
    size_t data_len;
    void *data_buf;
    struct strmap_entry *next;
};
    
struct zebra_strmap {
    NMEM nmem_str;
    NMEM nmem_ent;
    int hsize;
    struct strmap_entry **entries;
    struct strmap_entry *free_entries;
};

zebra_strmap_t zebra_strmap_create(void)
{
    int i;
    NMEM nmem_ent = nmem_create();
    zebra_strmap_t st = nmem_malloc(nmem_ent, sizeof(*st));
    st->nmem_ent = nmem_ent;
    st->nmem_str = nmem_create();
    st->hsize = 1001;
    st->free_entries = 0;
    st->entries = nmem_malloc(nmem_ent, st->hsize * sizeof(*st->entries));
    for (i = 0; i < st->hsize; i++)
        st->entries[i] = 0;
    return st;
}

void zebra_strmap_destroy(zebra_strmap_t st)
{
    if (st)
    {
        nmem_destroy(st->nmem_str);
        nmem_destroy(st->nmem_ent);
    }
}

static struct strmap_entry **hash(zebra_strmap_t st, const char *name)
{
    unsigned hash = 0;
    int i;
    for (i = 0; name[i]; i++)
        hash += hash*65519 + name[i];
    hash = hash % st->hsize;
    return st->entries + hash;
}

void zebra_strmap_add(zebra_strmap_t st, const char *name,
                      void *data_buf, size_t data_len)
{
    struct strmap_entry **e = hash(st, name);
    struct strmap_entry *ne = st->free_entries;

    if (ne)
        st->free_entries = ne->next;
    else
        ne = nmem_malloc(st->nmem_ent, sizeof(*ne));
    ne->next = *e;
    *e = ne;
    ne->name = nmem_strdup(st->nmem_str, name);
    ne->data_buf = nmem_malloc(st->nmem_str, data_len);
    memcpy(ne->data_buf, data_buf, data_len);
    ne->data_len = data_len;
}

void *zebra_strmap_lookup(zebra_strmap_t st, const char *name, int no,
                          size_t *data_len)
{
    struct strmap_entry *e = *hash(st, name);
    int i = 0;
    for (; e ; e = e->next)
        if (!strcmp(name, e->name))
        {
            if (i == no)
            {
                if (data_len)
                    *data_len = e->data_len;
                return e->data_buf;
            }
            i++;
        }
    return 0;
}

int zebra_strmap_remove(zebra_strmap_t st, const char *name)
{
    struct strmap_entry **e = hash(st, name);
    for (; *e ; e = &(*e)->next)
        if (!strcmp(name, (*e)->name))
        {
            struct strmap_entry *tmp = *e;
            *e = (*e)->next;

            tmp->next = st->free_entries;
            st->free_entries = tmp;
            return 1;
        }
    return 0;
}
			 
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

