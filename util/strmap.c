/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stddef.h>
#include <string.h>
#include <zebra_strmap.h>
#include <yaz/nmem.h>
#include <yaz/xmalloc.h>

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
    int size;
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
    st->size = 0;
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
    (st->size)++;
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

            --(st->size);
            return 1;
        }
    return 0;
}

int zebra_strmap_get_size(zebra_strmap_t st)
{
    return st->size;
}

struct zebra_strmap_it_s {
    int hno;
    struct strmap_entry *ent;
    zebra_strmap_t st;

};

zebra_strmap_it zebra_strmap_it_create(zebra_strmap_t st)
{
    zebra_strmap_it it = (zebra_strmap_it) xmalloc(sizeof(*it));
    it->hno = 0;
    it->ent = 0;
    it->st = st;
    return it;
}

void zebra_strmap_it_destroy(zebra_strmap_it it)
{
    xfree(it);
}

const char *zebra_strmap_it_next(zebra_strmap_it it, void **data_buf,
                                 size_t *data_len)
{
    struct strmap_entry *ent = 0;
    while (!it->ent && it->hno < it->st->hsize)
    {
        it->ent = it->st->entries[it->hno];
        it->hno++;
    }
    if (it->ent)
    {
        ent = it->ent;
        it->ent = ent->next;
    }
    if (ent)
    {
        if (data_buf)
            *data_buf = ent->data_buf;
        if (data_len)
            *data_len = ent->data_len;
        return ent->name;
    }
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

