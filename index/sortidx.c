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


#include <assert.h> 
#include <string.h>

#include <yaz/log.h>
#include <yaz/xmalloc.h>
#include <idzebra/isamb.h>
#include <idzebra/bfile.h>
#include <sortidx.h>
#include "recindex.h"

#define SORT_MAX_TERM 110
#define SORT_MAX_MULTI 4096

#define SORT_IDX_BLOCKSIZE 64

struct sort_term {
    zint sysno;
    zint section_id;
    zint length;
    char term[SORT_MAX_MULTI];
};


static void sort_term_log_item(int level, const void *b, const char *txt)
{
    struct sort_term a1;

    memcpy(&a1, b, sizeof(a1));

    yaz_log(level, "%s " ZINT_FORMAT " " ZINT_FORMAT " %.*s", txt, a1.sysno,
            a1.section_id, (int) a1.length-1, a1.term);
}

static int sort_term_compare(const void *a, const void *b)
{
    struct sort_term a1, b1;

    memcpy(&a1, a, sizeof(a1));
    memcpy(&b1, b, sizeof(b1));

    if (a1.sysno > b1.sysno)
        return 1;
    else if (a1.sysno < b1.sysno)
        return -1;
    if (a1.section_id > b1.section_id)
        return 1;
    else if (a1.section_id < b1.section_id)
        return -1;

    return 0;
}

static void *sort_term_code_start(void)
{
    return 0;
}

static void sort_term_encode1(void *p, char **dst, const char **src)
{
    struct sort_term a1;

    memcpy(&a1, *src, sizeof(a1));
    *src += sizeof(a1);

    zebra_zint_encode(dst, a1.sysno); /* encode record id */
    strcpy(*dst, a1.term); /* then sort term, 0 terminated */
    *dst += strlen(a1.term) + 1;
}

static void sort_term_encode2(void *p, char **dst, const char **src)
{
    struct sort_term a1;

    memcpy(&a1, *src, sizeof(a1));
    *src += sizeof(a1);

    zebra_zint_encode(dst, a1.sysno);
    zebra_zint_encode(dst, a1.section_id);
    zebra_zint_encode(dst, a1.length); /* encode length */
    memcpy(*dst, a1.term, a1.length);
    *dst += a1.length;
}

static void sort_term_decode1(void *p, char **dst, const char **src)
{
    struct sort_term a1;
    size_t slen;

    zebra_zint_decode(src, &a1.sysno);
    a1.section_id = 0;

    strcpy(a1.term, *src);
    slen = 1 + strlen(a1.term);
    *src += slen;
    a1.length = slen;

    memcpy(*dst, &a1, sizeof(a1));
    *dst += sizeof(a1);
}

static void sort_term_decode2(void *p, char **dst, const char **src)
{
    struct sort_term a1;

    zebra_zint_decode(src, &a1.sysno);
    zebra_zint_decode(src, &a1.section_id);
    zebra_zint_decode(src, &a1.length);

    memcpy(a1.term, *src, a1.length);
    *src += a1.length;

    memcpy(*dst, &a1, sizeof(a1));
    *dst += sizeof(a1);
}

static void sort_term_code_reset(void *p)
{
}

static void sort_term_code_stop(void *p)
{
}

struct sort_term_stream {
    int no;
    int insert_flag;
    struct sort_term st;
};

static int sort_term_code_read(void *vp, char **dst, int *insertMode)
{
    struct sort_term_stream *s = (struct sort_term_stream *) vp;

    if (s->no == 0)
        return 0;

    (s->no)--;
    
    *insertMode = s->insert_flag;
    memcpy(*dst, &s->st, sizeof(s->st));
    *dst += sizeof(s->st);
    return 1;
}

struct sortFileHead {
    zint sysno_max;
};

struct sortFile {
    int id;
    union {
        BFile bf;
        ISAMB isamb;
    } u;
    ISAM_P isam_p;
    ISAMB_PP isam_pp;
    struct sortFile *next;
    struct sortFileHead head;
    int no_inserted;
    int no_deleted;
};

struct zebra_sort_index {
    BFiles bfs;
    int write_flag;
    zint sysno;
    int type;
    char *entry_buf;
    struct sortFile *current_file;
    struct sortFile *files;
};

zebra_sort_index_t zebra_sort_open(BFiles bfs, int write_flag, int type)
{
    zebra_sort_index_t si = (zebra_sort_index_t) xmalloc(sizeof(*si));
    si->bfs = bfs;
    si->write_flag = write_flag;
    si->current_file = NULL;
    si->files = NULL;
    si->type = type;
    si->entry_buf = (char *) xmalloc(SORT_IDX_ENTRYSIZE);
    return si;
}

