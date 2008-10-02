/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

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

#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "index.h"
#include <yaz/diagbib1.h>
#include <yaz/snprintf.h>
#include <direntz.h>
#include <yaz/oid_db.h>
#include <zebra_strmap.h>

#define MAX_SYSNOS_PER_RECORD 40

#define ZEBRA_XML_HEADER_STR "<record xmlns=\"http://www.indexdata.com/zebra/\""

struct special_fetch_s {
    ZebraHandle zh;
    const char *setname;
    zint sysno;
    int score;
    NMEM nmem;
};

static int zebra_create_record_stream(ZebraHandle zh, 
                                      Record *rec,
                                      struct ZebraRecStream *stream)
{
    RecordAttr *recordAttr = rec_init_attr(zh->reg->zei, *rec);

    if ((*rec)->size[recInfo_storeData] > 0 
        || (*rec)->info[recInfo_filename] == 0)
        zebra_create_stream_mem(stream, (*rec)->info[recInfo_storeData],
                                (*rec)->size[recInfo_storeData]);
    else
    {
        char full_rep[1024];
        int fd;
            
        if (zh->path_reg && !yaz_is_abspath((*rec)->info[recInfo_filename])){
            strcpy(full_rep, zh->path_reg);
            strcat(full_rep, "/");
            strcat(full_rep, (*rec)->info[recInfo_filename]);
        }
        else
            strcpy(full_rep, (*rec)->info[recInfo_filename]);
            
        if ((fd = open(full_rep, O_BINARY|O_RDONLY)) == -1){
            yaz_log(YLOG_WARN|YLOG_ERRNO, "Retrieve fail; missing file: %s",
                     full_rep);
            rec_free(rec);
            return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
        }
        zebra_create_stream_fd(stream, fd, recordAttr->recordOffset);
    }
    return 0;
}
   

struct index_spec {
    const char *index_name;
    const char *index_type;
    const char *extra;
    struct index_spec *next;
};


struct index_spec *parse_index_spec(const char *elem, NMEM nmem,
                                    int *error)
{
    struct index_spec *first = 0;
    struct index_spec **last = &first;
    const char *cp = elem;

    *error = 0;
    if (cp[0] == ':' && cp[1] == ':')
    {

        cp++; /* skip first ':' */

        for (;;)
        {
            const char *cp0;
            struct index_spec *spec = nmem_malloc(nmem, sizeof(*spec));
            spec->index_type = 0;
            spec->next = 0;
            spec->extra = 0;

            if (!first)
                first = spec;
            *last = spec;
            last = &spec->next;

            cp++; /* skip ',' or second ':' */
            cp0 = cp;
            while (*cp != ':' && *cp != '\0' && *cp != ',')
                cp++;
            spec->index_name = nmem_strdupn(nmem, cp0, cp - cp0);
            if (*cp == ':') /* type as well */
            {
                cp++;
                cp0 = cp;
                
                while (*cp != '\0' && *cp != ',' && *cp != ':')
                    cp++;
                spec->index_type = nmem_strdupn(nmem, cp0, cp - cp0);
            }
            if (*cp == ':') /* extra arguments */
            {
                cp++;
                cp0 = cp;
                
                while (*cp != '\0' && *cp != ',' && *cp != ':')
                    cp++;
                spec->extra = nmem_strdupn(nmem, cp0, cp - cp0);
            }
            if (*cp != ',')
                break;
        }
    }
    if (*cp != '\0')
        *error = 1;
    return first;
}
                            
static int parse_zebra_elem(const char *elem,
                            const char **index, size_t *index_len,
                            const char **type, size_t *type_len)
{
    *index = 0;
    *index_len = 0;

    *type = 0;
    *type_len = 0;

    if (elem && *elem)
    {
        char *cp;
        /* verify that '::' is in the beginning of *elem 
           and something more follows */
        if (':' != *elem
            || !(elem +1) || ':' != *(elem +1)
            || !(elem +2) || '\0' == *(elem +2))
            return 0;
 
        /* pick out info from string after '::' */
        elem = elem + 2;
        cp = strchr(elem, ':');

        if (!cp) /* index, no colon, no type */
        {
            *index = elem;
            *index_len = strlen(elem);
        }
        else if (cp[1] == '\0') /* colon, but no following type */
        {
            return 0;
        }
        else  /* index, colon and type */
        {
            *index = elem;
            *index_len = cp - elem;
            *type = cp+1;
            *type_len = strlen(cp+1);
        }
    }
    return 1;
}


