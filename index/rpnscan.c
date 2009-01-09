/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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
#ifdef WIN32
#include <io.h>
#endif
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

#define RPN_MAX_ORDS 32

/* convert APT SCAN term to internal cmap */
static ZEBRA_RES trans_scan_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
				 char *termz, zebra_map_t zm)
{
    char term_utf8[IT_MAX_WORD];

    if (zapt_term_to_utf8(zh, zapt, term_utf8) == ZEBRA_FAIL)
        return ZEBRA_FAIL;    /* error */
    else if (zebra_maps_is_icu(zm))
    {
        const char *res_buf;
        size_t res_len;
        zebra_map_tokenize_start(zm, term_utf8, strlen(term_utf8));
        
        if (zebra_map_tokenize_next(zm, &res_buf, &res_len, 0, 0))
        {
            memcpy(termz, res_buf, res_len);
            termz[res_len] = '\0';
        }
        else
            termz[0] = '\0';
    }
    else
    {
        const char **map;
        const char *cp = (const char *) term_utf8;
        const char *cp_end = cp + strlen(cp);
        const char *src;
        int i = 0;
        const char *space_map = NULL;
        int len;
            
        while ((len = (cp_end - cp)) > 0)
        {
            map = zebra_maps_input(zm, &cp, len, 0);
            if (**map == *CHR_SPACE)
                space_map = *map;
            else
            {
                if (i && space_map)
                    for (src = space_map; *src; src++)
                        termz[i++] = *src;
                space_map = NULL;
                for (src = *map; *src; src++)
                    termz[i++] = *src;
            }
        }
        termz[i] = '\0';
    }
    return ZEBRA_OK;
}

static void get_first_snippet_from_rset(ZebraHandle zh, 
                                        RSET rset, zebra_snippets *snippets, 
                                        zint *sysno)
{
    struct it_key key;
    RSFD rfd;
    TERMID termid;
    size_t sysno_mem_index = 0;

    if (zh->m_staticrank)
	sysno_mem_index = 1;

    yaz_log(YLOG_DEBUG, "get_first_snippet_from_rset");

    rfd = rset_open(rset, RSETF_READ);
    *sysno = 0;
    while (rset_read(rfd, &key, &termid))
    {
        if (key.mem[sysno_mem_index] != *sysno)
        {
            if (*sysno)
                break;
            *sysno = key.mem[sysno_mem_index];
        }
        if (termid)
        {
            struct ord_list *ol;
            for (ol = termid->ol; ol; ol = ol->next)
            {
                zebra_snippets_append(snippets, key.mem[key.len-1], 0,
                                      ol->ord, termid->name);
            }
        }
    }
    rset_close(rfd);
}

struct scan2_info_entry {
    WRBUF term;
    char prefix[20];
    ISAM_P isam_p;
    int pos_to_save;
    int ord;
};

static int scan_handle2(char *name, const char *info, int pos, void *client)
{
    int len_prefix;
    struct scan2_info_entry *scan_info = (struct scan2_info_entry *) client;

    if (scan_info->pos_to_save != pos)
        return 0;

    len_prefix = strlen(scan_info->prefix);
    if (memcmp(name, scan_info->prefix, len_prefix))
        return 1;

    /* skip special terms such as first-in-field specials */
    if (name[len_prefix] < CHR_BASE_CHAR)
        return 1;

    wrbuf_rewind(scan_info->term);
    wrbuf_puts(scan_info->term, name+len_prefix);

    assert(*info == sizeof(ISAM_P));
    memcpy(&scan_info->isam_p, info+1, sizeof(ISAM_P));
    return 0;
}