void zebra_sort_close(zebra_sort_index_t si)
{
    struct sortFile *sf = si->files;
    while (sf)
    {
	struct sortFile *sf_next = sf->next;
        switch(si->type)
        {
        case ZEBRA_SORT_TYPE_FLAT:
	    bf_close(sf->u.bf);
            break;
        case ZEBRA_SORT_TYPE_ISAMB:
        case ZEBRA_SORT_TYPE_MULTI:
            if (sf->isam_pp)
                isamb_pp_close(sf->isam_pp);
            isamb_set_root_ptr(sf->u.isamb, sf->isam_p);
            isamb_close(sf->u.isamb);
            break;
        }
	xfree(sf);
	sf = sf_next;
    }
    xfree(si->entry_buf);
    xfree(si);
}

int zebra_sort_type(zebra_sort_index_t si, int id)
{
    int isam_block_size = 4096;

    ISAMC_M method;
    char fname[80];
    struct sortFile *sf;

    method.compare_item = sort_term_compare;
    method.log_item = sort_term_log_item;
    method.codec.reset = sort_term_code_reset;
    method.codec.start = sort_term_code_start;
    method.codec.stop = sort_term_code_stop;

    if (si->current_file && si->current_file->id == id)
	return 0;
    for (sf = si->files; sf; sf = sf->next)
	if (sf->id == id)
	{
	    si->current_file = sf;
	    return 0;
	}
    sf = (struct sortFile *) xmalloc(sizeof(*sf));
    sf->id = id;

    switch(si->type)
    {
    case ZEBRA_SORT_TYPE_FLAT:
        sf->u.bf = NULL;
        sprintf(fname, "sort%d", id);
        yaz_log(YLOG_DEBUG, "sort idx %s wr=%d", fname, si->write_flag);
        sf->u.bf = bf_open(si->bfs, fname, SORT_IDX_BLOCKSIZE, si->write_flag);
        if (!sf->u.bf)
        {
            xfree(sf);
            return -1;
        }
        if (!bf_read(sf->u.bf, 0, 0, sizeof(sf->head), &sf->head))
        {
            sf->head.sysno_max = 0;
            if (!si->write_flag)
            {
                bf_close(sf->u.bf);
                xfree(sf);
                return -1;
            }
        }
        break;
    case ZEBRA_SORT_TYPE_ISAMB:
        method.codec.encode = sort_term_encode1;
        method.codec.decode = sort_term_decode1;
        
        sprintf(fname, "sortb%d", id);
        sf->u.isamb = isamb_open2(si->bfs, fname, si->write_flag, &method,
                                  /* cache */ 0,
                                  /* no_cat */ 1, &isam_block_size,
                                  /* use_root_ptr */ 1);
        if (!sf->u.isamb)
        {
            xfree(sf);
            return -1;
        }
        else
        {
            sf->isam_p = isamb_get_root_ptr(sf->u.isamb);
        }
        break;
    case ZEBRA_SORT_TYPE_MULTI:
        isam_block_size = 32768;
        method.codec.encode = sort_term_encode2;
        method.codec.decode = sort_term_decode2;
        
        sprintf(fname, "sortm%d", id);
        sf->u.isamb = isamb_open2(si->bfs, fname, si->write_flag, &method,
                                  /* cache */ 0,
                                  /* no_cat */ 1, &isam_block_size,
                                  /* use_root_ptr */ 1);
        if (!sf->u.isamb)
        {
            xfree(sf);
            return -1;
        }
        else
        {
            sf->isam_p = isamb_get_root_ptr(sf->u.isamb);
        }
        break;
    }
    sf->isam_pp = 0;
    sf->no_inserted = 0;
    sf->no_deleted = 0;
    sf->next = si->files;
    si->current_file = si->files = sf;
    return 0;
}

static void zebra_sortf_rewind(struct sortFile *sf)
{
    if (sf->isam_pp)
        isamb_pp_close(sf->isam_pp);
    sf->isam_pp = 0;
    sf->no_inserted = 0;
    sf->no_deleted = 0;
}

void zebra_sort_sysno(zebra_sort_index_t si, zint sysno)
{
    zint new_sysno = rec_sysno_to_int(sysno);
    struct sortFile *sf;

    for (sf = si->files; sf; sf = sf->next)
    {
        if (sf->no_inserted || sf->no_deleted)
            zebra_sortf_rewind(sf);
        else if (sf->isam_pp && new_sysno <= si->sysno)
            zebra_sortf_rewind(sf);
    }
    si->sysno = new_sysno;
}


