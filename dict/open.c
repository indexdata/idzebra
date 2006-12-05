/* $Id: open.c,v 1.19.2.2 2006-12-05 21:14:40 adam Exp $
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <yaz/xmalloc.h>
#include <dict.h>

Dict dict_open (BFiles bfs, const char *name, int cache, int rw,
		int compact_flag)
{
    Dict dict;
    void *head_buf;
    char resource_str[80];
    int page_size;

    dict = (Dict) xmalloc (sizeof(*dict));

    if (cache < 5)
	cache = 5;
    sprintf (resource_str, "dict.%s.pagesize", name);

    dict->grep_cmap = NULL;
    page_size = DICT_DEFAULT_PAGESIZE;
    if (page_size < 2048)
    {
        yaz_log(YLOG_WARN, "Resource %s was too small. Set to 2048",
              resource_str);
        page_size = 2048;
    }
    dict->dbf = dict_bf_open (bfs, name, page_size, cache, rw);
    dict->rw = rw;

    if(!dict->dbf)
    {
        yaz_log(YLOG_WARN, "Cannot open `%s'", name);
        xfree (dict);
        return NULL;
    }
    if (dict_bf_readp (dict->dbf, 0, &head_buf) <= 0)
    {
	memset (dict->head.magic_str, 0, sizeof(dict->head.magic_str));
	strcpy (dict->head.magic_str, DICT_MAGIC);
	dict->head.last = 1;
	dict->head.root = 0;
	dict->head.freelist = 0;
	dict->head.page_size = page_size;
	dict->head.compact_flag = compact_flag;
	
	/* create header with information (page 0) */
        if (rw) 
            dict_bf_newp (dict->dbf, 0, &head_buf, page_size);
    }
    else /* header was there, check magic and page size */
    {
	memcpy (&dict->head, head_buf, sizeof(dict->head));
        if (strcmp (dict->head.magic_str, DICT_MAGIC))
        {
            yaz_log(YLOG_WARN, "Bad magic of `%s'", name);
            exit (1);
        }
        if (dict->head.page_size != page_size)
        {
            yaz_log(YLOG_WARN, "Resource %s is %d and pagesize of `%s' is %d",
                  resource_str, page_size, name, dict->head.page_size);
	    return 0;
        }
    }
    if (dict->head.compact_flag)
	dict_bf_compact(dict->dbf);
    return dict;
}

int dict_strcmp (const Dict_char *s1, const Dict_char *s2)
{
    return strcmp ((const char *) s1, (const char *) s2);
}

int dict_strncmp (const Dict_char *s1, const Dict_char *s2, size_t n)
{
    return strncmp ((const char *) s1, (const char *) s2, n);
}

int dict_strlen (const Dict_char *s)
{
    return strlen((const char *) s);
}