static int scan_save_set(ZebraHandle zh, ODR stream, NMEM nmem,
                         struct rset_key_control *kc,
                         Z_AttributesPlusTerm *zapt,
                         RSET limit_set,
                         const char *term, 
                         const char *index_type,
                         struct scan2_info_entry *ar, int ord_no,
                         ZebraScanEntry *glist, int pos)
{
    int i;
    RSET rset = 0;
    zint approx_limit = zh->approx_limit;
    AttrType global_hits_limit_attr;
    int l;
    attr_init_APT(&global_hits_limit_attr, zapt, 12);
            
    l = attr_find(&global_hits_limit_attr, NULL);
    if (l != -1)
        approx_limit = l;
    
    for (i = 0; i < ord_no; i++)
    {
        if (ar[i].isam_p && strcmp(wrbuf_cstr(ar[i].term), term) == 0)
        {
            struct ord_list *ol = ord_list_create(nmem);
            RSET rset_t;

            ol = ord_list_append(nmem, ol, ar[i].ord);

            assert(ol);
            rset_t = rset_trunc(
                    zh, &ar[i].isam_p, 1,
                    wrbuf_buf(ar[i].term), wrbuf_len(ar[i].term),
                    NULL, 1, zapt->term->which, nmem, 
                    kc, kc->scope, ol, index_type, 
                    0 /* hits_limit_value */,
                    0 /* term_ref_id_str */);
            if (!rset)
                rset = rset_t;
            else
            {
                RSET rsets[2];
                
                rsets[0] = rset;
                rsets[1] = rset_t;
                rset = rset_create_or(nmem, kc, kc->scope, 0 /* termid */,
                                      2, rsets);
            }
            ar[i].isam_p = 0;
        }
    }
    if (rset)
    {
        zint count;
        /* merge with limit_set if given */
        if (limit_set)
        {
            RSET rsets[2];
            rsets[0] = rset;
            rsets[1] = rset_dup(limit_set);
            
            rset = rset_create_and(nmem, kc, kc->scope, 2, rsets);
        }
        /* count it */
        zebra_count_set(zh, rset, &count, approx_limit);

        if (pos != -1)
        {
            zint sysno;
            zebra_snippets *hit_snippets = zebra_snippets_create();

            glist[pos].term = 0;
            glist[pos].display_term = 0;
            
            get_first_snippet_from_rset(zh, rset, hit_snippets, &sysno);
            if (sysno)
            {
                zebra_snippets *rec_snippets = zebra_snippets_create();
                int code = zebra_get_rec_snippets(zh, sysno, rec_snippets);
                if (code == 0)
                {
                    const struct zebra_snippet_word *w = 
                        zebra_snippets_lookup(rec_snippets, hit_snippets);
                    if (w)
                    {
                        glist[pos].display_term = odr_strdup(stream, w->term);
                    }
                    else
                    {
                        yaz_log(YLOG_WARN, "zebra_snippets_lookup failed for pos=%d", pos);
                    }
                }
                zebra_snippets_destroy(rec_snippets);
            }
            if (zebra_term_untrans_iconv(zh, stream->mem, index_type,
                                         &glist[pos].term, term))
            {
                /* failed.. use display_term instead (which could be 0) */
                glist[pos].term = glist[pos].display_term;
            }

            if (!glist[pos].term)
            {
                yaz_log(YLOG_WARN, "Could not generate scan term for pos=%d",
                        pos);
                glist[pos].term = "None";
            }
            glist[pos].occurrences = count;
            zebra_snippets_destroy(hit_snippets);
        }
        rset_delete(rset);
        if (count > 0)
            return 1;
        else
            return 0;
    }
    return 0;
}

