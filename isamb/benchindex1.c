/* $Id: benchindex1.c,v 1.5 2006-12-12 13:51:23 adam Exp $
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

#include <yaz/options.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <yaz/log.h>
#include <yaz/nmem.h>
#include <yaz/xmalloc.h>
#include <yaz/marcdisp.h>
#include <it_key.h>
#include <idzebra/isamb.h>
#include <idzebra/dict.h>
#include <assert.h>

struct index_block {
    NMEM nmem;
    int no_entries;
    size_t current_entry;
    size_t current_max;
    struct index_term *terms;
    struct index_term **ar;
    int round;
};

struct index_term {
    const char *term;
    zint docid;
    zint seqno;
    int word_id;
    struct index_term *next;
};

struct index_block *index_block_new(int memory)
{
    struct index_block *b = xmalloc(sizeof(*b));
    b->no_entries = 0;
    b->current_max = memory * 1024 * 1024;
    b->terms = 0;
    b->nmem = nmem_create();
    b->round = 0;
    return b;
}

void index_block_destroy(struct index_block **bp)
{
    if (*bp)
    {
        nmem_destroy((*bp)->nmem);
        xfree(*bp);
        *bp = 0;
    }
}

static int cmp_ar(const void *p1, const void *p2)
{
    struct index_term *t1 = *(struct index_term **) p1;
    struct index_term *t2 = *(struct index_term **) p2;
    int d = strcmp(t1->term, t2->term);
    if (d)
        return d;

    if (t1->docid > t2->docid)
        return 1;
    else if (t1->docid < t2->docid)
        return -1;
    if (t1->seqno > t2->seqno)
        return 1;
    else if (t1->seqno < t2->seqno)
        return -1;
    return 0;
}


int code_read(void *vp, char **dst, int *insertMode)
{
    struct index_block *b = (struct index_block *)vp;
    struct index_term *t;
    struct it_key key;

    if (b->current_entry >= b->no_entries)
	return 0;
    
    t = b->ar[b->current_entry];
    b->current_entry++;
    
    key.len = 3;
    key.mem[0] = t->word_id;
    key.mem[1] = t->docid;
    key.mem[2] = t->seqno;
    key.mem[3] = 0;

    memcpy(*dst, &key, sizeof(key));

    (*dst) += sizeof(key);
    *insertMode = 1;
#if 0
    yaz_log(YLOG_LOG, "returning " ZINT_FORMAT " " ZINT_FORMAT "\n",
            key.mem[0], key.mem[1]);
#endif
    return 1;
}

void index_block_flush(struct index_block *b, ISAMB isb, Dict dict,
                       int no_docs)
{
    struct index_term *t = b->terms;
    int i;
    int word_id_seq = 0;
    int no_words = 0, no_new_words = 0;
    const char *dict_info = 0;
    ISAM_P isamc_p = 0;
    zebra_timing_t tim_dict = 0;
    zebra_timing_t tim_isamb = 0;
    zint number_of_int_splits = isamb_get_int_splits(isb);
    zint number_of_leaf_splits = isamb_get_leaf_splits(isb);
    zint number_of_dict_splits = dict_get_no_split(dict);
    
    b->ar = xmalloc(sizeof(*b->ar) * b->no_entries);
    for (i = 0; i < b->no_entries; i++, t = t->next)
    {
        assert(t);
        b->ar[i] = t;
    }
    assert(!t);
    
    qsort(b->ar, b->no_entries, sizeof(*b->ar), cmp_ar);
    tim_dict = zebra_timing_create();
#if 0
    for (i = 0; i < b->no_entries; i++)
    {
        printf("%s " ZINT_FORMAT " " ZINT_FORMAT "\n",
               ar[i]->term, ar[i]->docid, ar[i]->seqno);
    }
#endif
    dict_info = dict_lookup(dict, "_w");
    if (dict_info)
    {
        assert(*dict_info == sizeof(word_id_seq));
        memcpy(&word_id_seq, dict_info+1, sizeof(word_id_seq));
    }

    dict_info = dict_lookup(dict, "_i");
    if (dict_info)
    {
        assert(*dict_info == sizeof(isamc_p));
        memcpy(&isamc_p, dict_info+1, sizeof(isamc_p));
    }

    for (i = 0; i < b->no_entries; i++)
    {
        if (i > 0 && strcmp(b->ar[i-1]->term, b->ar[i]->term) == 0)
            b->ar[i]->word_id = b->ar[i-1]->word_id;
        else
        {
            const char *dict_info = dict_lookup(dict, b->ar[i]->term);
            if (dict_info)
            {
                memcpy(&b->ar[i]->word_id, dict_info+1, sizeof(int));
            }
            else
            {
                word_id_seq++;
                no_new_words++;
                dict_insert(dict, b->ar[i]->term, sizeof(int), &word_id_seq);
                b->ar[i]->word_id = word_id_seq;
            }
            no_words++;
        }
    }
    dict_insert(dict, "_w", sizeof(word_id_seq), &word_id_seq);
    
    zebra_timing_stop(tim_dict);
    tim_isamb = zebra_timing_create();

    b->current_entry = 0;

    if (b->no_entries)
    {
        ISAMC_I isamc_i;
        
        isamc_i.clientData = b;
        isamc_i.read_item = code_read;

        isamb_merge (isb, &isamc_p, &isamc_i);

        assert(isamc_p);
        dict_insert(dict, "_i", sizeof(isamc_p), &isamc_p);
    }

    zebra_timing_stop(tim_isamb);

    number_of_int_splits = isamb_get_int_splits(isb) - number_of_int_splits;
    number_of_leaf_splits = isamb_get_leaf_splits(isb) - number_of_leaf_splits;
    number_of_dict_splits = dict_get_no_split(dict) - number_of_dict_splits;

    if (b->round == 0)
    {
        printf("# run     total dict-real  user   sys isam-real  user   sys "
               " intsp leafsp  docs postings  words    new d-spl\n");
    }
    b->round++;
    printf("%5d %9.6f %9.6f %5.2f %5.2f %9.6f %5.2f %5.2f "
           "%6" ZINT_FORMAT0 " %6" ZINT_FORMAT0 
           " %5d %8d %6d %6d" " %5" ZINT_FORMAT0 "\n",
           b->round,
           zebra_timing_get_real(tim_dict) + zebra_timing_get_real(tim_isamb),
           zebra_timing_get_real(tim_dict),
           zebra_timing_get_user(tim_dict),
           zebra_timing_get_sys(tim_dict),
           zebra_timing_get_real(tim_isamb),
           zebra_timing_get_user(tim_isamb),
           zebra_timing_get_sys(tim_isamb),
           number_of_int_splits,
           number_of_leaf_splits,
           no_docs,
           b->no_entries,
           no_words,
           no_new_words,
           number_of_dict_splits
        );
    fflush(stdout);

    xfree(b->ar);
    b->ar = 0;
    nmem_reset(b->nmem);
    b->no_entries = 0;
    b->terms = 0;

    zebra_timing_destroy(&tim_isamb);
    zebra_timing_destroy(&tim_dict);
}

void index_block_check_flush(struct index_block *b, ISAMB isb, Dict dict,
                             int no_docs)
{
    int total = nmem_total(b->nmem);
    int max = b->current_max;
    if (total > max)
    {
        index_block_flush(b, isb, dict, no_docs);
    }
}

void index_block_add(struct index_block *b,
                     const char *term, zint docid, zint seqno)
{
    struct index_term *t = nmem_malloc(b->nmem, sizeof(*t));
    t->term = nmem_strdup(b->nmem, term);
    t->docid = docid;
    t->seqno = seqno;
    t->next = b->terms;
    b->terms = t;
    b->no_entries++;
}

void index_term(struct index_block *b, const char *term,
                zint docid, zint *seqno)
{
#if 0
    printf("%s " ZINT_FORMAT " " ZINT_FORMAT "\n", term,
           docid, *seqno);
#endif
    index_block_add(b, term, docid, *seqno);
    (*seqno)++;
}

void index_wrbuf(struct index_block *b, WRBUF wrbuf, zint docid,
                 int subfield_char)
{
    int nl = 1;
    const char *cp = wrbuf_buf(wrbuf);
    char term[4096];
    size_t sz = 0;
    zint seqno = 0;

    while (*cp)
    {
        if (nl)
        {
            int i;
            if (cp[0] != ' ')
            {   /* skip field+indicator (e.g. 245 00) */
                for (i = 0; i<6 && *cp; i++, cp++)
                    ;
            }
            else
            {  /* continuation line */
                for (i = 0; i<4 && *cp; i++, cp++)
                    ;
            }   
        }
        nl = 0;
        if (*cp == '\n')
        {
            if (sz)
            {
                index_term(b, term, docid, &seqno);
                sz = 0;
            }
            nl = 1;
            cp++;
        }
        else if (*cp == subfield_char && cp[1])
        {
            if (sz)
            {
                index_term(b, term, docid, &seqno);
                sz = 0;
            }
            cp += 2;
        }
        else if (strchr("$*/-;,.:[]\"&(){} ", *cp))
        {
            if (sz)
            {
                index_term(b, term, docid, &seqno);
                sz = 0;
            }
            cp++;
        }
        else
        {
            unsigned ch = *(const unsigned char *)cp;
            if (sz < sizeof(term))
            {
                term[sz] = tolower(ch);
                term[sz+1] = '\0';
                sz++;
            }
            cp++;
        }            
    }
    if (sz)
        index_term(b, term, docid, &seqno);
}