static int sort_fetch(
    struct special_fetch_s *fi, const char *elemsetname,
    const Odr_oid *input_format,
    const Odr_oid **output_format,
    WRBUF result, WRBUF addinfo)
{
    const char *retrieval_index;
    size_t retrieval_index_len; 
    const char *retrieval_type;
    size_t retrieval_type_len;
    char retrieval_index_cstr[256];
    char retrieval_type_cstr[256];
    int ord;
    ZebraHandle zh = fi->zh;

    if (!parse_zebra_elem(elemsetname,
                          &retrieval_index, &retrieval_index_len,
                          &retrieval_type,  &retrieval_type_len))
    {
        return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
    }
    
    if (retrieval_type_len == 0)
        return -1;   /* must have a register type specified */
    if (!retrieval_index_len ||
        retrieval_index_len >= sizeof(retrieval_index_cstr)-1)
    {
        return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
    }
        
    memcpy(retrieval_index_cstr, retrieval_index, retrieval_index_len);
    retrieval_index_cstr[retrieval_index_len] = '\0';

    memcpy(retrieval_type_cstr, retrieval_type, retrieval_type_len);
    retrieval_type_cstr[retrieval_type_len] = '\0';

    ord = zebraExplain_lookup_attr_str(zh->reg->zei,
                                       zinfo_index_category_sort,
                                       retrieval_type_cstr,
                                       retrieval_index_cstr);
    if (ord == -1)
        return -1;  /* is not a sort index */
    else
    {
        WRBUF wrbuf_str = wrbuf_alloc();
        const char *index_type;
        const char *db = 0;
        const char *string_index = 0;
        WRBUF wrbuf_result = result;
        int off = 0;
        
        zebraExplain_lookup_ord(zh->reg->zei, ord, &index_type, &db, 
                                &string_index);
        if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
        {
            *output_format = yaz_oid_recsyn_xml;
            wrbuf_printf(wrbuf_result, ZEBRA_XML_HEADER_STR
                         " sysno=\"" ZINT_FORMAT "\""
                         " set=\"zebra::index%s/\">\n",
                         fi->sysno, elemsetname);
        }
        else if (!oid_oidcmp(input_format, yaz_oid_recsyn_sutrs))
        {
            *output_format = yaz_oid_recsyn_sutrs;
        }
        else
        {
            yaz_log(YLOG_WARN, "unsupported format for element set zebra::%s", 
                    elemsetname);
            *output_format = 0;
            wrbuf_destroy(wrbuf_str);
            return YAZ_BIB1_NO_SYNTAXES_AVAILABLE_FOR_THIS_REQUEST;
        }
        zebra_sort_type(zh->reg->sort_index, ord);
        zebra_sort_sysno(zh->reg->sort_index, fi->sysno);
        zebra_sort_read(zh->reg->sort_index, wrbuf_str);

        while (off != wrbuf_len(wrbuf_str))
        {
            char dst_buf[IT_MAX_WORD];
            assert(off < wrbuf_len(wrbuf_str));
            zebra_term_untrans(zh, index_type, dst_buf,
                               wrbuf_buf(wrbuf_str)+off);
            
            if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
            {
                wrbuf_printf(wrbuf_result, "  <index name=\"%s\"", 
                             string_index);
                wrbuf_printf(wrbuf_result, " type=\"%s\">", index_type);
                wrbuf_xmlputs(wrbuf_result, dst_buf);
                wrbuf_printf(wrbuf_result, "</index>\n");
            }
            else if (!oid_oidcmp(input_format, yaz_oid_recsyn_sutrs))
            {
                wrbuf_printf(wrbuf_result, "%s %s %s\n", string_index, index_type,
                             dst_buf);
            }
            off += strlen(wrbuf_buf(wrbuf_str)+off) + 1;
        }
        if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
        {
            wrbuf_printf(wrbuf_result, "</record>\n");
        }
        wrbuf_destroy(wrbuf_str);
        return 0;
    }
}
                            