static ZEBRA_RES rpn_scan_norm(ZebraHandle zh, ODR stream, NMEM nmem,
                               struct rset_key_control *kc,
                               Z_AttributesPlusTerm *zapt,
                               int *position, int *num_entries, 
                               ZebraScanEntry **list,
                               int *is_partial, RSET limit_set,
                               const char *index_type,
                               int ord_no, int *ords)
{
    struct scan2_info_entry *ar = nmem_malloc(nmem, sizeof(*ar) * ord_no);
    struct rpn_char_map_info rcmi;
    zebra_map_t zm = zebra_map_get_or_add(zh->reg->zebra_maps, index_type);
    int i, dif;
    int after_pos;
    int pos = 0;

    ZebraScanEntry *glist = (ZebraScanEntry *)
        odr_malloc(stream, *num_entries * sizeof(*glist));

    *is_partial = 0;
    if (*position > *num_entries+1)
    {
        *is_partial = 1;
        *position = 1;
        *num_entries = 0;
        return ZEBRA_OK;
    }
    rpn_char_map_prepare(zh->reg, zm, &rcmi);

    for (i = 0; i < ord_no; i++)
	ar[i].term = wrbuf_alloc();

    for (i = 0; i < ord_no; i++)
    {
        char termz[IT_MAX_WORD+20];
        int prefix_len = 0;
        
        prefix_len = key_SU_encode(ords[i], termz);
        termz[prefix_len] = 0;
        strcpy(ar[i].prefix, termz);
        
        if (trans_scan_term(zh, zapt, termz+prefix_len, zm) == 
            ZEBRA_FAIL)
        {
            for (i = 0; i < ord_no; i++)
                wrbuf_destroy(ar[i].term);
            return ZEBRA_FAIL;
        }
        wrbuf_rewind(ar[i].term);
        wrbuf_puts(ar[i].term, termz + prefix_len);
        ar[i].isam_p = 0;
        ar[i].ord = ords[i];
    }
    /** deal with terms before position .. */
    /* the glist index starts at zero (unlike scan positions */
    for (pos = *position-2; pos >= 0; )
    {
        const char *hi = 0;

        /* scan on all maximum terms */
        for (i = 0; i < ord_no; i++)
        {
            if (ar[i].isam_p == 0)
            {
                char termz[IT_MAX_WORD+20];
                int before = 1;
                int after = 0;

                ar[i].pos_to_save = -1;

                strcpy(termz, ar[i].prefix);
                strcat(termz, wrbuf_cstr(ar[i].term));
                dict_scan(zh->reg->dict, termz, &before, &after,
                          ar+i, scan_handle2);
            }
        }
        /* get maximum after scan */
        for (i = 0; i < ord_no; i++)
        {
            if (ar[i].isam_p 
                && (hi == 0 || strcmp(wrbuf_cstr(ar[i].term), hi) > 0))
                hi = wrbuf_cstr(ar[i].term);
        }
        if (!hi)
            break;
        if (scan_save_set(zh, stream, nmem, kc, zapt, limit_set, hi,
                          index_type, ar, ord_no, glist,
                          (pos >= 0 && pos < *num_entries) ? pos : -1))
            --pos;
    }
    /* see if we got all terms before.. */
    dif = 1 + pos;
    if (dif > 0)
    {
        /* did not get all terms; adjust the real position and reduce
           number of entries */
        yaz_log(YLOG_LOG, "before terms dif=%d", dif);
        glist = glist + dif;
        *num_entries -= dif;
        *position -= dif;
	*is_partial = 1;
    }
    for (i = 0; i < ord_no; i++)
    {
        char termz[IT_MAX_WORD+20];
        int prefix_len = 0;
        
        prefix_len = key_SU_encode(ords[i], termz);
        termz[prefix_len] = 0;
        strcpy(ar[i].prefix, termz);
        
        if (trans_scan_term(zh, zapt, termz+prefix_len, zm) == 
            ZEBRA_FAIL)
            return ZEBRA_FAIL;
        wrbuf_rewind(ar[i].term);
        wrbuf_puts(ar[i].term, termz + prefix_len);
        ar[i].isam_p = 0;
        ar[i].ord = ords[i];
    }

    after_pos = 1;  /* immediate term first.. */
    for (pos = *position-1; pos < *num_entries; )
    {
        const char *lo = 0;

        /* scan on all minimum terms */
        for (i = 0; i < ord_no; i++)
        {
            if (ar[i].isam_p == 0)
            {
                char termz[IT_MAX_WORD+20];
                int before = 0;
                int after = after_pos;

                ar[i].pos_to_save = 1;

                strcpy(termz, ar[i].prefix);
                strcat(termz, wrbuf_cstr(ar[i].term));
                dict_scan(zh->reg->dict, termz, &before, &after,
                          ar+i, scan_handle2);
            }
        }
        after_pos = 2;  /* next round we grab following term */

        /* get minimum after scan */
        for (i = 0; i < ord_no; i++)
        {
            if (ar[i].isam_p 
                && (lo == 0 || strcmp(wrbuf_cstr(ar[i].term), lo) < 0))
                lo = wrbuf_cstr(ar[i].term);
        }
        if (!lo)
            break;
        if (scan_save_set(zh, stream, nmem, kc, zapt, limit_set, lo,
                          index_type, ar, ord_no, glist,
                          (pos >= 0 && pos < *num_entries) ? pos : -1))
            pos++;

    }
    if (pos != *num_entries)
    {
        if (pos >= 0)
            *num_entries = pos;
        else
            *num_entries = 0;
        *is_partial = 1;
    }

    *list = glist;

    for (i = 0; i < ord_no; i++)
	wrbuf_destroy(ar[i].term);

    return ZEBRA_OK;
}

