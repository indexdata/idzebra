/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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
#include <stdio.h>
#include <stdlib.h>

#include <yaz/log.h>
#include <idzebra/data1.h>

#define DATA1_FLAG_XML  1

struct data1_handle_info {
    WRBUF wrbuf;
    char *tab_path;
    char *tab_root;

    char *read_buf;
    int read_len;

    data1_absyn_cache absyn_cache;
    data1_attset_cache attset_cache;

    char *map_buf;
    int map_len;

    NMEM mem;
};

data1_handle data1_create(void)
{
    data1_handle p = (data1_handle)xmalloc (sizeof(*p));
    if (!p)
	return 0;
    p->tab_path = NULL;
    p->tab_root = NULL;
    p->wrbuf = wrbuf_alloc();
    p->read_buf = NULL;
    p->read_len = 0;
    p->map_buf = NULL;
    p->map_len = 0;
    p->absyn_cache = NULL;
    p->attset_cache = NULL;
    p->mem = nmem_create ();
    return p;
}

NMEM data1_nmem_get (data1_handle dh)
{
    return dh->mem;
}

data1_absyn_cache *data1_absyn_cache_get (data1_handle dh)
{
    return &dh->absyn_cache;
}

data1_attset_cache *data1_attset_cache_get (data1_handle dh)
{
    return &dh->attset_cache;
}

void data1_destroy (data1_handle dh)
{
    if (!dh)
	return;

    /* *ostrich*
       We need to destroy DFAs, in xp_element (xelm) definitions
       pop, 2002-12-13
    */
    data1_absyn_destroy(dh);

    wrbuf_destroy(dh->wrbuf);
    if (dh->tab_path)
	xfree (dh->tab_path);
    if (dh->tab_root)
	xfree (dh->tab_root);
    if (dh->read_buf)
	xfree (dh->read_buf);
    if (dh->map_buf)
        xfree (dh->map_buf);
    nmem_destroy (dh->mem);

    xfree (dh);
}

WRBUF data1_get_wrbuf (data1_handle dp)
{
    return dp->wrbuf;
}

char **data1_get_read_buf (data1_handle dp, int **lenp)
{
    *lenp = &dp->read_len;
    yaz_log (YLOG_DEBUG, "data1_get_read_buf lenp=%u", **lenp);
    return &dp->read_buf;
}

char **data1_get_map_buf (data1_handle dp, int **lenp)
{
    *lenp = &dp->map_len;
    yaz_log (YLOG_DEBUG, "data1_get_map_buf lenp=%u", **lenp);
    return &dp->map_buf;
}

void data1_set_tabpath (data1_handle dp, const char *p)
{
    xfree (dp->tab_path);
    dp->tab_path = NULL;
    if (p)
        dp->tab_path = xstrdup (p);
}

void data1_set_tabroot (data1_handle dp, const char *p)
{
    xfree (dp->tab_root);
    dp->tab_root = NULL;
    if (p)
        dp->tab_root = xstrdup (p);
}

const char *data1_get_tabpath (data1_handle dp)
{
    return dp->tab_path;
}

const char *data1_get_tabroot (data1_handle dp)
{
    return dp->tab_root;
}

FILE *data1_path_fopen (data1_handle dh, const char *file, const char *mode)
{
    FILE *f;
    const char *path = data1_get_tabpath(dh);
    const char *root = data1_get_tabroot(dh);

    yaz_log(YLOG_DEBUG, "data1_path_fopen path=%s root=%s "
            "file=%s mode=%s", path ? path : "NULL",
            root ? root : "NULL", file, mode);
    if (!path || !*path)
	return 0;
    f = yaz_fopen(path, file, mode, root);
    if (!f)
    {
	yaz_log(YLOG_WARN|YLOG_ERRNO, "Couldn't open %s", file);
        if (root)
            yaz_log(YLOG_LOG, "for root=%s", root);
        if (path)
            yaz_log(YLOG_LOG, "for profilePath=%s", path);
    }
    return f;
}

int data1_is_xmlmode(data1_handle dh)
{
    return 1;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

