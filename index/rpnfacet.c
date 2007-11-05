/* $Id: rpnfacet.c,v 1.3 2007-11-05 11:20:39 adam Exp $
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

#include <stdio.h>
#include <assert.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#include <yaz/diagbib1.h>
#include "index.h"
#include <zebra_xpath.h>
#include <yaz/wrbuf.h>
#include <attrfind.h>
#include <charmap.h>
#include <rset.h>
#include <yaz/oid_db.h>

ZEBRA_RES rpn_facet(ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
                    const Odr_oid *attributeset,
                    int *position, int *num_entries, 
                    ZebraScanEntry **list, int *is_partial, 
                    const char *set_name)
{
    int ord;
    int use_sort_idx = 1;
    ZEBRA_RES res = zebra_attr_list_get_ord(zh,
                                            zapt->attributes,
                                            zinfo_index_category_sort,
                                            0 /* index_type */,
                                            attributeset, &ord);
    if (res != ZEBRA_OK)
        return res;
    else if (use_sort_idx)
    {
        const char *index_type = 0;
        const char *db = 0;
        const char *string_index = 0;
        /* for each ord .. */
        /*   check that sort idx exist for ord */
        /*   sweep through result set and sort_idx at the same time */
        char *this_entry_buf = xmalloc(SORT_IDX_ENTRYSIZE);
        char *dst_buf = xmalloc(SORT_IDX_ENTRYSIZE);
        size_t sysno_mem_index = 0;
        RSET rset = resultSetRef(zh, set_name);
        zint p_this_sys = 0;
        RSFD rfd;
        TERMID termid;
        struct it_key key;

        if (zebraExplain_lookup_ord(zh->reg->zei,
                                    ord, &index_type, &db, &string_index))
        {
            yaz_log(YLOG_WARN, "zebraExplain_lookup_ord failed");
        }
        
        if (zh->m_staticrank)
            sysno_mem_index = 1;
        
        rfd = rset_open(rset, RSETF_READ);
        while (rset_read(rfd, &key, &termid))
        {
            zint sysno = key.mem[sysno_mem_index];
            if (sysno != p_this_sys)
            {
                p_this_sys = sysno;
                zebra_sort_sysno(zh->reg->sort_index, sysno);
                zebra_sort_type(zh->reg->sort_index, ord);
                zebra_sort_read(zh->reg->sort_index, this_entry_buf);

                zebra_term_untrans(zh, index_type, dst_buf, this_entry_buf);
                yaz_log(YLOG_LOG, "dst_buf=%s", dst_buf);
            }
        }
        rset_close(rfd);
        xfree(this_entry_buf);
        xfree(dst_buf);
        zebra_setError(zh, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, "facet not done1");
        return ZEBRA_FAIL;
    }
    else
    {
        int num = 100; /* to be customizable */
        int i;

        ZebraMetaRecord *meta = zebra_meta_records_create_range(
            zh, set_name, 0, num);

        for (i = 0; i < num; i++)
        {
            zint sysno = meta[i].sysno;
            Record rec = rec_get(zh->reg->records, sysno);
            if (!rec)
            {
                yaz_log(YLOG_WARN, "rec_get fail on sysno=" ZINT_FORMAT,
                        sysno);
                break;
            }
            else
            {
                

                rec_free(&rec);
            }
        }
        zebra_meta_records_destroy(zh, meta, num);
        zebra_setError(zh, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, "facet not done2");
        return ZEBRA_FAIL;
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