void index_marc_line_records(ISAMB isb,
                             Dict dict,
                             zint *docid_seq,
                             FILE *inf,
                             int memory)
{
    WRBUF wrbuf = wrbuf_alloc();
    int no_docs = 0;
    int new_rec = 1;
    char line[4096];
    struct index_block *b = index_block_new(memory);
    while(fgets(line, sizeof(line)-1, inf))
    {
        if (line[0] == '$')
        {
            if (!new_rec)
                new_rec = 1;
            else
                new_rec = 0;
            continue;
        }
        if (new_rec)
        {
            (*docid_seq)++;
            no_docs++;
            index_block_check_flush(b, isb, dict, no_docs);
            new_rec = 0;
        }

        if (line[0] == ' ')
        {
            /* continuation */
            wrbuf_puts(wrbuf, line);
            continue;
        }
        else
        {
            /* index existing buffer (if any) */
            if (wrbuf_len(wrbuf))
            {
                index_wrbuf(b, wrbuf, *docid_seq, '*');
                wrbuf_rewind(wrbuf);
            }
            if (line[0] != ' ' && line[1] != ' ' && line[2] != ' ' &&
                line[3] == ' ')
            {
                /* normal field+indicator line */
                wrbuf_puts(wrbuf, line);
            }
        }
    }
    if (wrbuf_len(wrbuf))
    {
        index_wrbuf(b, wrbuf, *docid_seq, '*');
        wrbuf_rewind(wrbuf);
    }
    (*docid_seq)++;
    no_docs++;
    index_block_flush(b, isb, dict, no_docs);
    index_block_destroy(&b);
}