static int special_index_fetch(
    struct special_fetch_s *fi, const char *elemsetname,
    const Odr_oid *input_format,
    const Odr_oid **output_format,
    WRBUF result, WRBUF addinfo,
    Record rec)
{
    const char *retrieval_index;
    size_t retrieval_index_len; 
    const char *retrieval_type;
    size_t retrieval_type_len;
    zebra_rec_keys_t keys;
    int ret_code = 0;
    char retrieval_type_cstr[256];
    ZebraHandle zh = fi->zh;
    
    /* set output variables before processing possible error states */
    /* *rec_lenp = 0; */

    /* only accept XML and SUTRS requests */
    if (oid_oidcmp(input_format, yaz_oid_recsyn_xml)
        && oid_oidcmp(input_format, yaz_oid_recsyn_sutrs))
    {
        yaz_log(YLOG_WARN, "unsupported format for element set zebra::%s", 
                elemsetname);
        *output_format = 0;
        return YAZ_BIB1_NO_SYNTAXES_AVAILABLE_FOR_THIS_REQUEST;
    }

    if (!parse_zebra_elem(elemsetname,
                     &retrieval_index, &retrieval_index_len,
                     &retrieval_type,  &retrieval_type_len))
        return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;

    if (retrieval_type_len)
    {
        memcpy(retrieval_type_cstr, retrieval_type, retrieval_type_len);
        retrieval_type_cstr[retrieval_type_len] = '\0';
    }
    
    if (retrieval_index_len)
    {
        char retrieval_index_cstr[256];

        if (retrieval_index_len < sizeof(retrieval_index_cstr) -1)
        {
            memcpy(retrieval_index_cstr, retrieval_index, retrieval_index_len);
            retrieval_index_cstr[retrieval_index_len] = '\0';
            
            if (zebraExplain_lookup_attr_str(zh->reg->zei,
                                             zinfo_index_category_index,
                                             (retrieval_type_len == 0 ? 0 : 
                                              retrieval_type_cstr),
                                             retrieval_index_cstr) == -1)
                return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
        }
    }

    keys = zebra_rec_keys_open();
    zebra_rec_keys_set_buf(keys, rec->info[recInfo_delKeys],
                           rec->size[recInfo_delKeys], 0);

    if (!zebra_rec_keys_rewind(keys))
    {
        ret_code = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }
    else
    {
        size_t slen;
        const char *str;
        struct it_key key_in;
        WRBUF wrbuf = result;
    
        if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
        {
            *output_format = input_format;
            wrbuf_printf(wrbuf, ZEBRA_XML_HEADER_STR
                         " sysno=\"" ZINT_FORMAT "\""
                         " set=\"zebra::index%s/\">\n",
                         fi->sysno, elemsetname);
        }
        else if (!oid_oidcmp(input_format, yaz_oid_recsyn_sutrs))
            *output_format = input_format;

        while (zebra_rec_keys_read(keys, &str, &slen, &key_in))
        {
            int i;
            int ord = CAST_ZINT_TO_INT(key_in.mem[0]);
            const char *index_type;
            const char *db = 0;
            const char *string_index = 0;
            size_t string_index_len;
            char dst_buf[IT_MAX_WORD];
            
            zebraExplain_lookup_ord(zh->reg->zei, ord, &index_type, &db,
                                    &string_index);
            string_index_len = strlen(string_index);

            /* process only if index is not defined, 
               or if defined and matching */
            if (retrieval_index == 0 
                || (string_index_len == retrieval_index_len 
                    && !memcmp(string_index, retrieval_index,
                               string_index_len)))
            {
                /* process only if type is not defined, or is matching */
                if (retrieval_type == 0 
                    || !strcmp(retrieval_type_cstr, index_type))
                {
                    if (zebra_term_untrans(zh, index_type, dst_buf, str))
                        *dst_buf = '\0'; /* untrans failed */

                    if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
                    {
                        wrbuf_printf(wrbuf, "  <index name=\"%s\"", 
                                     string_index);
                        
                        wrbuf_printf(wrbuf, " type=\"%s\"", index_type);
                        
                        wrbuf_printf(wrbuf, " seq=\"" ZINT_FORMAT "\">", 
                                     key_in.mem[key_in.len -1]);
                        wrbuf_xmlputs(wrbuf, dst_buf);
                        wrbuf_printf(wrbuf, "</index>\n");
                    }
                    else 
                    {
                        wrbuf_printf(wrbuf, "%s ", string_index);
                        
                        wrbuf_printf(wrbuf, "%s", index_type);
                        
                        for (i = 1; i < key_in.len; i++)
                            wrbuf_printf(wrbuf, " " ZINT_FORMAT, 
                                             key_in.mem[i]);
                        
                        wrbuf_printf(wrbuf, " %s", dst_buf);
                        
                        wrbuf_printf(wrbuf, "\n");

                    }
                    
                }
            }
        }
        if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
            wrbuf_printf(wrbuf, "</record>\n");
    }
    zebra_rec_keys_close(keys);
    return ret_code;
}


static void retrieve_puts_attr(WRBUF wrbuf, const char *name,
                               const char *value)
{
    if (value)
    {
        wrbuf_printf(wrbuf, " %s=\"", name);
        wrbuf_xmlputs(wrbuf, value);
        wrbuf_printf(wrbuf, "\"");
    }
}

static void retrieve_puts_attr_int(WRBUF wrbuf, const char *name,
                               const int value)
{
    wrbuf_printf(wrbuf, " %s=\"%i\"", name, value);
}

static void retrieve_puts_str(WRBUF wrbuf, const char *name,
                               const char *value)
{
    if (value)
        wrbuf_printf(wrbuf, "%s %s\n", name, value);
}

static void retrieve_puts_int(WRBUF wrbuf, const char *name,
                               const int value)
{
    wrbuf_printf(wrbuf, "%s %i\n", name, value);
}


static void snippet_xml_record(ZebraHandle zh, WRBUF wrbuf, zebra_snippets *doc)
{
    const zebra_snippet_word *doc_w;
    int mark_state = 0;

    wrbuf_printf(wrbuf, "%s>\n", ZEBRA_XML_HEADER_STR);
    for (doc_w = zebra_snippets_constlist(doc); doc_w; doc_w = doc_w->next)
    {
        if (doc_w->mark)
        {
            const char *index_type;
            const char *db = 0;
            const char *string_index = 0;

            zebraExplain_lookup_ord(zh->reg->zei, doc_w->ord, 
                                    &index_type, &db, &string_index);

            if (mark_state == 0)
            {
                wrbuf_printf(wrbuf, "  <snippet name=\"%s\"",  string_index);
                wrbuf_printf(wrbuf, " type=\"%s\">", index_type);
            }
            if (doc_w->match)
                wrbuf_puts(wrbuf, "<s>");
            /* not printing leading ws */
            if (mark_state || !doc_w->ws || doc_w->match) 
                wrbuf_xmlputs(wrbuf, doc_w->term);
            if (doc_w->match)
                wrbuf_puts(wrbuf, "</s>");
        }
        else if (mark_state == 1)
        {
            wrbuf_puts(wrbuf, "</snippet>\n");
        }
        mark_state = doc_w->mark;
    }
    if (mark_state == 1)
    {
        wrbuf_puts(wrbuf, "</snippet>\n");
    }
    wrbuf_printf(wrbuf, "</record>");
}