void zebra_sort_delete(zebra_sort_index_t si, zint section_id)
{
    struct sortFile *sf = si->current_file;

    if (!sf || !sf->u.bf)
        return;
    switch(si->type)
    {
    case ZEBRA_SORT_TYPE_FLAT:
        memset(si->entry_buf, 0, SORT_IDX_ENTRYSIZE);
        bf_write(sf->u.bf, si->sysno+1, 0, 0, si->entry_buf);
        break;
    case ZEBRA_SORT_TYPE_ISAMB:
    case ZEBRA_SORT_TYPE_MULTI:
        assert(sf->u.isamb);
        if (sf->no_deleted == 0)
        {
            struct sort_term_stream s;
            ISAMC_I isamc_i;

            s.st.sysno = si->sysno;
            s.st.section_id = section_id;
            s.st.length = 0;
            s.st.term[0] = '\0';
            
            s.no = 1;
            s.insert_flag = 0;
            isamc_i.clientData = &s;
            isamc_i.read_item = sort_term_code_read;
            
            isamb_merge(sf->u.isamb, &sf->isam_p, &isamc_i);
            sf->no_deleted++;
        }
        break;
    }
}

void zebra_sort_add(zebra_sort_index_t si, zint section_id, WRBUF wrbuf)
{
    struct sortFile *sf = si->current_file;
    int len;

    if (!sf || !sf->u.bf)
        return;
    switch(si->type)
    {
    case ZEBRA_SORT_TYPE_FLAT:
        /* take first entry from wrbuf - itself is 0-terminated */
        len = strlen(wrbuf_buf(wrbuf));
        if (len > SORT_IDX_ENTRYSIZE)
            len = SORT_IDX_ENTRYSIZE;
        
        memcpy(si->entry_buf, wrbuf_buf(wrbuf), len);
        if (len < SORT_IDX_ENTRYSIZE-len)
            memset(si->entry_buf+len, 0, SORT_IDX_ENTRYSIZE-len);
        bf_write(sf->u.bf, si->sysno+1, 0, 0, si->entry_buf);
        break;
    case ZEBRA_SORT_TYPE_ISAMB:
        assert(sf->u.isamb);

        if (sf->no_inserted == 0)
        {
            struct sort_term_stream s;
            ISAMC_I isamc_i;
            /* take first entry from wrbuf - itself is 0-terminated */

            len = wrbuf_len(wrbuf);
            if (len > SORT_MAX_TERM)
            {
                len = SORT_MAX_TERM;
                wrbuf_buf(wrbuf)[len-1] = '\0';
            }
            memcpy(s.st.term, wrbuf_buf(wrbuf), len);
            s.st.length = len;
            s.st.sysno = si->sysno;
            s.st.section_id = 0;
            s.no = 1;
            s.insert_flag = 1;
            isamc_i.clientData = &s;
            isamc_i.read_item = sort_term_code_read;
            
            isamb_merge(sf->u.isamb, &sf->isam_p, &isamc_i);
            sf->no_inserted++;
        }
        break;
    case ZEBRA_SORT_TYPE_MULTI:
        assert(sf->u.isamb);
        if (sf->no_inserted == 0)
        {
            struct sort_term_stream s;
            ISAMC_I isamc_i;
            len = wrbuf_len(wrbuf);
            if (len > SORT_MAX_MULTI)
            {
                len = SORT_MAX_MULTI;
                wrbuf_buf(wrbuf)[len-1] = '\0';
            }
            memcpy(s.st.term, wrbuf_buf(wrbuf), len);
            s.st.length = len;
            s.st.sysno = si->sysno;
            s.st.section_id = section_id;
            s.no = 1;
            s.insert_flag = 1;
            isamc_i.clientData = &s;
            isamc_i.read_item = sort_term_code_read;
            
            isamb_merge(sf->u.isamb, &sf->isam_p, &isamc_i);
            sf->no_inserted++;
        }
        break;
    }
}


int zebra_sort_read(zebra_sort_index_t si, zint *section_id, WRBUF w)
{
    int r;
    struct sortFile *sf = si->current_file;
    char tbuf[SORT_IDX_ENTRYSIZE];

    assert(sf);
    assert(sf->u.bf);

    switch(si->type)
    {
    case ZEBRA_SORT_TYPE_FLAT:
        r = bf_read(sf->u.bf, si->sysno+1, 0, 0, tbuf);
        if (r && *tbuf)
        {
            wrbuf_puts(w, tbuf);
            wrbuf_putc(w, '\0');
            return 1;
        }
        break;
    case ZEBRA_SORT_TYPE_ISAMB:
    case ZEBRA_SORT_TYPE_MULTI:
        if (sf->isam_p)
        {

            if (!sf->isam_pp)
                sf->isam_pp = isamb_pp_open(sf->u.isamb, sf->isam_p, 1);
            if (sf->isam_pp)
            {
                struct sort_term st, st_untilbuf;

                st_untilbuf.sysno = si->sysno;
                st_untilbuf.section_id = 0;
                st_untilbuf.length = 0;
                st_untilbuf.term[0] = '\0';
                r = isamb_pp_forward(sf->isam_pp, &st, &st_untilbuf);
                if (r && st.sysno == si->sysno)
                {
                    wrbuf_write(w, st.term, st.length);
                    if (section_id)
                        *section_id = st.section_id;
                    return 1;
                }
            }
        }
        break;
    }
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