void index_marc_from_file(ISAMB isb,
                          Dict dict,
                          zint *docid_seq,
                          FILE *inf,
                          int memory,
                          int verbose, int print_offset)
{
    yaz_marc_t mt = yaz_marc_create();
    WRBUF wrbuf = wrbuf_alloc();
    struct index_block *b = index_block_new(memory);
    int no_docs = 0;

    while (1)
    {
        size_t r;
        char buf[100001];
        int len, rlen;

        r = fread (buf, 1, 5, inf);
        if (r < 5)
        {
            if (r && print_offset && verbose)
                printf ("<!-- Extra %ld bytes at end of file -->\n",
                        (long) r);
            break;
        }
        while (*buf < '0' || *buf > '9')
        {
            int i;
            long off = ftell(inf) - 5;
            if (verbose || print_offset)
                printf("<!-- Skipping bad byte %d (0x%02X) at offset "
                       "%ld (0x%lx) -->\n", 
                       *buf & 0xff, *buf & 0xff,
                       off, off);
            for (i = 0; i<4; i++)
                buf[i] = buf[i+1];
            r = fread(buf+4, 1, 1, inf);
            if (r < 1)
                break;
        }
        if (r < 1)
        {
            if (verbose || print_offset)
                printf ("<!-- End of file with data -->\n");
            break;
        }
        len = atoi_n(buf, 5);
        if (len < 25 || len > 100000)
        {
            long off = ftell(inf) - 5;
            printf("Bad Length %ld read at offset %ld (%lx)\n",
                   (long)len, (long) off, (long) off);
            break;
        }
        rlen = len - 5;
        r = fread (buf + 5, 1, rlen, inf);
        if (r < rlen)
            break;
        yaz_marc_read_iso2709(mt, buf, len);
        
        if (yaz_marc_write_line(mt, wrbuf))
            break;

        index_wrbuf(b, wrbuf, *docid_seq, '$');
        wrbuf_rewind(wrbuf);
        (*docid_seq)++;

        no_docs++;
        index_block_check_flush(b, isb, dict, no_docs);
    }
    index_block_flush(b, isb, dict, no_docs);
    wrbuf_free(wrbuf, 1);
    yaz_marc_destroy(mt);
    index_block_destroy(&b);
}

