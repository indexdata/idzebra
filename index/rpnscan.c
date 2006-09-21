/* $Id: rpnscan.c,v 1.3 2006-09-21 20:22:34 adam Exp $
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

#define RPN_MAX_ORDS 32

int log_scan = YLOG_LOG;

/* convert APT SCAN term to internal cmap */
static ZEBRA_RES trans_scan_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
				 char *termz, int reg_type)
{
    char termz0[IT_MAX_WORD];

    if (zapt_term_to_utf8(zh, zapt, termz0) == ZEBRA_FAIL)
        return ZEBRA_FAIL;    /* error */
    else
    {
        const char **map;
        const char *cp = (const char *) termz0;
        const char *cp_end = cp + strlen(cp);
        const char *src;
        int i = 0;
        const char *space_map = NULL;
        int len;
            
        while ((len = (cp_end - cp)) > 0)
        {
            map = zebra_maps_input(zh->reg->zebra_maps, reg_type, &cp, len, 0);
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

static void count_set(ZebraHandle zh, RSET rset, zint *count)
{
    zint psysno = 0;
    struct it_key key;
    RSFD rfd;

    yaz_log(YLOG_DEBUG, "count_set");

    rset->hits_limit = zh->approx_limit;

    *count = 0;
    rfd = rset_open(rset, RSETF_READ);
    while (rset_read(rfd, &key,0 /* never mind terms */))
    {
        if (key.mem[0] != psysno)
        {
            psysno = key.mem[0];
	    if (rfd->counted_items >= rset->hits_limit)
		break;
        }
    }
    rset_close (rfd);
    *count = rset->hits_count;
}

struct scan2_info_entry {
    WRBUF term;
    char prefix[20];
    ISAM_P isam_p;
    int pos_to_save;
};

static int scan_handle2(char *name, const char *info, int pos, void *client)
{
    int len_prefix;
    struct scan2_info_entry *scan_info = (struct scan2_info_entry *) client;

    if (scan_info->pos_to_save != pos)
        return 0;

    len_prefix = strlen(scan_info->prefix);
    if (memcmp (name, scan_info->prefix, len_prefix))
        return 1;

    /* skip special terms such as first-in-field specials */
    if (name[len_prefix] < CHR_BASE_CHAR)
        return 1;

    wrbuf_rewind(scan_info->term);
    wrbuf_puts(scan_info->term, name+len_prefix);

    assert (*info == sizeof(ISAM_P));
    memcpy (&scan_info->isam_p, info+1, sizeof(ISAM_P));
    return 0;
}


static int scan_save_set(ZebraHandle zh, ODR stream, NMEM nmem,
                         struct rset_key_control *kc,
                         Z_AttributesPlusTerm *zapt,
                         RSET limit_set,
                         const char *term, 
                         int index_type,
                         struct scan2_info_entry *ar, int ord_no,
                         ZebraScanEntry *glist, int pos)
{
    int i;
    RSET rset = 0;
    for (i = 0; i < ord_no; i++)
    {
        if (ar[i].isam_p && strcmp(wrbuf_buf(ar[i].term), term) == 0)
        {
            RSET rset_t = rset_trunc(
                    zh, &ar[i].isam_p, 1,
                    wrbuf_buf(ar[i].term), wrbuf_len(ar[i].term),
                    NULL, 0, zapt->term->which, nmem, 
                    kc, kc->scope, 0, index_type, 
                    0 /* hits_limit */,
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
        count_set(zh, rset, &count);
        rset_delete(rset);
        if (count > 0)
        {
            if (pos != -1)
            {
                zebra_term_untrans_iconv(zh, stream->mem, index_type,
                                         &glist[pos].term, term);
                glist[pos].occurrences = count;
            }
            return 1;
        }
    }
    return 0;
}
        
static ZEBRA_RES rpn_scan_ver2(ZebraHandle zh, ODR stream, NMEM nmem,
                               struct rset_key_control *kc,
                               Z_AttributesPlusTerm *zapt,
                               int *position, int *num_entries, 
                               ZebraScanEntry **list,
                               int *is_partial, RSET limit_set,
                               int index_type, int ord_no, int *ords)
{
    struct scan2_info_entry *ar = nmem_malloc(nmem, sizeof(*ar) * ord_no);
    struct rpn_char_map_info rcmi;
    int i, dif;
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
    rpn_char_map_prepare (zh->reg, index_type, &rcmi);

    for (i = 0; i < ord_no; i++)
	ar[i].term = wrbuf_alloc();

    for (i = 0; i < ord_no; i++)
    {
        char termz[IT_MAX_WORD+20];
        int prefix_len = 0;
        
        prefix_len = key_SU_encode (ords[i], termz);
        termz[prefix_len] = 0;
        strcpy(ar[i].prefix, termz);
        
        if (trans_scan_term(zh, zapt, termz+prefix_len, index_type) == 
            ZEBRA_FAIL)
            return ZEBRA_FAIL;
        wrbuf_rewind(ar[i].term);
        wrbuf_puts(ar[i].term, termz + prefix_len);
        ar[i].isam_p = 0;
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
                strcat(termz, wrbuf_buf(ar[i].term));
                dict_scan(zh->reg->dict, termz, &before, &after,
                          ar+i, scan_handle2);
            }
        }
        /* get maximum after scan */
        for (i = 0; i < ord_no; i++)
        {
            if (ar[i].isam_p 
                && (hi == 0 || strcmp(wrbuf_buf(ar[i].term), hi) > 0))
                hi = wrbuf_buf(ar[i].term);
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
        
        prefix_len = key_SU_encode (ords[i], termz);
        termz[prefix_len] = 0;
        strcpy(ar[i].prefix, termz);
        
        if (trans_scan_term(zh, zapt, termz+prefix_len, index_type) == 
            ZEBRA_FAIL)
            return ZEBRA_FAIL;
        wrbuf_rewind(ar[i].term);
        wrbuf_puts(ar[i].term, termz + prefix_len);
        ar[i].isam_p = 0;
    }

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
                int after = (pos == *position-1) ? 1 : 2;

                ar[i].pos_to_save = 1;

                strcpy(termz, ar[i].prefix);
                strcat(termz, wrbuf_buf(ar[i].term));
                dict_scan(zh->reg->dict, termz, &before, &after,
                          ar+i, scan_handle2);
            }
        }
        /* get minimum after scan */
        for (i = 0; i < ord_no; i++)
        {
            if (ar[i].isam_p 
                && (lo == 0 || strcmp(wrbuf_buf(ar[i].term), lo) < 0))
                lo = wrbuf_buf(ar[i].term);
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

static int scan_handle1(char *name, const char *info, int pos, void *client)
{
    int len_prefix, idx;
    struct scan_info *scan_info = (struct scan_info *) client;

    len_prefix = strlen(scan_info->prefix);
    if (memcmp (name, scan_info->prefix, len_prefix))
        return 1;
    if (pos > 0)
	idx = scan_info->after - pos + scan_info->before;
    else
        idx = - pos - 1;

    /* skip special terms such as first-in-field specials */
    if (name[len_prefix] < CHR_BASE_CHAR)
        return 1;

    if (idx < 0)
	return 0;
    scan_info->list[idx].term = (char *)
        odr_malloc(scan_info->odr, strlen(name + len_prefix)+1);
    strcpy(scan_info->list[idx].term, name + len_prefix);
    assert (*info == sizeof(ISAM_P));
    memcpy (&scan_info->list[idx].isam_p, info+1, sizeof(ISAM_P));
    return 0;
}

static ZEBRA_RES rpn_scan_ver1(ZebraHandle zh, ODR stream, NMEM rset_nmem,
                               struct rset_key_control *kc,
                               Z_AttributesPlusTerm *zapt,
                               int *position, int *num_entries, 
                               ZebraScanEntry **list,
                               int *is_partial, RSET limit_set,
                               int index_type, int ord_no, int *ords)
{
    int pos = *position;
    int num = *num_entries;
    int before;
    int after;
    int i;
    struct scan_info *scan_info_array;
    char termz[IT_MAX_WORD+20];
    ZebraScanEntry *glist;
    int ptr[RPN_MAX_ORDS];

    before = pos-1;
    if (before < 0)
	before = 0;
    after = 1+num-pos;
    if (after < 0)
	after = 0;
    yaz_log(YLOG_DEBUG, "rpn_scan pos=%d num=%d before=%d "
	    "after=%d before+after=%d",
	    pos, num, before, after, before+after);
    scan_info_array = (struct scan_info *)
        odr_malloc(stream, ord_no * sizeof(*scan_info_array));
    for (i = 0; i < ord_no; i++)
    {
        int j, prefix_len = 0;
        int before_tmp = before, after_tmp = after;
        struct scan_info *scan_info = scan_info_array + i;
        struct rpn_char_map_info rcmi;

        rpn_char_map_prepare (zh->reg, index_type, &rcmi);

        scan_info->before = before;
        scan_info->after = after;
        scan_info->odr = stream;

        scan_info->list = (struct scan1_info_entry *)
            odr_malloc(stream, (before+after) * sizeof(*scan_info->list));
        for (j = 0; j<before+after; j++)
            scan_info->list[j].term = NULL;

        prefix_len += key_SU_encode (ords[i], termz + prefix_len);
        termz[prefix_len] = 0;
        strcpy(scan_info->prefix, termz);

        if (trans_scan_term(zh, zapt, termz+prefix_len, index_type) == 
            ZEBRA_FAIL)
            return ZEBRA_FAIL;
	
        dict_scan(zh->reg->dict, termz, &before_tmp, &after_tmp,
		  scan_info, scan_handle1);
    }
    glist = (ZebraScanEntry *)
        odr_malloc(stream, (before+after)*sizeof(*glist));

    /* consider terms after main term */
    for (i = 0; i < ord_no; i++)
        ptr[i] = before;
    
    *is_partial = 0;
    for (i = 0; i<after; i++)
    {
        int j, j0 = -1;
        const char *mterm = NULL;
        const char *tst;
        RSET rset = 0;
	int lo = i + pos-1; /* offset in result list */

	/* find: j0 is the first of the minimal values */
        for (j = 0; j < ord_no; j++)
        {
            if (ptr[j] < before+after && ptr[j] >= 0 &&
                (tst = scan_info_array[j].list[ptr[j]].term) &&
                (!mterm || strcmp (tst, mterm) < 0))
            {
                j0 = j;
                mterm = tst;
            }
        }
        if (j0 == -1)
            break;  /* no value found, stop */

	/* get result set for first one , but only if it's within bounds */
	if (lo >= 0)
	{
	    /* get result set for first term */
	    zebra_term_untrans_iconv(zh, stream->mem, index_type,
				     &glist[lo].term, mterm);
	    rset = rset_trunc(zh, &scan_info_array[j0].list[ptr[j0]].isam_p, 1,
			      glist[lo].term, strlen(glist[lo].term),
			      NULL, 0, zapt->term->which, rset_nmem, 
			      kc, kc->scope, 0, index_type, 0 /* hits_limit */,
			      0 /* term_ref_id_str */);
	}
	ptr[j0]++; /* move index for this set .. */
	/* get result set for remaining scan terms */
        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before+after && ptr[j] >= 0 &&
                (tst = scan_info_array[j].list[ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
		if (lo >= 0)
		{
		    RSET rsets[2];
		    
		    rsets[0] = rset;
		    rsets[1] =
			rset_trunc(
			    zh, &scan_info_array[j].list[ptr[j]].isam_p, 1,
			    glist[lo].term,
			    strlen(glist[lo].term), NULL, 0,
			    zapt->term->which,rset_nmem,
			    kc, kc->scope, 0, index_type, 0 /* hits_limit */,
			    0 /* term_ref_id_str */ );
		    rset = rset_create_or(rset_nmem, kc,
                                          kc->scope, 0 /* termid */,
                                          2, rsets);
		}
                ptr[j]++;
            }
        }
	if (lo >= 0)
	{
	    zint count;
	    /* merge with limit_set if given */
	    if (limit_set)
	    {
		RSET rsets[2];
		rsets[0] = rset;
		rsets[1] = rset_dup(limit_set);
		
		rset = rset_create_and(rset_nmem, kc, kc->scope, 2, rsets);
	    }
	    /* count it */
	    count_set(zh, rset, &count);
	    glist[lo].occurrences = count;
	    rset_delete(rset);
	}
    }
    if (i < after)
    {
	*num_entries -= (after-i);
	*is_partial = 1;
	if (*num_entries < 0)
	{
	    *num_entries = 0;
	    return ZEBRA_OK;
	}
    }
    /* consider terms before main term */
    for (i = 0; i<ord_no; i++)
	ptr[i] = 0;
    
    for (i = 0; i<before; i++)
    {
	int j, j0 = -1;
	const char *mterm = NULL;
	const char *tst;
	RSET rset;
	int lo = before-1-i; /* offset in result list */
	zint count;
	
	for (j = 0; j <ord_no; j++)
	{
	    if (ptr[j] < before && ptr[j] >= 0 &&
		(tst = scan_info_array[j].list[before-1-ptr[j]].term) &&
		(!mterm || strcmp (tst, mterm) > 0))
	    {
		j0 = j;
		    mterm = tst;
	    }
	}
	if (j0 == -1)
	    break;
	
	zebra_term_untrans_iconv(zh, stream->mem, index_type,
				 &glist[lo].term, mterm);
	
	rset = rset_trunc
	    (zh, &scan_info_array[j0].list[before-1-ptr[j0]].isam_p, 1,
	     glist[lo].term, strlen(glist[lo].term),
	     NULL, 0, zapt->term->which, rset_nmem,
	     kc, kc->scope, 0, index_type, 0 /* hits_limit */,
	     0 /* term_ref_id_str */);
	
	ptr[j0]++;
	
	for (j = j0+1; j<ord_no; j++)
	{
	    if (ptr[j] < before && ptr[j] >= 0 &&
		(tst = scan_info_array[j].list[before-1-ptr[j]].term) &&
		!strcmp (tst, mterm))
	    {
		RSET rsets[2];
		
		rsets[0] = rset;
		rsets[1] = rset_trunc(
		    zh,
		    &scan_info_array[j].list[before-1-ptr[j]].isam_p, 1,
		    glist[lo].term,
		    strlen(glist[lo].term), NULL, 0,
		    zapt->term->which, rset_nmem,
		    kc, kc->scope, 0, index_type, 0 /* hits_limit */,
		    0 /* term_ref_id_str */);
		rset = rset_create_or(rset_nmem, kc,
                                      kc->scope, 0 /* termid */, 2, rsets);
		
		ptr[j]++;
	    }
	}
        if (limit_set)
	{
	    RSET rsets[2];
	    rsets[0] = rset;
	    rsets[1] = rset_dup(limit_set);
	    
	    rset = rset_create_and(rset_nmem, kc, kc->scope, 2, rsets);
	}
	count_set(zh, rset, &count);
	glist[lo].occurrences = count;
	rset_delete (rset);
    }
    i = before-i;
    if (i)
    {
        *is_partial = 1;
        *position -= i;
        *num_entries -= i;
	if (*num_entries <= 0)
	{
	    *num_entries = 0;
	    return ZEBRA_OK;
	}
    }
    
    *list = glist + i;               /* list is set to first 'real' entry */
    
    yaz_log(YLOG_DEBUG, "position = %d, num_entries = %d",
	    *position, *num_entries);
    return ZEBRA_OK;
}


ZEBRA_RES rpn_scan(ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
		   oid_value attributeset,
		   int num_bases, char **basenames,
		   int *position, int *num_entries, ZebraScanEntry **list,
		   int *is_partial, RSET limit_set)
{
    int base_no;
    int ords[RPN_MAX_ORDS], ord_no = 0;

    unsigned index_type;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    NMEM nmem;
    ZEBRA_RES res;
    struct rset_key_control *kc = 0;

    *list = 0;
    *is_partial = 0;

    if (attributeset == VAL_NONE)
        attributeset = VAL_BIB1;

    if (!limit_set) /* no limit set given already */
    {
        /* see if there is a @attr 8=set */
        AttrType termset;
        int termset_value_numeric;
        const char *termset_value_string;
        attr_init_APT(&termset, zapt, 8);
        termset_value_numeric =
            attr_find_ex(&termset, NULL, &termset_value_string);
        if (termset_value_numeric != -1)
        {
            char resname[32];
            const char *termset_name = 0;
            
            if (termset_value_numeric != -2)
            {
                
                sprintf(resname, "%d", termset_value_numeric);
                termset_name = resname;
            }
            else
                termset_name = termset_value_string;
            
            limit_set = resultSetRef (zh, termset_name);
        }
    }
        
    yaz_log(YLOG_DEBUG, "position = %d, num = %d set=%d",
	    *position, *num_entries, attributeset);
        
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

	if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
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

    res = rpn_scan_ver2(zh, stream, nmem, kc, zapt, position, num_entries,
                        list,
                        is_partial, limit_set, index_type, ord_no, ords);
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

