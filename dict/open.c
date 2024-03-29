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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dict-p.h"

void dict_clean(Dict dict)
{
    int page_size = dict->head.page_size;
    void *head_buf;
    int compact_flag = dict->head.compact_flag;

    memset(dict->head.magic_str, 0, sizeof(dict->head.magic_str));
    strcpy(dict->head.magic_str, DICT_MAGIC);
    dict->head.last = 1;
    dict->head.root = 0;
    dict->head.freelist = 0;
    dict->head.page_size = page_size;
    dict->head.compact_flag = compact_flag;

    /* create header with information (page 0) */
    if (dict->rw)
        dict_bf_newp(dict->dbf, 0, &head_buf, page_size);
}

Dict dict_open(BFiles bfs, const char *name, int cache, int rw,
               int compact_flag, int page_size)
{
    Dict dict;
    void *head_buf;

    dict = (Dict) xmalloc(sizeof(*dict));

    if (cache < 5)
        cache = 5;

    dict->grep_cmap = NULL;
    page_size = DICT_DEFAULT_PAGESIZE;
    if (page_size < 2048)
    {
        yaz_log(YLOG_WARN, "Page size for dict %s %d<2048. Set to 2048",
                name, page_size);
        page_size = 2048;
    }
    dict->dbf = dict_bf_open(bfs, name, page_size, cache, rw);
    dict->rw = rw;
    dict->no_split = 0;
    dict->no_insert = 0;
    dict->no_lookup = 0;

    if(!dict->dbf)
    {
        yaz_log(YLOG_WARN, "Cannot open `%s'", name);
        xfree(dict);
        return NULL;
    }
    if (dict_bf_readp(dict->dbf, 0, &head_buf) <= 0)
    {
        dict->head.page_size = page_size;
        dict->head.compact_flag = compact_flag;
        dict_clean(dict);
    }
    else /* header was there, check magic and page size */
    {
        memcpy(&dict->head, head_buf, sizeof(dict->head));
        if (strcmp(dict->head.magic_str, DICT_MAGIC))
        {
            yaz_log(YLOG_WARN, "Bad magic of `%s'", name);
            dict_bf_close(dict->dbf);
            xfree(dict);
            return 0;
        }
        if (dict->head.page_size != page_size)
        {
            yaz_log(YLOG_WARN, "Page size for existing dict %s is %d. Current is %d",
                    name, dict->head.page_size, page_size);
        }
    }
    if (dict->head.compact_flag)
        dict_bf_compact(dict->dbf);
    return dict;
}

int dict_strcmp(const Dict_char *s1, const Dict_char *s2)
{
    return strcmp((const char *) s1, (const char *) s2);
}

int dict_strncmp(const Dict_char *s1, const Dict_char *s2, size_t n)
{
    return strncmp((const char *) s1, (const char *) s2, n);
}

int dict_strlen(const Dict_char *s)
{
    return strlen((const char *) s);
}

zint dict_get_no_lookup(Dict dict)
{
    return dict->no_lookup;
}

zint dict_get_no_insert(Dict dict)
{
    return dict->no_insert;
}

zint dict_get_no_split(Dict dict)
{
    return dict->no_split;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

