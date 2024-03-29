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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <yaz/log.h>
#include <yaz/xmalloc.h>
#include <idzebra/isams.h>

typedef struct {
    int last_offset;
    int last_block;
} ISAMS_head;

typedef unsigned ISAMS_BLOCK_SIZE;

struct ISAMS_s {
    ISAMS_M *method;
    ISAMS_head head;
    ISAMS_head head_old;
    char *merge_buf;

    int block_size;
    int debug;
    BFile bf;
};

struct ISAMS_PP_s {
    ISAMS is;
    char *buf;
    int block_offset;
    int block_no;
    void *decodeClientData;
    int numKeys;
    int numRead;
};

void isams_getmethod (ISAMS_M *m)
{
    m->codec.start = NULL;
    m->codec.decode = NULL;
    m->codec.encode = NULL;
    m->codec.stop = NULL;
    m->codec.reset = NULL;

    m->compare_item = NULL;
    m->log_item = NULL;

    m->debug = 1;
    m->block_size = 128;
}

ISAMS isams_open (BFiles bfs, const char *name, int writeflag,
                  ISAMS_M *method)
{
    ISAMS is = (ISAMS) xmalloc (sizeof(*is));

    is->method = (ISAMS_M *) xmalloc (sizeof(*is->method));
    memcpy (is->method, method, sizeof(*method));
    is->block_size = is->method->block_size;
    is->debug = is->method->debug;

    is->bf = bf_open (bfs, name, is->block_size, writeflag);

    if (!bf_read (is->bf, 0, 0, sizeof(ISAMS_head), &is->head))
    {
        is->head.last_block = 1;
        is->head.last_offset = 0;
    }
    memcpy (&is->head_old, &is->head, sizeof(is->head));
    is->merge_buf = (char *) xmalloc(2*is->block_size);
    memset(is->merge_buf, 0, 2*is->block_size);
    return is;
}

int isams_close (ISAMS is)
{
    if (memcmp(&is->head, &is->head_old, sizeof(is->head)))
    {
        if (is->head.last_offset)
            bf_write(is->bf, is->head.last_block, 0, is->head.last_offset,
                     is->merge_buf);
        bf_write (is->bf, 0, 0, sizeof(is->head), &is->head);
    }
    bf_close (is->bf);
    xfree (is->merge_buf);
    xfree (is->method);
    xfree (is);
    return 0;
}

ISAM_P isams_merge (ISAMS is, ISAMS_I data)
{
    char i_item[128];
    int i_more, i_mode;
    void *r_clientData;
    int first_block = is->head.last_block;
    int first_offset = is->head.last_offset;
    int count = 0;

    r_clientData = (*is->method->codec.start)();

    is->head.last_offset += sizeof(int);
    if (is->head.last_offset > is->block_size)
    {
        if (is->debug > 2)
            yaz_log (YLOG_LOG, "first_block=%d", first_block);
        bf_write(is->bf, is->head.last_block, 0, 0, is->merge_buf);
        (is->head.last_block)++;
        is->head.last_offset -= is->block_size;
        memcpy (is->merge_buf, is->merge_buf + is->block_size,
                is->head.last_offset);
    }
    while (1)
    {
        char *tmp_ptr = i_item;
        i_more = (*data->read_item)(data->clientData, &tmp_ptr, &i_mode);
        assert (i_mode);

        if (!i_more)
            break;
        else
        {
            char *r_out_ptr = is->merge_buf + is->head.last_offset;

            const char *i_item_ptr = i_item;
            (*is->method->codec.encode)(r_clientData, &r_out_ptr, &i_item_ptr);
            is->head.last_offset = r_out_ptr - is->merge_buf;
            if (is->head.last_offset > is->block_size)
            {
                bf_write(is->bf, is->head.last_block, 0, 0, is->merge_buf);
                (is->head.last_block)++;
                is->head.last_offset -= is->block_size;
                memcpy (is->merge_buf, is->merge_buf + is->block_size,
                        is->head.last_offset);
            }
            count++;
        }
    }
    (*is->method->codec.stop)(r_clientData);
    if (first_block == is->head.last_block)
        memcpy(is->merge_buf + first_offset, &count, sizeof(int));
    else if (first_block == is->head.last_block-1)
    {
        int gap = first_offset + sizeof(int) - is->block_size;
        assert (gap <= (int) sizeof(int));
        if (gap > 0)
        {
            if (gap < (int) sizeof(int))
                bf_write(is->bf, first_block, first_offset, sizeof(int)-gap,
                         &count);
            memcpy (is->merge_buf, ((char*)&count)+(sizeof(int)-gap), gap);
        }
        else
            bf_write(is->bf, first_block, first_offset, sizeof(int), &count);
    }
    else
    {
        bf_write(is->bf, first_block, first_offset, sizeof(int), &count);
    }
    return first_block * is->block_size + first_offset;
}

ISAMS_PP isams_pp_open (ISAMS is, ISAM_P pos)
{
    ISAMS_PP pp = (ISAMS_PP) xmalloc (sizeof(*pp));

    if (is->debug > 1)
        yaz_log (YLOG_LOG, "isams: isams_pp_open pos=%ld", (long) pos);
    pp->is = is;
    pp->decodeClientData = (*is->method->codec.start)();
    pp->numKeys = 0;
    pp->numRead = 0;
    pp->buf = (char *) xmalloc(is->block_size*2);
    pp->block_no = (int) (pos/is->block_size);
    pp->block_offset = (int) (pos - pp->block_no * is->block_size);
    if (is->debug)
        yaz_log (YLOG_LOG, "isams: isams_pp_open off=%d no=%d",
              pp->block_offset, pp->block_no);
    if (pos)
    {
        bf_read (is->bf, pp->block_no, 0, 0, pp->buf);
        bf_read (is->bf, pp->block_no+1, 0, 0, pp->buf + is->block_size);
        memcpy(&pp->numKeys, pp->buf + pp->block_offset, sizeof(int));
        if (is->debug)
            yaz_log (YLOG_LOG, "isams: isams_pp_open numKeys=%d", pp->numKeys);
        pp->block_offset += sizeof(int);
    }
    return pp;
}

void isams_pp_close (ISAMS_PP pp)
{
    (*pp->is->method->codec.stop)(pp->decodeClientData);
    xfree(pp->buf);
    xfree(pp);
}

int isams_pp_num (ISAMS_PP pp)
{
    return pp->numKeys;
}

int isams_pp_read (ISAMS_PP pp, void *buf)
{
    char *cp = buf;
    return isams_read_item (pp, &cp);
}

int isams_read_item (ISAMS_PP pp, char **dst)
{
    const char *src;
    if (pp->numRead >= pp->numKeys)
        return 0;
    (pp->numRead)++;
    if (pp->block_offset > pp->is->block_size)
    {
        pp->block_offset -= pp->is->block_size;
        (pp->block_no)++;
        memcpy (pp->buf, pp->buf + pp->is->block_size, pp->is->block_size);
        bf_read (pp->is->bf, pp->block_no+1, 0, 0,
                 pp->buf + pp->is->block_size);
    }
    src = pp->buf + pp->block_offset;
    (*pp->is->method->codec.decode)(pp->decodeClientData, dst, &src);
    pp->block_offset = src - pp->buf;
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

