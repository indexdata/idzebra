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
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <idzebra/isamb.h>
#include <yaz/yaz-util.h>
#include "recindex.h"

#define RIDX_CHUNK 128


struct recindex {
    char *index_fname;
    BFile index_BFile;
    ISAMB isamb;
    ISAM_P isam_p;
};

struct record_index_entry {
    zint next;         /* first block of record info / next free entry */
    int size;          /* size of record or 0 if free entry */
} ent;


static void rect_log_item(int level, const void *b, const char *txt)
{
    zint sys;
    int len;


    memcpy(&sys, b, sizeof(sys));
    len = ((const char *) b)[sizeof(sys)];

    if (len == sizeof(struct record_index_entry))
    {
        memcpy(&ent, (const char *)b + sizeof(sys) + 1, len);
        yaz_log(YLOG_LOG, "%s " ZINT_FORMAT " next=" ZINT_FORMAT " sz=%d", txt, sys,
                ent.next, ent.size);

    }
    else
        yaz_log(YLOG_LOG, "%s " ZINT_FORMAT, txt, sys);
}

int rect_compare(const void *a, const void *b)
{
    zint s_a, s_b;

    memcpy(&s_a, a, sizeof(s_a));
    memcpy(&s_b, b, sizeof(s_b));

    if (s_a > s_b)
        return 1;
    else if (s_a < s_b)
        return -1;
    return 0;
}

void *rect_code_start(void)
{
    return 0;
}

void rect_encode(void *p, char **dst, const char **src)
{
    zint sys;
    int len;

    memcpy(&sys, *src, sizeof(sys));
    zebra_zint_encode(dst, sys);
    (*src) += sizeof(sys);

    len = **src;
    **dst = len;
    (*src)++;
    (*dst)++;

    memcpy(*dst, *src, len);
    *dst += len;
    *src += len;
}

void rect_decode(void *p, char **dst, const char **src)
{
    zint sys;
    int len;

    zebra_zint_decode(src, &sys);
    memcpy(*dst, &sys, sizeof(sys));
    *dst += sizeof(sys);

    len = **src;
    **dst = len;
    (*src)++;
    (*dst)++;

    memcpy(*dst, *src, len);
    *dst += len;
    *src += len;
}

void rect_code_reset(void *p)
{
}

void rect_code_stop(void *p)
{
}


recindex_t recindex_open(BFiles bfs, int rw, int use_isamb)
{
    recindex_t p = xmalloc(sizeof(*p));
    p->index_BFile = 0;
    p->isamb = 0;

    p->index_fname = "reci";
    p->index_BFile = bf_open(bfs, p->index_fname, RIDX_CHUNK, rw);
    if (p->index_BFile == NULL)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "open %s", p->index_fname);
        xfree(p);
        return 0;
    }

    if (use_isamb)
    {
        int isam_block_size = 4096;
        ISAMC_M method;

        method.compare_item = rect_compare;
        method.log_item = rect_log_item;
        method.codec.start = rect_code_start;
        method.codec.encode = rect_encode;
        method.codec.decode = rect_decode;
        method.codec.reset = rect_code_reset;
        method.codec.stop = rect_code_stop;

        p->index_fname = "rect";
        p->isamb = isamb_open2(bfs, p->index_fname, rw, &method,
                               /* cache */ 0,
                               /* no_cat */ 1, &isam_block_size,
                               /* use_root_ptr */ 1);

        p->isam_p = 0;
        if (p->isamb)
            p->isam_p = isamb_get_root_ptr(p->isamb);

    }
    return p;
}

static void log_pr(const char *txt)
{
    yaz_log(YLOG_LOG, "%s", txt);
}


void recindex_close(recindex_t p)
{
    if (p)
    {
        if (p->index_BFile)
            bf_close(p->index_BFile);
        if (p->isamb)
        {
            isamb_set_root_ptr(p->isamb, p->isam_p);
            isamb_dump(p->isamb, p->isam_p, log_pr);
            isamb_close(p->isamb);
        }
        xfree(p);
    }
}

int recindex_read_head(recindex_t p, void *buf)
{
    return bf_read(p->index_BFile, 0, 0, 0, buf);
}

const char *recindex_get_fname(recindex_t p)
{
    return p->index_fname;
}