int zebra_get_rec_snippets(ZebraHandle zh, zint sysno,
                           zebra_snippets *snippets)
{
    int return_code = 0;
    Record rec = rec_get(zh->reg->records, sysno);
    if (!rec)
    {
        yaz_log(YLOG_WARN, "rec_get fail on sysno=" ZINT_FORMAT, sysno);
        return_code = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }
    else
    {
        const char *file_type = rec->info[recInfo_fileType];
        void *recTypeClientData;
        RecType rt = recType_byName(zh->reg->recTypes, zh->res,
                                    file_type, &recTypeClientData);

        if (!rt)
            return_code = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
        else
        {
            struct ZebraRecStream stream;
            return_code = zebra_create_record_stream(zh, &rec, &stream);
            if (return_code == 0)
            {
                extract_snippet(zh, snippets, &stream,
                                rt, recTypeClientData);

                stream.destroy(&stream);
            }
        }
        rec_free(&rec);
    }
    return return_code;
}

static int snippet_fetch(
    struct special_fetch_s *fi, const char *elemsetname,
    const Odr_oid *input_format,
    const Odr_oid **output_format,
    WRBUF result, WRBUF addinfo)
{
    ZebraHandle zh = fi->zh;
    zebra_snippets *rec_snippets = zebra_snippets_create();
    int return_code = zebra_get_rec_snippets(zh, fi->sysno, rec_snippets);

    if (!return_code)
    {
        WRBUF wrbuf = result;
        zebra_snippets *hit_snippet = zebra_snippets_create();

        zebra_snippets_hit_vector(zh, fi->setname, fi->sysno, hit_snippet);

#if 0
        /* for debugging purposes */
        yaz_log(YLOG_LOG, "---------------------------");
        yaz_log(YLOG_LOG, "REC SNIPPET:");
        zebra_snippets_log(rec_snippets, YLOG_LOG, 1);
        yaz_log(YLOG_LOG, "---------------------------");
        yaz_log(YLOG_LOG, "HIT SNIPPET:");
        zebra_snippets_log(hit_snippet, YLOG_LOG, 1);
#endif
        
        zebra_snippets_ring(rec_snippets, hit_snippet, 5, 5);
        
#if 0
        yaz_log(YLOG_LOG, "---------------------------");
        yaz_log(YLOG_LOG, "RING SNIPPET:");
        zebra_snippets_log(rec_snippets, YLOG_LOG, 1);
#endif
        snippet_xml_record(zh, wrbuf, rec_snippets);
        
        *output_format = yaz_oid_recsyn_xml;
        
        zebra_snippets_destroy(hit_snippet);
    }
    zebra_snippets_destroy(rec_snippets);
    return return_code;
}

struct term_collect {
    const char *term;
    int oc;
    zint set_occur;
};

static zint freq_term(ZebraHandle zh, int ord, const char *term, RSET rset_set)
{
    struct rset_key_control *kc = zebra_key_control_create(zh);
    char ord_buf[IT_MAX_WORD];
    int ord_len = key_SU_encode(ord, ord_buf);
    char *info;
    zint hits = 0;
    NMEM nmem = nmem_create();
    
    strcpy(ord_buf + ord_len, term);
    
    info = dict_lookup(zh->reg->dict, ord_buf);
    if (info)
    {
        ISAM_P isam_p;
        RSET rsets[2], rset;
        memcpy(&isam_p, info+1, sizeof(ISAM_P));
        
        rsets[0] = zebra_create_rset_isam(zh, nmem, kc, kc->scope, isam_p, 0);
        rsets[1] = rset_dup(rset_set);
        
        rset = rset_create_and(nmem, kc, kc->scope, 2, rsets);

        zebra_count_set(zh, rset, &hits, zh->approx_limit);

        rset_delete(rsets[0]);
        rset_delete(rset);
    }
    (*kc->dec)(kc);
    nmem_destroy(nmem);
    return hits;
}

static int term_qsort_handle(const void *a, const void *b)
{
    const struct term_collect *l = a;
    const struct term_collect *r = b;
    if (l->set_occur < r->set_occur)
        return 1;
    else if (l->set_occur > r->set_occur)
        return -1;
    else
    {
        const char *lterm = l->term ? l->term : "";
        const char *rterm = r->term ? r->term : "";
        return strcmp(lterm, rterm);
    }
}

static void term_collect_freq(ZebraHandle zh,
                              struct term_collect *col, int no_terms_collect,
                              int ord, RSET rset, double scale_factor)
{
    int i;
    for (i = 0; i < no_terms_collect; i++)
    {
        if (col[i].term)
        {
            if (scale_factor < 0.0)
            {
                col[i].set_occur = freq_term(zh, ord, col[i].term, rset);
            }
            else
                col[i].set_occur = scale_factor * col[i].oc;
        }
    }
    qsort(col, no_terms_collect, sizeof(*col), term_qsort_handle);
}