void exit_usage(void)
{
    fprintf(stderr, "benchindex1 [-t type] [-c d:i] [-m mem] [-i] [inputfile]\n");
    exit(1);
}

int main(int argc, char **argv)
{
    BFiles bfs;
    ISAMB isb;
    ISAMC_M method;
    Dict dict;
    int ret;
    int reset = 0;
    char *arg;
    int memory = 5;
    int isam_cache_size = 40;
    int dict_cache_size = 50;
    const char *fname = 0;
    FILE *inf = stdin;
    zebra_timing_t tim = 0;
    zint docid_seq = 1;
    const char *dict_info;
    const char *type = "iso2709";
    int int_count_enable = 1;

    while ((ret = options("im:t:c:N", argv, argc, &arg)) != -2)
    {
        switch(ret)
        {
        case 'm':
            memory = atoi(arg);
            break;
        case 'i':
            reset = 1;
            break;
        case 't':
            if (!strcmp(arg, "iso2709"))
                type = "iso2709";
            else if (!strcmp(arg, "line"))
                type = "line";
            else
            {
                fprintf(stderr, "bad type: %s.\n", arg);
                exit_usage();
            }
            break;
        case 'c':
            if (sscanf(arg, "%d:%d", &dict_cache_size, &isam_cache_size) 
                != 2)
            {
                fprintf(stderr, "bad cache sizes for -c\n");
                exit_usage();
            }
            break;
        case 0:
            fname = arg;
            break;
        case 'N':
            int_count_enable = 0;
            break;
        default:
            fprintf(stderr, "bad option.\n");
            exit_usage();
        }
    }
	
    if (fname)
    {
        inf = fopen(fname, "rb");
        if (!inf)
        {
            fprintf(stderr, "Cannot open %s\n", fname);
            exit(1);
        }
    }
    printf("# benchindex1 %s %s\n", __DATE__, __TIME__);
    printf("# isam_cache_size = %d\n", isam_cache_size);
    printf("# dict_cache_size = %d\n", dict_cache_size);
    printf("# int_count_enable = %d\n", int_count_enable);
    printf("# memory = %d\n", memory);
    
    /* setup method (attributes) */
    method.compare_item = key_compare;
    method.log_item = key_logdump_txt;

    method.codec.start = iscz1_start;
    method.codec.decode = iscz1_decode;
    method.codec.encode = iscz1_encode;
    method.codec.stop = iscz1_stop;
    method.codec.reset = iscz1_reset;

    method.debug = 0;

    /* create block system */
    bfs = bfs_create(0, 0);
    if (!bfs)
    {
	yaz_log(YLOG_WARN, "bfs_create failed");
	exit(1);
    }

    if (reset)
        bf_reset(bfs);

    tim = zebra_timing_create();
    /* create isam handle */
    isb = isamb_open (bfs, "isamb", isam_cache_size ? 1 : 0, &method, 0);
    if (!isb)
    {
	yaz_log(YLOG_WARN, "isamb_open failed");
	exit(2);
    }
    isamb_set_cache_size(isb, isam_cache_size);
    isamb_set_int_count(isb, int_count_enable);
    dict = dict_open(bfs, "dict", dict_cache_size, 1, 0, 4096);

    dict_info = dict_lookup(dict, "_s");
    if (dict_info)
    {
        assert(*dict_info == sizeof(docid_seq));
        memcpy(&docid_seq, dict_info+1, sizeof(docid_seq));
    }

    if (!strcmp(type, "iso2709"))
        index_marc_from_file(isb, dict, &docid_seq, inf, memory, 
                             0 /* verbose */ , 0 /* print_offset */);
    else if (!strcmp(type, "line"))
        index_marc_line_records(isb, dict, &docid_seq, inf, memory);

    printf("# Total " ZINT_FORMAT " documents\n", docid_seq);
    dict_insert(dict, "_s", sizeof(docid_seq), &docid_seq);

    dict_close(dict);
    isamb_close(isb);

    if (fname)
        fclose(inf);
    /* exit block system */
    bfs_destroy(bfs);
    zebra_timing_stop(tim);

    printf("# Total timings real=%8.6f user=%3.2f system=%3.2f\n",
            zebra_timing_get_real(tim),
            zebra_timing_get_user(tim),
            zebra_timing_get_sys(tim));
    
    zebra_timing_destroy(&tim);

    exit(0);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

