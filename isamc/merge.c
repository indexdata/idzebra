/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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
#include "isamc-p.h"

struct isamc_merge_block {
    int offset;   /* offset in r_buf */
    zint block;   /* block number of file (0 if none) */
    int dirty;    /* block is different from that on file */
};

#if 0
static void opt_blocks (ISAMC is, struct isamc_merge_block *mb, int ptr,
			int last)
{
    int i, no_dirty = 0;
    for (i = 0; i<ptr; i++)
	if (mb[i].dirty)
	    no_dirty++;
    if (no_dirty*4 < ptr*3)
	return;
    /* bubble-sort it */
    for (i = 0; i<ptr; i++)
    {
	int tmp, j, j_min = -1;
	for (j = i; j<ptr; j++)
	{
	    if (j_min < 0 || mb[j_min].block > mb[j].block)
		j_min = j;
	}
	assert (j_min >= 0);
	tmp = mb[j_min].block;
	mb[j_min].block = mb[i].block;
	mb[i].block = tmp;
	mb[i].dirty = 1;
    }
    if (!last)
	mb[i].dirty = 1;
}
#endif

static void flush_blocks (ISAMC is, struct isamc_merge_block *mb, int ptr,
                          char *r_buf, zint *firstpos, int cat, int last,
                          zint *numkeys)
{
    int i;

    for (i = 0; i<ptr; i++)
    {
        /* consider this block number */
        if (!mb[i].block)
        {
            mb[i].block = isamc_alloc_block (is, cat);
            mb[i].dirty = 1;
        }

        /* consider next block pointer */
        if (last && i == ptr-1)
            mb[i+1].block = 0;
        else if (!mb[i+1].block)
        {
            mb[i+1].block = isamc_alloc_block (is, cat);
            mb[i+1].dirty = 1;
            mb[i].dirty = 1;
        }
    }

    for (i = 0; i<ptr; i++)
    {
        char *src;
        ISAMC_BLOCK_SIZE ssize = mb[i+1].offset - mb[i].offset;

        assert (ssize);

        /* skip rest if not dirty */
        if (!mb[i].dirty)
        {
            assert (mb[i].block);
            if (!*firstpos)
                *firstpos = mb[i].block;
            if (is->method->debug > 2)
                yaz_log (YLOG_LOG, "isc: skip ptr=%d size=%d %d " ZINT_FORMAT,
                     i, ssize, cat, mb[i].block);
            ++(is->files[cat].no_skip_writes);
            continue;
        }
        /* write block */

        if (!*firstpos)
        {
            *firstpos = mb[i].block;
            src = r_buf + mb[i].offset - ISAMC_BLOCK_OFFSET_1;
            ssize += ISAMC_BLOCK_OFFSET_1;

            memcpy (src+sizeof(zint)+sizeof(ssize), numkeys, sizeof(*numkeys));
            if (is->method->debug > 2)
                yaz_log (YLOG_LOG, "isc: flush ptr=%d numk=" ZINT_FORMAT " size=%d nextpos="
		      ZINT_FORMAT, i, *numkeys, (int) ssize, mb[i+1].block);
        }
        else
        {
            src = r_buf + mb[i].offset - ISAMC_BLOCK_OFFSET_N;
            ssize += ISAMC_BLOCK_OFFSET_N;
            if (is->method->debug > 2)
                yaz_log (YLOG_LOG, "isc: flush ptr=%d size=%d nextpos=" ZINT_FORMAT,
                     i, (int) ssize, mb[i+1].block);
        }
        memcpy (src, &mb[i+1].block, sizeof(zint));
        memcpy (src+sizeof(zint), &ssize, sizeof(ssize));
        isamc_write_block (is, cat, mb[i].block, src);
    }
}