ZEBRA_RES recindex_write_head(recindex_t p, const void *buf, size_t len)
{
    int r;

    assert(p);

    assert(p->index_BFile);

    r = bf_write(p->index_BFile, 0, 0, len, buf);
    if (r)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "write head of %s", p->index_fname);
        return ZEBRA_FAIL;
    }
    return ZEBRA_OK;
}

int recindex_read_indx(recindex_t p, zint sysno, void *buf, int itemsize,
                       int ignoreError)
{
    int r = 0;
    if (p->isamb)
    {
        if (p->isam_p)
        {
            char item[256];
            char *st = item;
            char untilbuf[sizeof(zint) + 1];

            ISAMB_PP isam_pp = isamb_pp_open(p->isamb, p->isam_p, 1);

            memcpy(untilbuf, &sysno, sizeof(sysno));
            untilbuf[sizeof(sysno)] = 0;
            r = isamb_pp_forward(isam_pp, st, untilbuf);

            isamb_pp_close(isam_pp);
            if (!r)
                return 0;

            if (item[sizeof(sysno)] != itemsize)
            {
                yaz_log(YLOG_WARN, "unexpected entry size %d != %d",
                        item[sizeof(sysno)], itemsize);
                return 0;
            }
            memcpy(buf, item + sizeof(sysno) + 1, itemsize);
        }
    }
    else
    {
        zint pos = (sysno-1)*itemsize;
        int off = CAST_ZINT_TO_INT(pos%RIDX_CHUNK);
        int sz1 = RIDX_CHUNK - off;    /* sz1 is size of buffer to read.. */

        if (sz1 > itemsize)
            sz1 = itemsize;  /* no more than itemsize bytes */

        r = bf_read(p->index_BFile, 1+pos/RIDX_CHUNK, off, sz1, buf);
        if (r == 1 && sz1 < itemsize) /* boundary? - must read second part */
            r = bf_read(p->index_BFile, 2+pos/RIDX_CHUNK, 0, itemsize - sz1,
                        (char*) buf + sz1);
        if (r != 1 && !ignoreError)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "read in %s at pos %ld",
                    p->index_fname, (long) pos);
        }
    }
#if 0
    {
        struct record_index_entry *ep = buf;
        yaz_log(YLOG_LOG, "read r=%d sysno=" ZINT_FORMAT " next=" ZINT_FORMAT
            " sz=%d", r, sysno, ep->next, ep->size);
    }
#endif
    return r;
}

struct code_read_data {
    int no;
    zint sysno;
    void *buf;
    int itemsize;
    int insert_flag;
};

int bt_code_read(void *vp, char **dst, int *insertMode)
{
    struct code_read_data *s = (struct code_read_data *) vp;

    if (s->no == 0)
        return 0;

    (s->no)--;

    memcpy(*dst, &s->sysno, sizeof(zint));
    *dst += sizeof(zint);
    **dst = s->itemsize;
    (*dst)++;
    memcpy(*dst, s->buf, s->itemsize);
    *dst += s->itemsize;
    *insertMode = s->insert_flag;
    return 1;
}

void recindex_write_indx(recindex_t p, zint sysno, void *buf, int itemsize)
{
#if 0
    yaz_log(YLOG_LOG, "write_indx sysno=" ZINT_FORMAT, sysno);
#endif
    if (p->isamb)
    {
        struct code_read_data input;
        ISAMC_I isamc_i;

        input.sysno = sysno;
        input.buf = buf;
        input.itemsize = itemsize;

        isamc_i.clientData = &input;
        isamc_i.read_item = bt_code_read;

        input.no = 1;
        input.insert_flag = 2;
        isamb_merge(p->isamb, &p->isam_p, &isamc_i);
    }
    else
    {
        zint pos = (sysno-1)*itemsize;
        int off = CAST_ZINT_TO_INT(pos%RIDX_CHUNK);
        int sz1 = RIDX_CHUNK - off;    /* sz1 is size of buffer to read.. */

        if (sz1 > itemsize)
            sz1 = itemsize;  /* no more than itemsize bytes */

        bf_write(p->index_BFile, 1+pos/RIDX_CHUNK, off, sz1, buf);
        if (sz1 < itemsize)   /* boundary? must write second part */
            bf_write(p->index_BFile, 2+pos/RIDX_CHUNK, 0, itemsize - sz1,
                     (char*) buf + sz1);
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