static struct term_collect *term_collect_create(zebra_strmap_t sm, 
                                                int no_terms_collect,
                                                NMEM nmem)
{
    const char *term;
    void *data_buf;
    size_t data_len;
    zebra_strmap_it it;
    struct term_collect *col = nmem_malloc(nmem, 
                                           sizeof *col *no_terms_collect);
    int i;
    for (i = 0; i < no_terms_collect; i++)
    {
        col[i].term = 0;
        col[i].oc = 0;
        col[i].set_occur = 0;
    }
    /* iterate over terms and collect the most frequent ones */
    it = zebra_strmap_it_create(sm);
    while ((term = zebra_strmap_it_next(it, &data_buf, &data_len)))
    {
        /* invariant:
           col[0] has lowest oc .  col[no_terms_collect-1] has highest oc */
        int oc = *(int*) data_buf;
        int j = 0;
        /* insertion may be slow but terms terms will be "infrequent" and
           thus number of iterations should be small below 
        */
        while (j < no_terms_collect && oc > col[j].oc)
            j++;
        if (j) 
        {   /* oc <= col[j] and oc > col[j-1] */
            --j;
            memmove(col, col+1, sizeof(*col) * j);
            col[j].term = term;
            col[j].oc = oc;
        }
    }
    zebra_strmap_it_destroy(it);
    return col;
}

static int perform_facet_sort(ZebraHandle zh, int no_ord, int *ord_array,
                              zebra_strmap_t *map_array,
                              int num_recs, ZebraMetaRecord *poset)
{
    int rec_i;
    WRBUF w = wrbuf_alloc();
    int ord_i;

    for (ord_i = 0; ord_i < no_ord; ord_i++)
    {
        for (rec_i = 0; rec_i < num_recs; rec_i++)
        {
            if (!poset[rec_i].sysno)
                continue;
            
            zebra_sort_sysno(zh->reg->sort_index, poset[rec_i].sysno);
            zebra_sort_type(zh->reg->sort_index, ord_array[ord_i]);
            
            wrbuf_rewind(w);
            if (zebra_sort_read(zh->reg->sort_index, w))
            {
                zebra_strmap_t sm = map_array[ord_i];
                int off = 0;
                while (off != wrbuf_len(w))
                {
                    const char *str = wrbuf_buf(w) + off;
                    int *freq = zebra_strmap_lookup(sm, str, 0, 0);
                    if (freq)
                        (*freq)++;
                    else
                    {
                        int v = 1;
                        zebra_strmap_add(sm, str, &v, sizeof v);
                    }
                    off += strlen(str)+1;
                }
            }
        }
    }
    wrbuf_destroy(w);
    return 0;
}


static int perform_facet_index(ZebraHandle zh,
                               struct special_fetch_s *fi,
                               int no_ord, int *ord_array,
                               zebra_strmap_t *map_array,
                               int num_recs, ZebraMetaRecord *poset,
                               struct index_spec *spec_list)
{
    int max_chunks = 2;
    int rec_i;
    res_get_int(zh->res, "facetMaxChunks", &max_chunks);

    for (rec_i = 0; rec_i < num_recs; rec_i++)
    {
        int ret;
        int j;
        zint sysnos[MAX_SYSNOS_PER_RECORD];
        int no_sysnos = MAX_SYSNOS_PER_RECORD;
        if (!poset[rec_i].sysno)
            continue;
        ret = zebra_result_recid_to_sysno(zh, fi->setname,
                                          poset[rec_i].sysno,
                                          sysnos, &no_sysnos);
        assert(no_sysnos > 0);
        yaz_log(YLOG_DEBUG, "Analyzing rec=%d ISAM sysno=" ZINT_FORMAT " chunks=%d",
                rec_i, poset[rec_i].sysno, no_sysnos);
        for (j = 0; j < no_sysnos && j < max_chunks; j++)
        {
            size_t slen;
            const char *str;
            struct it_key key_in;
            Record rec = rec_get(zh->reg->records, sysnos[j]);
            zebra_rec_keys_t keys = zebra_rec_keys_open();
            zebra_rec_keys_set_buf(keys, rec->info[recInfo_delKeys],
                                   rec->size[recInfo_delKeys], 0);
            
            yaz_log(YLOG_DEBUG, "rec %d " ZINT_FORMAT " %s", 
                    j, sysnos[j], zebra_rec_keys_empty(keys) ? "empty" : "non-empty");
            if (zebra_rec_keys_rewind(keys))
            {
                while (zebra_rec_keys_read(keys, &str, &slen, &key_in))
                {
                    int ord_i;
                    struct index_spec *spec;
                    for (spec = spec_list, ord_i = 0; ord_i < no_ord; 
                         ord_i++, spec = spec->next)
                    {
                        int ord = CAST_ZINT_TO_INT(key_in.mem[0]);
                        if (ord == ord_array[ord_i] && 
                            str[0] != FIRST_IN_FIELD_CHAR)
                        {
                            int *freq;
                            zebra_strmap_t sm = map_array[ord_i];
                            
                            freq = zebra_strmap_lookup(sm, str, 0, 0);
                            if (freq)
                                (*freq)++;
                            else
                            {
                                int v = 1;
                                zebra_strmap_add(sm, str, &v, sizeof v);
                            }
                        }
                    }
                }
            }
            zebra_rec_keys_close(keys);
            rec_free(&rec);
        }
    }
    return 0;
}