struct scan1_info_entry {
    char *term;
    ISAM_P isam_p;
};

struct scan_info {
    struct scan1_info_entry *list;
    ODR odr;
    int before, after;
    char prefix[20];
};

ZEBRA_RES rpn_scan(ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
		   const Odr_oid *attributeset,
		   int num_bases, char **basenames,
		   int *position, int *num_entries, ZebraScanEntry **list,
		   int *is_partial, const char *set_name)
{
    int base_no;
    int ords[RPN_MAX_ORDS], ord_no = 0;

    const char *index_type;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    NMEM nmem;
    ZEBRA_RES res;
    struct rset_key_control *kc = 0;
    RSET limit_set = 0;

    *list = 0;
    *is_partial = 0;

    if (!attributeset)
        attributeset = yaz_oid_attset_bib_1;

    if (!set_name)
    {
        /* see if there is a @attr 8=set */
        AttrType termset;
        int termset_value_numeric;
        const char *termset_value_string = 0;
        attr_init_APT(&termset, zapt, 8);
        termset_value_numeric =
            attr_find_ex(&termset, NULL, &termset_value_string);
        if (termset_value_numeric != -1)
        {
            if (termset_value_numeric != -2)
            {
                char resname[32];
                sprintf(resname, "%d", termset_value_numeric);
                set_name = odr_strdup(stream, resname); 
            }
            else
                set_name = odr_strdup(stream, termset_value_string);
        }
    }

    if (set_name)
    {
        limit_set = resultSetRef(zh, set_name);
        
        if (!limit_set)
        {
            zebra_setError(zh, 
                           YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
                           set_name);
            return ZEBRA_FAIL;
        }
    }

    yaz_log(YLOG_DEBUG, "position = %d, num = %d",
	    *position, *num_entries);
        
    if (zebra_maps_attr(zh->reg->zebra_maps, zapt, &index_type, &search_type,
			rank_type, &complete_flag, &sort_flag))
    {
        *num_entries = 0;
	zebra_setError(zh, YAZ_BIB1_UNSUPP_ATTRIBUTE_TYPE, 0);
        return ZEBRA_FAIL;
    }
    if (num_bases > RPN_MAX_ORDS)
    {
	zebra_setError(zh, YAZ_BIB1_TOO_MANY_DATABASES_SPECIFIED, 0);
        return ZEBRA_FAIL;
    }
    for (base_no = 0; base_no < num_bases; base_no++)
    {
	int ord;

	if (zebraExplain_curDatabase(zh->reg->zei, basenames[base_no]))
	{
	    zebra_setError(zh, YAZ_BIB1_DATABASE_UNAVAILABLE,
			   basenames[base_no]);
	    *num_entries = 0;
	    return ZEBRA_FAIL;
	}
        if (zebra_apt_get_ord(zh, zapt, index_type, 0, attributeset, &ord) 
            != ZEBRA_OK)
            continue;
        ords[ord_no++] = ord;
    }
    if (ord_no == 0)
    {
        *num_entries = 0; /* zebra_apt_get_ord should set error reason */
        return ZEBRA_FAIL;
    }
    if (*num_entries < 1)
    {
	*num_entries = 0;
        return ZEBRA_OK;
    }
    nmem = nmem_create();
    kc = zebra_key_control_create(zh);

    res = rpn_scan_norm(zh, stream, nmem, kc, zapt, position, num_entries,
                        list, is_partial, limit_set,
                        index_type, ord_no, ords);
    nmem_destroy(nmem);
    (*kc->dec)(kc);
    return res;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