static int get_border (ISAMC is, struct isamc_merge_block *mb, zint ptr,
                       int cat, zint firstpos)
{
   /* Border set to initial fill or block size depending on
      whether we are creating a new one or updating and old one.
    */

    int fill = mb[ptr].block ? is->method->filecat[cat].bsize :
                               is->method->filecat[cat].ifill;
    int off = (ptr||firstpos) ? ISAMC_BLOCK_OFFSET_N : ISAMC_BLOCK_OFFSET_1;

    assert (ptr < 199);

    return mb[ptr].offset + fill - off;
}

void isamc_merge (ISAMC is, ISAM_P *ipos, ISAMC_I *data)
{

    char i_item[128], *i_item_ptr;
    int i_more, i_mode, i;

    ISAMC_PP pp;
    char f_item[128], *f_item_ptr;
    int f_more;
    int last_dirty = 0;
    int debug = is->method->debug;

    struct isamc_merge_block mb[200];

    zint firstpos = 0;
    int cat = 0;
    char r_item_buf[128]; /* temporary result output */
    char *r_buf;          /* block with resulting data */
    int r_offset = 0;     /* current offset in r_buf */
    int ptr = 0;          /* pointer */
    void *r_clientData;   /* encode client data */
    zint border;
    zint numKeys = 0;

    r_clientData = (*is->method->codec.start)();
    r_buf = is->merge_buf + 128;

    pp = isamc_pp_open (is, *ipos);
    /* read first item from file. make sure f_more indicates no boundary */
    f_item_ptr = f_item;
    f_more = isamc_read_item (pp, &f_item_ptr);
    if (f_more > 0)
        f_more = 1;
    cat = pp->cat;

    if (debug > 1)
        yaz_log (YLOG_LOG, "isc: isamc_merge begin %d " ZINT_FORMAT, cat, pp->pos);

    /* read first item from i */
    i_item_ptr = i_item;
    i_more = (*data->read_item)(data->clientData, &i_item_ptr, &i_mode);

    mb[ptr].block = pp->pos;     /* is zero if no block on disk */
    mb[ptr].dirty = 0;
    mb[ptr].offset = 0;

    border = get_border (is, mb, ptr, cat, firstpos);
    while (i_more || f_more)
    {
        char *r_item = r_item_buf;
        int cmp;

        if (f_more > 1)
        {
            /* block to block boundary in the original file. */
            f_more = 1;
            if (cat == pp->cat)
            {
                /* the resulting output is of the same category as the
                   the original
		*/
                if (r_offset <= mb[ptr].offset +is->method->filecat[cat].mfill)
                {
                    /* the resulting output block is too small/empty. Delete
                       the original (if any)
		    */
                    if (debug > 3)
                        yaz_log (YLOG_LOG, "isc: release A");
                    if (mb[ptr].block)
                        isamc_release_block (is, pp->cat, mb[ptr].block);
                    mb[ptr].block = pp->pos;
		    if (!mb[ptr].dirty)
			mb[ptr].dirty = 1;
                    if (ptr > 0)
                        mb[ptr-1].dirty = 1;
                }
                else
                {
                    /* indicate new boundary based on the original file */
                    mb[++ptr].block = pp->pos;
                    mb[ptr].dirty = last_dirty;
                    mb[ptr].offset = r_offset;
                    if (debug > 3)
                        yaz_log (YLOG_LOG, "isc: bound ptr=%d,offset=%d",
                            ptr, r_offset);
                    if (cat==is->max_cat && ptr >= is->method->max_blocks_mem)
                    {
                        /* We are dealing with block(s) of max size. Block(s)
                           except 1 will be flushed.
                         */
                        if (debug > 2)
                            yaz_log (YLOG_LOG, "isc: flush A %d sections", ptr);
                        flush_blocks (is, mb, ptr-1, r_buf, &firstpos, cat,
                                      0, &pp->numKeys);
                        mb[0].block = mb[ptr-1].block;
                        mb[0].dirty = mb[ptr-1].dirty;
                        memcpy (r_buf, r_buf + mb[ptr-1].offset,
                                mb[ptr].offset - mb[ptr-1].offset);
                        mb[0].offset = 0;

                        mb[1].block = mb[ptr].block;
                        mb[1].dirty = mb[ptr].dirty;
                        mb[1].offset = mb[ptr].offset - mb[ptr-1].offset;
                        ptr = 1;
                        r_offset = mb[ptr].offset;
                    }
                }
            }
            border = get_border (is, mb, ptr, cat, firstpos);
        }
	last_dirty = 0;
        if (!f_more)
            cmp = -1;
        else if (!i_more)
            cmp = 1;
        else
            cmp = (*is->method->compare_item)(i_item, f_item);
        if (cmp == 0)                   /* insert i=f */
        {
            if (!i_mode)   /* delete item? */
            {
                /* move i */
                i_item_ptr = i_item;
                i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                           &i_mode);
                /* is next input item the same as current except
                   for the delete flag? */
                cmp = (*is->method->compare_item)(i_item, f_item);
                if (!cmp && i_mode)   /* delete/insert nop? */
                {
                    /* yes! insert as if it was an insert only */
                    memcpy (r_item, i_item, i_item_ptr - i_item);
                    i_item_ptr = i_item;
                    i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                                &i_mode);
                }
                else
                {
                    /* no! delete the item */
                    r_item = NULL;
		    last_dirty = 1;
                    mb[ptr].dirty = 2;
                }
            }
            else
            {
                memcpy (r_item, f_item, f_item_ptr - f_item);

                /* move i */
                i_item_ptr = i_item;
                i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                           &i_mode);
            }
            /* move f */
            f_item_ptr = f_item;
            f_more = isamc_read_item (pp, &f_item_ptr);
        }
        else if (cmp > 0)               /* insert f */
        {
            memcpy (r_item, f_item, f_item_ptr - f_item);
            /* move f */
            f_item_ptr = f_item;
            f_more = isamc_read_item (pp, &f_item_ptr);
        }
        else                            /* insert i */
        {
            if (!i_mode)                /* delete item which isn't there? */
            {
                yaz_log (YLOG_FATAL, "Inconsistent register at offset %d",
                                 r_offset);
                abort ();
            }
            memcpy (r_item, i_item, i_item_ptr - i_item);
            mb[ptr].dirty = 2;
	    last_dirty = 1;
            /* move i */
            i_item_ptr = i_item;
            i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                        &i_mode);
        }
        if (r_item)  /* insert resulting item? */
        {
            char *r_out_ptr = r_buf + r_offset;
	    const char *src = r_item;
            int new_offset;

            (*is->method->codec.encode)(r_clientData, &r_out_ptr, &src);
            new_offset = r_out_ptr - r_buf;

            numKeys++;

            if (border < new_offset && border >= r_offset)
            {
                if (debug > 2)
                    yaz_log (YLOG_LOG, "isc: border %d " ZINT_FORMAT,
			  ptr, border);
                /* Max size of current block category reached ...
                   make new virtual block entry */
                mb[++ptr].block = 0;
                mb[ptr].dirty = 1;
                mb[ptr].offset = r_offset;
                if (cat == is->max_cat && ptr >= is->method->max_blocks_mem)
                {
                    /* We are dealing with block(s) of max size. Block(s)
                       except one will be flushed. Note: the block(s) are
                       surely not the last one(s).
                     */
                    if (debug > 2)
                        yaz_log (YLOG_LOG, "isc: flush B %d sections", ptr-1);
                    flush_blocks (is, mb, ptr-1, r_buf, &firstpos, cat,
                                  0, &pp->numKeys);
                    mb[0].block = mb[ptr-1].block;
                    mb[0].dirty = mb[ptr-1].dirty;
                    memcpy (r_buf, r_buf + mb[ptr-1].offset,
                            mb[ptr].offset - mb[ptr-1].offset);
                    mb[0].offset = 0;

                    mb[1].block = mb[ptr].block;
                    mb[1].dirty = mb[0].dirty;
                    mb[1].offset = mb[ptr].offset - mb[ptr-1].offset;
                    memcpy (r_buf + mb[1].offset, r_buf + r_offset,
                            new_offset - r_offset);
                    new_offset = (new_offset - r_offset) + mb[1].offset;
                    ptr = 1;
                }
                border = get_border (is, mb, ptr, cat, firstpos);
            }
            r_offset = new_offset;
        }
        if (cat < is->max_cat && ptr >= is->method->filecat[cat].mblocks)
        {
            /* Max number blocks in current category reached ->
               must switch to next category (with larger block size)
            */
            int j = 0;

            (is->files[cat].no_remap)++;
            /* delete all original block(s) read so far */
            for (i = 0; i < ptr; i++)
                if (mb[i].block)
                    isamc_release_block (is, pp->cat, mb[i].block);
            /* also delete all block to be read in the future */
            pp->deleteFlag = 1;

            /* remap block offsets */
            assert (mb[j].offset == 0);
            cat++;
            mb[j].dirty = 1;
            mb[j].block = 0;
	    mb[ptr].offset = r_offset;
            for (i = 1; i < ptr; i++)
            {
                int border = is->method->filecat[cat].ifill -
                         ISAMC_BLOCK_OFFSET_1 + mb[j].offset;
                if (debug > 3)
                    yaz_log (YLOG_LOG, "isc: remap %d border=%d", i, border);
                if (mb[i+1].offset > border && mb[i].offset <= border)
                {
                    if (debug > 3)
                        yaz_log (YLOG_LOG, "isc:  to %d %d", j, mb[i].offset);
                    mb[++j].dirty = 1;
                    mb[j].block = 0;
                    mb[j].offset = mb[i].offset;
                }
            }
            if (debug > 2)
                yaz_log (YLOG_LOG, "isc: remap from %d to %d sections to cat %d",
                      ptr, j, cat);
            ptr = j;
            border = get_border (is, mb, ptr, cat, firstpos);
	    if (debug > 3)
		yaz_log (YLOG_LOG, "isc: border=" ZINT_FORMAT " r_offset=%d",
		      border, r_offset);
        }
    }
    if (mb[ptr].offset < r_offset)
    {   /* make the final boundary offset */
        mb[++ptr].dirty = 1;
        mb[ptr].block = 0;
        mb[ptr].offset = r_offset;
    }
    else
    {   /* empty output. Release last block if any */
        if (cat == pp->cat && mb[ptr].block)
        {
            if (debug > 3)
                yaz_log (YLOG_LOG, "isc: release C");
            isamc_release_block (is, pp->cat, mb[ptr].block);
            mb[ptr].block = 0;
	    if (ptr > 0)
		mb[ptr-1].dirty = 1;
        }
    }

    if (debug > 2)
        yaz_log (YLOG_LOG, "isc: flush C, %d sections", ptr);

    if (firstpos)
    {
        /* we have to patch initial block with num keys if that
           has changed */
        if (numKeys != isamc_pp_num (pp))
        {
            if (debug > 2)
                yaz_log (YLOG_LOG, "isc: patch num keys firstpos=" ZINT_FORMAT " num=" ZINT_FORMAT,
                                firstpos, numKeys);
            bf_write (is->files[cat].bf, firstpos, ISAMC_BLOCK_OFFSET_N,
                      sizeof(numKeys), &numKeys);
        }
    }
    else if (ptr > 0)
    {   /* we haven't flushed initial block yet and there surely are some
           blocks to flush. Make first block dirty if numKeys differ */
        if (numKeys != isamc_pp_num (pp))
            mb[0].dirty = 1;
    }
    /* flush rest of block(s) in r_buf */
    flush_blocks (is, mb, ptr, r_buf, &firstpos, cat, 1, &numKeys);

    (*is->method->codec.stop)(r_clientData);
    if (!firstpos)
        cat = 0;
    if (debug > 1)
        yaz_log (YLOG_LOG, "isc: isamc_merge return %d " ZINT_FORMAT, cat, firstpos);
    isamc_pp_close (pp);
    *ipos = cat + firstpos * 8;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