static int perform_facet(ZebraHandle zh,  
                         struct special_fetch_s *fi,
                         WRBUF result,
                         int num_recs, ZebraMetaRecord *poset,
                         struct index_spec *spec_list,
                         int no_ord, int *ord_array,
                         int use_xml,
                         zinfo_index_category_t cat)
{
    int i;
    int ret = 0;
    WRBUF wr = result;
    struct index_spec *spec;
    yaz_timing_t timing = yaz_timing_create();
    zebra_strmap_t *map_array
        = nmem_malloc(fi->nmem, sizeof *map_array * no_ord);
    for (i = 0; i < no_ord; i++)
        map_array[i] = zebra_strmap_create();

    if (cat == zinfo_index_category_sort)
        perform_facet_sort(zh, no_ord, ord_array, map_array,
                           num_recs, poset);
    else
        perform_facet_index(zh, fi, no_ord, ord_array, map_array,
                            num_recs, poset, spec_list);
    yaz_timing_stop(timing);
    yaz_log(YLOG_LOG, "facet first phase real=%4.2f cat=%s",
            yaz_timing_get_real(timing),
            (cat == zinfo_index_category_sort) ? "sort" : "index");
    yaz_timing_start(timing);
    for (spec = spec_list, i = 0; i < no_ord; i++, spec = spec->next)
    {
        int j;
        NMEM nmem = nmem_create();
        struct term_collect *col;
        int no_collect_terms = 20;
        
        if (spec->extra)
            no_collect_terms = atoi(spec->extra);
        if (no_collect_terms < 1)
            no_collect_terms = 1;
        col = term_collect_create(map_array[i], no_collect_terms, nmem);
        term_collect_freq(zh, col, no_collect_terms, ord_array[i],
                          resultSetRef(zh, fi->setname), 
                          cat == zinfo_index_category_sort ? 1.0 : -1.0);
        
        if (use_xml)
            wrbuf_printf(wr, "  <facet type=\"%s\" index=\"%s\">\n",
                         spec->index_type, spec->index_name);
        else
            wrbuf_printf(wr, "facet %s %s\n",
                         spec->index_type, spec->index_name);
        for (j = 0; j < no_collect_terms; j++)
        {
            if (col[j].term)
            {
                char dst_buf[IT_MAX_WORD];
                zebra_term_untrans(zh, spec->index_type, dst_buf, col[j].term);
                if (use_xml)
                {
                    wrbuf_printf(wr, "    <term coccur=\"%d\"", col[j].oc);
                    if (col[j].set_occur)
                        wrbuf_printf(wr, " occur=\"" ZINT_FORMAT "\"", 
                                     col[j].set_occur);
                    wrbuf_printf(wr, ">");
                    wrbuf_xmlputs(wr, dst_buf);
                    wrbuf_printf(wr, "</term>\n");
                }
                else
                {
                    wrbuf_printf(wr, "term %d", col[j].oc);
                    if (col[j].set_occur)
                        wrbuf_printf(wr, " " ZINT_FORMAT, 
                                     col[j].set_occur);
                    wrbuf_printf(wr, ": %s\n", dst_buf);
                }
            }
        }
        if (use_xml)
            wrbuf_puts(wr, "  </facet>\n");
        nmem_destroy(nmem);
    }
    for (i = 0; i < no_ord; i++)
        zebra_strmap_destroy(map_array[i]);
    yaz_timing_stop(timing);
    yaz_log(YLOG_LOG, "facet second phase real=%4.2f",
            yaz_timing_get_real(timing));
    yaz_timing_destroy(&timing);
    return ret;
}

static int facet_fetch(
    struct special_fetch_s *fi, const char *elemsetname,
    const Odr_oid *input_format,
    const Odr_oid **output_format,
    WRBUF result, WRBUF addinfo)
{
    zint *pos_array;
    int i;
    int num_recs = 10; /* number of records to analyze */
    ZebraMetaRecord *poset;
    ZEBRA_RES ret = ZEBRA_OK;
    int *ord_array;
    int use_xml = 0;
    int no_ord = 0;
    struct index_spec *spec, *spec_list;
    int error;
    ZebraHandle zh = fi->zh;
    /* whether sort or index based */
    zinfo_index_category_t cat = zinfo_index_category_sort;

    /* see if XML is required for response */
    if (oid_oidcmp(input_format, yaz_oid_recsyn_xml) == 0)
        use_xml = 1;

    spec_list = parse_index_spec(elemsetname, fi->nmem, &error);
              
    if (!spec_list || error)
    {
        return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
    }          
  
    for (spec = spec_list; spec; spec = spec->next)
    {
        if (!spec->index_type)
            return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
        no_ord++;
    }

    /* try to see if all specs are sort based.. If not, try the
       index based ones */
    ord_array = nmem_malloc(fi->nmem, sizeof(*ord_array) * no_ord);

    for (spec = spec_list, i = 0; spec; spec = spec->next, i++)
    {
        int ord = zebraExplain_lookup_attr_str(zh->reg->zei,
                                               zinfo_index_category_sort,
                                               spec->index_type,
                                               spec->index_name);
        if (ord == -1)
            break;
        ord_array[i] = ord;
        num_recs = 10000;
    }
    if (spec)
    {
        cat = zinfo_index_category_index;
        for (spec = spec_list, i = 0; spec; spec = spec->next, i++)
        {
            int ord = zebraExplain_lookup_attr_str(zh->reg->zei,
                                                   zinfo_index_category_index,
                                                   spec->index_type,
                                                   spec->index_name);
            if (ord == -1)
                break;
            ord_array[i] = ord;
            
        }
    }
    if (spec)
        return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;

    res_get_int(zh->res, "facetNumRecs", &num_recs);

    pos_array = (zint *) nmem_malloc(fi->nmem, num_recs * sizeof(*pos_array));
    for (i = 0; i < num_recs; i++)
	pos_array[i] = i+1;
    poset = zebra_meta_records_create(zh, fi->setname, num_recs, pos_array);
    if (!poset)
    {
        wrbuf_puts(addinfo, fi->setname);
	return YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST;
    }
    else
    {
        if (use_xml)
        {
            wrbuf_printf(result, ZEBRA_XML_HEADER_STR ">\n");
        }
        ret = perform_facet(zh, fi, result, num_recs, poset,
                            spec_list, no_ord, ord_array, use_xml,
                            cat);
        if (use_xml)
            wrbuf_puts(result, "</record>\n");
    }
    *output_format = yaz_oid_recsyn_xml;
    zebra_meta_records_destroy(zh, poset, num_recs);
    return ret;
}


