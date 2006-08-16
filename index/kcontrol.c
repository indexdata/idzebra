/* $Id: kcontrol.c,v 1.5 2006-08-16 13:16:36 adam Exp $
   Copyright (C) 1995-2006
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

#include <assert.h>
#include "index.h"

struct context_control {
    int ref_count;
    void (*filter_destroy)(void *data);
};

static void my_inc(struct rset_key_control *kc)
{
    struct context_control *cp;

    assert(kc);
    cp = kc->context;
    (cp->ref_count)++;
}

static void my_dec(struct rset_key_control *kc)
{
    struct context_control *cp;

    assert(kc);
    cp = kc->context;
    (cp->ref_count)--;
    if (cp->ref_count == 0)
    {
	if (cp->filter_destroy)
	    (*cp->filter_destroy)(kc->filter_data);
	xfree(cp);
	xfree(kc);
    }
}


struct rset_key_control *zebra_key_control_create(ZebraHandle zh)
{
    const char *res_val;
    struct rset_key_control *kc = xmalloc(sizeof(*kc));
    struct context_control *cp = xmalloc(sizeof(*cp));

    kc->context = cp;
    kc->key_size = sizeof(struct it_key);
    kc->cmp = key_compare_it;
    kc->key_logdump_txt = key_logdump_txt;
    kc->getseq = key_get_seq;

    if (zh->m_segment_indexing)
    {
        kc->scope = 3;  /* segment + seq is "same" record */
        kc->get_segment = key_get_segment;
    }
    else
    {
        kc->scope = 2;  /* seq is "same" record */
        kc->get_segment = 0;
    }

    zebra_limit_for_rset(zh->m_limit, 
			 &kc->filter_func,
			 &cp->filter_destroy,
			 &kc->filter_data);
    kc->inc = my_inc;
    kc->dec = my_dec;
    cp->ref_count = 1;
    return kc;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

