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
#include <stdio.h>
#include <assert.h>

#include <yaz/xmalloc.h>
#include <yaz/diagbib1.h>
#include "index.h"

#define ZEBRA_LIMIT_DEBUG 0

struct zebra_limit {
    int complement_flag;
    zint *ids;
};

void zebra_limit_destroy(struct zebra_limit *zl)
{
    if (zl)
    {
        xfree(zl->ids);
        xfree(zl);
    }
}

struct zebra_limit *zebra_limit_create(int complement_flag, zint *ids)
{
    struct zebra_limit *zl = 0;
    size_t i;
    if (ids)
    {
        for (i = 0; ids[i]; i++)
            ;
        zl = xmalloc(sizeof(*zl));
        zl->ids = xmalloc((i+1) * sizeof(*ids));
        memcpy(zl->ids, ids, (i+1) * sizeof(*ids));
        zl->complement_flag = complement_flag;
    }
    return zl;
}

static int zebra_limit_filter_cb(const void *buf, void *data)
{
    struct zebra_limit *zl = data;
    const struct it_key *key = buf;
    size_t i;

#if ZEBRA_LIMIT_DEBUG
    yaz_log(YLOG_LOG, "zebra_limit_filter_cb zl=%p key->len=%d", zl, key->len);
#endif
    for (i = 0; zl->ids[i]; i++)
    {
#if ZEBRA_LIMIT_DEBUG
        yaz_log(YLOG_LOG, " i=%d ids=" ZINT_FORMAT " mem=" ZINT_FORMAT,
                i, zl->ids[i], key->mem[1]);
#endif
        if (zl->ids[i] == key->mem[1])
        {
#if ZEBRA_LIMIT_DEBUG
            yaz_log(YLOG_LOG, " match. Ret=%d", zl->complement_flag ? 0:1);
#endif
            return zl->complement_flag ? 0 : 1;
        }
    }
#if ZEBRA_LIMIT_DEBUG
    yaz_log(YLOG_LOG, " no match. Ret=%d", zl->complement_flag ? 1:0);
#endif
    return zl->complement_flag ? 1 : 0;
}

static void zebra_limit_destroy_cb(void *data)
{
    zebra_limit_destroy(data);
}

void zebra_limit_for_rset(struct zebra_limit *zl,
                          int (**filter_func)(const void *buf, void *data),
                          void (**filter_destroy)(void *data),
                          void **filter_data)
{
#if ZEBRA_LIMIT_DEBUG
    yaz_log(YLOG_LOG, "zebra_limit_for_rset debug enabled zl=%p", zl);
#endif
    if (zl && zl->ids)
    {
        struct zebra_limit *hl;

#if ZEBRA_LIMIT_DEBUG
        yaz_log(YLOG_LOG, "enable limit");
#endif
        hl = zebra_limit_create(zl->complement_flag, zl->ids);
        *filter_data = hl;
        *filter_func = zebra_limit_filter_cb;
        *filter_destroy = zebra_limit_destroy_cb;
    }
    else
    {
        *filter_data = 0;
        *filter_func = 0;
        *filter_destroy = 0;
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