static int zebra_special_fetch(
    void *handle, const char *elemsetname,
    const Odr_oid *input_format,
    const Odr_oid **output_format,
    WRBUF result, WRBUF addinfo)
{
    Record rec = 0;
    struct special_fetch_s *fi = (struct special_fetch_s *) handle;
    ZebraHandle zh = fi->zh;
    zint sysno = fi->sysno;
    
    /* processing zebra::facet */
    if (elemsetname && 0 == strncmp(elemsetname, "facet", 5))
    {
        return facet_fetch(fi, elemsetname + 5, 
                           input_format, output_format,
                           result, addinfo);
    }

    if (elemsetname && 0 == strcmp(elemsetname, "snippet"))
    {
        return snippet_fetch(fi, elemsetname + 7,
                             input_format, output_format,
                             result, addinfo);
    }

    /* processing zebra::meta::sysno  */
    if (elemsetname && 0 == strcmp(elemsetname, "meta::sysno"))
    {
        int ret = 0;
        if (!oid_oidcmp(input_format, yaz_oid_recsyn_sutrs))
        {
            wrbuf_printf(result, ZINT_FORMAT, fi->sysno);
            *output_format = input_format;
        } 
        else if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
        {
            wrbuf_printf(result, ZEBRA_XML_HEADER_STR
                         " sysno=\"" ZINT_FORMAT "\"/>\n",
                         fi->sysno);
            *output_format = input_format;
        }
        else
            ret = YAZ_BIB1_NO_SYNTAXES_AVAILABLE_FOR_THIS_REQUEST;
        return ret;
    }

    /* processing special elementsetname zebra::index:: for sort elements */
    if (elemsetname && 0 == strncmp(elemsetname, "index", 5))
    {
        int ret = sort_fetch(
            fi, elemsetname + 5,
            input_format, output_format,
            result, addinfo);
        if (ret != -1)
            return ret;
        /* not a sort index so we continue to get the full record */
    }


    /* fetching binary record up for all other display elementsets */
    rec = rec_get(zh->reg->records, sysno);
    if (!rec)
    {
        yaz_log(YLOG_WARN, "rec_get fail on sysno=" ZINT_FORMAT, sysno);
        return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }

    /* processing special elementsetnames zebra::data */    
    if (elemsetname && 0 == strcmp(elemsetname, "data"))
    {
        struct ZebraRecStream stream;
        RecordAttr *recordAttr = rec_init_attr(zh->reg->zei, rec); 
        char *b;

        zebra_create_record_stream(zh, &rec, &stream);
        *output_format = input_format;

        b = nmem_malloc(fi->nmem, recordAttr->recordSize);
        stream.readf(&stream, b, recordAttr->recordSize);
        wrbuf_write(result, b, recordAttr->recordSize);

        stream.destroy(&stream);
        rec_free(&rec);
        return 0;
    }

    /* processing special elementsetnames zebra::meta:: */
    if (elemsetname && 0 == strcmp(elemsetname, "meta"))
    {
        int ret = 0;
        RecordAttr *recordAttr = rec_init_attr(zh->reg->zei, rec); 

        if (!oid_oidcmp(input_format, yaz_oid_recsyn_xml))
        {
            *output_format = input_format;
            
            wrbuf_printf(result, ZEBRA_XML_HEADER_STR
                         " sysno=\"" ZINT_FORMAT "\"", sysno);
            retrieve_puts_attr(result, "base", rec->info[recInfo_databaseName]);
            retrieve_puts_attr(result, "file", rec->info[recInfo_filename]);
            retrieve_puts_attr(result, "type", rec->info[recInfo_fileType]);
            if (fi->score >= 0)
                retrieve_puts_attr_int(result, "score", fi->score);
           
            wrbuf_printf(result,
                         " rank=\"" ZINT_FORMAT "\""
                         " size=\"%i\""
                         " set=\"zebra::%s\"/>\n",
                         recordAttr->staticrank,
                         recordAttr->recordSize,
                         elemsetname);
        }
        else if (!oid_oidcmp(input_format, yaz_oid_recsyn_sutrs))
        {
            *output_format = input_format;
            wrbuf_printf(result, "sysno " ZINT_FORMAT "\n", sysno);
            retrieve_puts_str(result, "base", rec->info[recInfo_databaseName]);
            retrieve_puts_str(result, "file", rec->info[recInfo_filename]);
            retrieve_puts_str(result, "type", rec->info[recInfo_fileType]);
            if (fi->score >= 0)
                retrieve_puts_int(result, "score", fi->score);

            wrbuf_printf(result,
                         "rank " ZINT_FORMAT "\n"
                         "size %i\n"
                         "set zebra::%s\n",
                         recordAttr->staticrank,
                         recordAttr->recordSize,
                         elemsetname);
        }
        else
            ret = YAZ_BIB1_NO_SYNTAXES_AVAILABLE_FOR_THIS_REQUEST;

        rec_free(&rec);
        return ret;
    }

    /* processing special elementsetnames zebra::index:: */
    if (elemsetname && 0 == strncmp(elemsetname, "index", 5))
    {
        int ret = special_index_fetch(
            fi, elemsetname + 5,
            input_format, output_format,
            result, addinfo, rec);
        rec_free(&rec);
        return ret;
    }

    if (rec)
        rec_free(&rec);
    return YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
}

int zebra_record_fetch(ZebraHandle zh, const char *setname,
                       zint sysno, int score,
                       ODR odr,
                       const Odr_oid *input_format, Z_RecordComposition *comp,
                       const Odr_oid **output_format,
                       char **rec_bufp, int *rec_lenp, char **basenamep,
                       WRBUF addinfo_w)
{
    Record rec;
    char *fname, *file_type, *basename;
    const char *elemsetname;
    struct ZebraRecStream stream;
    RecordAttr *recordAttr;
    void *clientData;
    int return_code = 0;
    zint sysnos[MAX_SYSNOS_PER_RECORD];
    int no_sysnos = MAX_SYSNOS_PER_RECORD;
    ZEBRA_RES res;
    struct special_fetch_s fetch_info;

    res = zebra_result_recid_to_sysno(zh, setname, sysno, sysnos, &no_sysnos);
    if (res != ZEBRA_OK)
        return ZEBRA_FAIL;

    sysno = sysnos[0];
    *basenamep = 0;
    elemsetname = yaz_get_esn(comp);

    fetch_info.zh = zh;
    fetch_info.setname = setname;
    fetch_info.sysno = sysno;
    fetch_info.score = score;
    fetch_info.nmem = odr->mem;

    /* processing zebra special elementset names of form 'zebra:: */
    if (elemsetname && 0 == strncmp(elemsetname, "zebra::", 7))
    {
        WRBUF result = wrbuf_alloc();
        int r = zebra_special_fetch(&fetch_info, elemsetname + 7,
                                    input_format, output_format,
                                    result, addinfo_w);
        if (r == 0)
        {
            *rec_bufp = odr_strdup(odr, wrbuf_cstr(result));
            *rec_lenp = wrbuf_len(result);
        }
        wrbuf_destroy(result);
        return r;
    }

    /* processing all other element set names */
    rec = rec_get(zh->reg->records, sysno);
    if (!rec)
    {
        yaz_log(YLOG_WARN, "rec_get fail on sysno=" ZINT_FORMAT, sysno);
        *basenamep = 0;
        return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }


    recordAttr = rec_init_attr(zh->reg->zei, rec);

    file_type = rec->info[recInfo_fileType];
    fname = rec->info[recInfo_filename];
    basename = rec->info[recInfo_databaseName];
    *basenamep = (char *) odr_malloc(odr, strlen(basename)+1);
    strcpy(*basenamep, basename);

    yaz_log(YLOG_DEBUG, "retrieve localno=" ZINT_FORMAT " score=%d",
            sysno, score);

    return_code = zebra_create_record_stream(zh, &rec, &stream);

    if (rec)
    {
        RecType rt;
        struct recRetrieveCtrl retrieveCtrl;

        retrieveCtrl.stream = &stream;
        retrieveCtrl.fname = fname;
        retrieveCtrl.localno = sysno;
        retrieveCtrl.staticrank = recordAttr->staticrank;
        retrieveCtrl.score = score;
        retrieveCtrl.recordSize = recordAttr->recordSize;
        retrieveCtrl.odr = odr;
        retrieveCtrl.input_format = retrieveCtrl.output_format = input_format;
        retrieveCtrl.comp = comp;
        retrieveCtrl.encoding = zh->record_encoding;
        retrieveCtrl.diagnostic = 0;
        retrieveCtrl.addinfo = 0;
        retrieveCtrl.dh = zh->reg->dh;
        retrieveCtrl.res = zh->res;
        retrieveCtrl.rec_buf = 0;
        retrieveCtrl.rec_len = -1;
        retrieveCtrl.handle = &fetch_info;
        retrieveCtrl.special_fetch = zebra_special_fetch;

        if (!(rt = recType_byName(zh->reg->recTypes, zh->res,
                                  file_type, &clientData)))
        {
            wrbuf_printf(addinfo_w, "Could not handle record type %.40s",
                         file_type);
            return_code = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
        }
        else
        {
            (*rt->retrieve)(clientData, &retrieveCtrl);
            return_code = retrieveCtrl.diagnostic;

            *output_format = retrieveCtrl.output_format;
            *rec_bufp = (char *) retrieveCtrl.rec_buf;
            *rec_lenp = retrieveCtrl.rec_len;
            if (retrieveCtrl.addinfo)
                wrbuf_puts(addinfo_w, retrieveCtrl.addinfo);
        }

        stream.destroy(&stream);
        rec_free(&rec);
    }

    return return_code;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

