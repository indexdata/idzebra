/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isamc.c,v $
 * Revision 1.2  1996-10-29 16:44:56  adam
 * Work on isc_merge.
 *
 * Revision 1.1  1996/10/29  13:40:48  adam
 * First work.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <log.h>
#include "isamc-p.h"

ISAMC_M isc_getmethod (void)
{
    static struct ISAMC_filecat_s def_cat[] = {
        {   32,    28,     0,    20 },
        {  512,   490,   100,    20 },
        { 4096,  3950,  1000,    20 },
        {32768, 32000, 10000,     0 },
        {    0,     0,     0,     0 }
    };
    ISAMC_M m = xmalloc (sizeof(*m));
    m->filecat = def_cat;

    m->code_start = NULL;
    m->code_item = NULL;
    m->code_stop = NULL;

    m->compare_item = NULL;

    m->debug = 0;

    return m;
}


ISAMC isc_open (const char *name, int writeflag, ISAMC_M method)
{
    ISAMC is;
    ISAMC_filecat filecat;
    int i;
    int max_buf_size = 0;

    is = xmalloc (sizeof(*is));

    is->method = xmalloc (sizeof(*is->method));
    memcpy (is->method, method, sizeof(*method));
    filecat = is->method->filecat;
    assert (filecat);

    /* determine number of block categories */
    if (is->method->debug)
        logf (LOG_LOG, "isc: bsize  ifill  mfill mblocks");
    for (i = 0; filecat[i].bsize; i++)
    {
        if (is->method->debug)
            logf (LOG_LOG, "isc:%6d %6d %6d %6d",
                  filecat[i].bsize, filecat[i].ifill, 
                  filecat[i].mfill, filecat[i].mblocks);
        if (max_buf_size < filecat[i].mblocks * filecat[i].bsize)
            max_buf_size = filecat[i].mblocks * filecat[i].bsize;
    }
    is->no_files = i;
    is->max_cat = --i;
    /* max_buf_size is the larget buffer to be used during merge */
    max_buf_size = (1 + max_buf_size / filecat[i].bsize) * filecat[i].bsize;
    if (is->method->debug)
        logf (LOG_LOG, "isc: max_buf_size %d", max_buf_size);
    
    assert (is->no_files > 0);
    is->files = xmalloc (sizeof(*is->files)*i);
    is->r_buf = xmalloc (max_buf_size+128);
    for (i = 0; i<is->no_files; i++)
    {
        char fname[512];

        sprintf (fname, "%s%c", name, i+'A');
        is->files[i].bf = bf_open (fname, is->method->filecat[i].bsize,
                                   writeflag);
        is->files[i].head_is_dirty = 0;
        if (!bf_read (is->files[i].bf, 0, 0, sizeof(ISAMC_head),
                     &is->files[i].head))
        {
            is->files[i].head.lastblock = 1;
            is->files[i].head.freelist = 0;
        }
    }
    return is;
}

int isc_close (ISAMC is)
{
    int i;

    for (i = 0; i<is->no_files; i++)
        if (is->files[i].bf)
        {
            if (is->files[i].head_is_dirty)
                bf_write (is->files[i].bf, 0, 0, sizeof(ISAMC_head),
                     &is->files[i].head);
            bf_close (is->files[i].bf);
        }
    xfree (is->files);
    xfree (is->r_buf);
    xfree (is);
    return 0;
}

int isc_read_block (ISAMC is, int cat, int pos, char *dst)
{
    if (is->method->debug > 1)
        logf (LOG_LOG, "isc: read_block %d %d", cat, pos);
    return bf_read (is->files[cat].bf, pos, 0, 0, dst);
}

int isc_write_block (ISAMC is, int cat, int pos, char *src)
{
    if (is->method->debug > 1)
        logf (LOG_LOG, "isc: write_block %d %d", cat, pos);
    return bf_write (is->files[cat].bf, pos, 0, 0, src);
}

int isc_write_dblock (ISAMC is, int cat, int pos, char *src,
                      int nextpos, int offset)
{
    int xoffset = offset + 2*sizeof(int);
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: write_dblock. offset=%d nextpos=%d",
              offset, nextpos);
    memcpy (src - sizeof(int)*2, &nextpos, sizeof(int));
    memcpy (src - sizeof(int), &xoffset, sizeof(int));
    return isc_write_block (is, cat, pos, src - sizeof(int)*2);
}

int isc_alloc_block (ISAMC is, int cat)
{
    int block;
    char buf[sizeof(int)];

    is->files[cat].head_is_dirty = 1;
    if ((block = is->files[cat].head.freelist))
    {
        bf_read (is->files[cat].bf, block, 0, sizeof(int), buf);
        memcpy (&is->files[cat].head.freelist, buf, sizeof(int));
    }
    else
        block = (is->files[cat].head.lastblock)++;
    if (is->method->debug > 2)
        logf (LOG_LOG, "isc: alloc_block in cat %d -> %d", cat, block);
    return block;
}

void isc_release_block (ISAMC is, int cat, int pos)
{
    char buf[sizeof(int)];
   
    is->files[cat].head_is_dirty = 1; 
    memcpy (buf, &is->files[cat].head.freelist, sizeof(int));
    is->files[cat].head.freelist = pos;
    bf_write (is->files[cat].bf, pos, 0, sizeof(int), buf);
}

static void isc_flush_blocks (ISAMC is, int *r_ptr, int r_ptri, char *r_buf,
                              int *nextpos, int *firstpos, int cat, int last)
{
    int i;

    for (i = 1; i<r_ptri; i++)
    {
        int pos;
        if (*nextpos)
            pos = *nextpos;
        else
            pos = isc_alloc_block (is, cat);
        if (!*firstpos)
            *firstpos = pos;
        if (last && i == r_ptri-1)
            *nextpos = 0;
        else
            *nextpos = isc_alloc_block (is, cat);
        isc_write_dblock (is, cat, pos, r_buf + r_ptr[i-1], *nextpos,
                          r_ptr[i] - r_ptr[i-1]);
    }
}


ISAMC_P isc_merge_first (ISAMC is, ISAMC_I data)
{
    char i_item[128], *i_item_ptr;
    int i_more, i_mode, i;

    int firstpos = 0;
    int nextpos = 0;    
    int cat = 0;
    char r_item_buf[128];
    int r_offset = 0;
    int r_ptr[100];
    int r_ptri = 0;
    void *r_clientData = (*is->method->code_start)(ISAMC_ENCODE);
    char *r_buf = is->r_buf + ISAMC_BLOCK_OFFSET;

    /* read first item from i */
    i_item_ptr = i_item;
    i_more = (*data->read_item)(data->clientData, &i_item_ptr, &i_mode);
    if (i_more)
        r_ptr[r_ptri++] = 0;
    while (i_more)
    {
        char *r_item = r_item_buf;

        memcpy (r_item, i_item, i_item_ptr - i_item);
        
        if (r_item)  /* insert resulting item? */
        {
            char *r_out_ptr = r_buf + r_offset;
            int new_offset;
            int border = r_ptr[r_ptri-1] + is->method->filecat[cat].ifill
                         -ISAMC_BLOCK_OFFSET;

            (*is->method->code_item)(ISAMC_ENCODE, r_clientData,
                                     &r_out_ptr, &r_item);
            new_offset = r_out_ptr - r_buf; 

            if (border >= r_offset && border < new_offset)
            {
                /* Initial fill of current block category reached... 
                   Save offset in r_ptr 
                 */
                r_ptr[r_ptri++] = r_offset;
                if (cat == is->max_cat)
                {
                    /* We are dealing with block of max size. Block(s)
                       will be flushed. Note: the block(s) are surely not
                       the last one(s).
                     */
                    if (is->method->debug > 1)
                        logf (LOG_LOG, "isc: flush %d sections", r_ptri-1);
                    isc_flush_blocks (is, r_ptr, r_ptri, r_buf,
                                      &nextpos, &firstpos, cat, 0);
                    r_ptri = 0;
                    r_ptr[r_ptri++] = 0;
                    memcpy (r_buf, r_buf + r_offset, new_offset - r_offset);
                    new_offset = (new_offset - r_offset);
                }
            }
            r_offset = new_offset;
            if (cat < is->max_cat &&
                r_ptri>is->method->filecat[cat].mblocks)
            {
                /* Max number blocks in current category reached ->
                   must switch to next category (with larger block size) 
                */
                int j = 1;
                cat++;
                /* r_ptr[0] = r_ptr[0] = 0 true anyway.. */
                for (i = 2; i < r_ptri; i++)
                {
                    border = is->method->filecat[cat].ifill -
                             ISAMC_BLOCK_OFFSET + r_ptr[j-1];
                    if (r_ptr[i] > border && r_ptr[i-1] <= border)
                        r_ptr[j++] = r_ptr[i-1];
                }
                r_ptri = j;
            }
        }
        i_item_ptr = i_item;
        i_more = (*data->read_item)(data->clientData, &i_item_ptr, &i_mode);
    }
    r_ptr[r_ptri++] = r_offset;
    /* flush rest of block(s) in r_buf */
    if (is->method->debug > 1)
        logf (LOG_LOG, "isc: flush rest, %d sections", r_ptri-1);
    isc_flush_blocks (is, r_ptr, r_ptri, r_buf, &nextpos, &firstpos, cat, 1);
    (*is->method->code_stop)(ISAMC_ENCODE, r_clientData);
    return cat + firstpos * 8;
}

ISAMC_P isc_merge (ISAMC is, ISAMC_P ipos, ISAMC_I data)
{
    char i_item[128], *i_item_ptr;
    int i_more, i_mode, i;

    ISAMC_PP pp; 
    char f_item[128], *f_item_ptr;
    int f_more;
    int block_ptr[100];   /* block pointer (0 if none) */
    int dirty_ptr[100];   /* dirty flag pointer (1 if dirty) */
 
    int firstpos = 0;
    int nextpos = 0;    
    int cat = 0;
    char r_item_buf[128]; /* temporary result output */
    char *r_buf;          /* block with resulting data */
    int r_offset = 0;     /* current offset in r_buf */
    int r_ptr[100];       /* offset pointer */
    int r_ptri = 0;       /* pointer */
    void *r_clientData;   /* encode client data */

    if (ipos == 0)
        return isc_merge_first (is, data);

    r_clientData = (*is->method->code_start)(ISAMC_ENCODE);
    r_buf = is->r_buf + ISAMC_BLOCK_OFFSET;

    pp = isc_pp_open (is, ipos);
    f_item_ptr = f_item;
    f_more = isc_read_item (pp, &f_item_ptr);
    cat = pp->cat;

    /* read first item from i */
    i_item_ptr = i_item;
    i_more = (*data->read_item)(data->clientData, &i_item_ptr, &i_mode);
    block_ptr[r_ptri] = pp->pos;
    dirty_ptr[r_ptri] = 0;
    r_ptr[r_ptri++] = 0;

    while (i_more || f_more)
    {
        char *r_item = r_item_buf;
        int cmp;

        if (!f_more)
            cmp = -1;
        else if (!i_more)
            cmp = 1;
        else
            cmp = (*is->method->compare_item)(i_item, f_item);
        if (cmp == 0)                   /* insert i=f */
        {
            if (!i_mode) 
            {
                r_item = NULL;
                dirty_ptr[r_ptri-1] = 1;
            }
            else
                memcpy (r_item, f_item, f_item_ptr - f_item);

            /* move i */
            i_item_ptr = i_item;
            i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                        &i_mode);
            /* move f */
            f_item_ptr = f_item;
            f_more = isc_read_item (pp, &f_item_ptr);
        }
        else if (cmp > 0)               /* insert f */
        {
            memcpy (r_item, f_item, f_item_ptr - f_item);
            /* move f */
            f_item_ptr = f_item;
            f_more = isc_read_item (pp, &f_item_ptr);
        }
        else                            /* insert i */
        {
            if (!i_mode)                /* delete item which isn't there? */
            {
                logf (LOG_FATAL, "Inconsistent register");
                abort ();
            }
            memcpy (r_item, i_item, i_item_ptr - i_item);
            dirty_ptr[r_ptri-1] = 1;
            /* move i */
            i_item_ptr = i_item;
            i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                        &i_mode);
        }
        if (r_item)  /* insert resulting item? */
        {
            char *r_out_ptr = r_buf + r_offset;
            int new_offset;
            int border;

            /* border set to initial fill or block size depending on
               whether we are creating a new one or updating and old
             */
            if (block_ptr[r_ptri-1])
                border = r_ptr[r_ptri-1] + is->method->filecat[cat].bsize
                         -ISAMC_BLOCK_OFFSET;
            else
                border = r_ptr[r_ptri-1] + is->method->filecat[cat].ifill
                         -ISAMC_BLOCK_OFFSET;

            (*is->method->code_item)(ISAMC_ENCODE, r_clientData,
                                     &r_out_ptr, &r_item);
            new_offset = r_out_ptr - r_buf; 

            if (border >= r_offset && border < new_offset)
            {
                /* Initial fill of current block category reached... 
                   Save offset in r_ptr 
                 */
                r_ptr[r_ptri++] = r_offset;
                if (cat == is->max_cat)
                {
                    /* We are dealing with block of max size. Block(s)
                       will be flushed. Note: the block(s) are surely not
                       the last one(s).
                     */
                    if (is->method->debug > 1)
                        logf (LOG_LOG, "isc: flush %d sections", r_ptri-1);
                    isc_flush_blocks (is, r_ptr, r_ptri, r_buf,
                                      &nextpos, &firstpos, cat, 0);
                    r_ptri = 0;
                    r_ptr[r_ptri++] = 0;
                    memcpy (r_buf, r_buf + r_offset, new_offset - r_offset);
                    new_offset = (new_offset - r_offset);
                }
            }
            r_offset = new_offset;
            if (cat < is->max_cat &&
                r_ptri>is->method->filecat[cat].mblocks)
            {
                /* Max number blocks in current category reached ->
                   must switch to next category (with larger block size) 
                */
                int j = 1;
                cat++;
                /* r_ptr[0] = r_ptr[0] = 0 true anyway.. */
                /* AD: Any old blocks should be deleted */
                for (i = 2; i < r_ptri; i++)
                {
                    border = is->method->filecat[cat].ifill -
                             ISAMC_BLOCK_OFFSET + r_ptr[j-1];
                    if (r_ptr[i] > border && r_ptr[i-1] <= border)
                        r_ptr[j++] = r_ptr[i-1];
                }
                r_ptri = j;
            }
        }
    }
    r_ptr[r_ptri++] = r_offset;
    /* flush rest of block(s) in r_buf */
    if (is->method->debug > 1)
        logf (LOG_LOG, "isc: flush rest, %d sections", r_ptri-1);
    isc_flush_blocks (is, r_ptr, r_ptri, r_buf, &nextpos, &firstpos, cat, 1);
    (*is->method->code_stop)(ISAMC_ENCODE, r_clientData);
    return cat + firstpos * 8;
}


#if 0
ISAMC_P isc_merge (ISAMC is, ISAMC_P ipos, ISAMC_I data)
{
    ISAMC_PP pp; 
    char f_item[128], *f_item_ptr;
    int f_more;
    int cat = 0;
    int nextpos;

    char i_item[128], *i_item_ptr;
    int i_more, insertMode;

    char r_item_buf[128];
    int r_offset = ISAMC_BLOCK_OFFSET;
    int r_dirty = 0;
    char *r_ptr[100];
    int r_ptri = 0;
    int r_start = 0;
    void *r_clientData = (*is->method->code_start)();

    /* rewind and read first item from file */
    pp = isc_position (is, ipos);
    f_item_ptr = f_item;
    f_more = isc_read_item (pp, &f_item_ptr);
    cat = pp->cat;

    /* read first item from i */
    i_item_ptr = i_item;
    i_more = (*data->read_item)(data->clientData, &i_item_ptr, &insertMode);
   
    while (f_more || i_more)
    {
        int cmp;
        char *r_item = r_item_buf;

        if (!f_more)
            cmp = -1;
        else if (!i_more)
            cmp = 1;
        else
            cmp = (*is->method->compare_item)(i_item, f_item);
        if (cmp == 0)                   /* insert i=f */
        {
            if (!insertMode) 
            {
                r_item = NULL;
                r_dirty = 1;
            }
            else
                memcpy (r_item, f_item, f_item_ptr - f_item);

            /* move i */
            i_item_ptr = i_item;
            i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                        &insertMode);
            /* move f */
            f_item_ptr = f_item;
            f_more = isc_read_item (pp, &f_item_ptr);
        }
        else if (cmp > 0)               /* insert f */
        {
            memcpy (r_item, f_item, f_item_ptr - f_item);
            /* move f */
            f_item_ptr = f_item;
            f_more = isc_read_item (pp, &f_item_ptr);
        }
        else                            /* insert i */
        {
            if (!insertMode)            /* delete item which isn't there? */
            {
                logf (LOG_FATAL, "Inconsistent register");
                abort ();
            }
            memcpy (r_item, i_item, i_item_ptr - i_item);
            r_dirty = 1;
            /* move i */
            i_item_ptr = i_item;
            i_more = (*data->read_item)(data->clientData, &i_item_ptr,
                                        &insertMode);
        }
        /* check for end of input block condition */

        if (r_item)  /* insert resulting item? */
        {
            char *r_out_ptr = is->r_buf + r_offset;
            int new_offset;
            int border = is->method->filecat[cat].initsize - r_start;

            (*is->method->code_item)(r_clientData, &r_out_ptr, &r_item);
            new_offset = r_out_ptr - is->r_buf; 

            if (border >= r_offset && border < r_newoffset)
            {
                r_ptr[r_ptri++] = r_offset;
                if (!is->method->filecat[cat].mblocks)
                {
                    assert (r_ptri == 1);
                    /* dump block from 0 -> r_offset in max cat */
                    r_ptri = 0;
                    r_offset = ISAMC_BLOCK_OFFSET;
                }
            }
            r_offset = new_offset;
        }
        if (r_ptri && r_ptri == is->method->filecat[cat].mblocks)
        {
            int i, j = 0;

            /* dump previous blocks in chain */

            /* recalc r_ptr's */
            cat++;
            for (i = 1; i<r_ptr; i++)
            {
                if (r_ptr[i] > is->method->filecat[cat].ifill &&
                    r_ptr[i-1] <= is->method->filecat[cat].ifill)
                    r_ptr[j++] = r_ptr[i-1];
            }
            r_ptri = j;
        }
    }
    (*is->method->code_stop)(r_clientData);
    return ipos;
}
#endif

void isc_pp_close (ISAMC_PP pp)
{
    ISAMC is = pp->is;

    (*is->method->code_stop)(ISAMC_DECODE, pp->decodeClientData);
    xfree (pp->buf);
    xfree (pp);
}

ISAMC_PP isc_pp_open (ISAMC is, ISAMC_P ipos)
{
    ISAMC_PP pp = xmalloc (sizeof(*pp));
    char *src;
   
    pp->cat = isc_type(ipos);
    pp->next = isc_block(ipos); 

    src = pp->buf = xmalloc (is->method->filecat[pp->cat].bsize);

    pp->pos = 0;    
    pp->size = 0;
    pp->offset = 0;
    pp->is = is;
    pp->decodeClientData = (*is->method->code_start)(ISAMC_DECODE);
    return pp;
}

/* returns 1 if item could be read; 0 otherwise */
int isc_read_key (ISAMC_PP pp, void *buf)
{
    return isc_read_item (pp, (char **) &buf);
}

/* returns 1 if item could be read; 0 otherwise */
int isc_read_item (ISAMC_PP pp, char **dst)
{
    ISAMC is = pp->is;
    char *src = pp->buf + pp->offset;

    if (pp->offset >= pp->size)
    {
        pp->pos = pp->next;
        if (!pp->pos)
            return 0;
        src = pp->buf;
        isc_read_block (is, pp->cat, pp->pos, src);
        
        memcpy (&pp->next, src, sizeof(pp->next));
        src += sizeof(pp->next);
        memcpy (&pp->size, src, sizeof(pp->size));
        src += sizeof(pp->size);
        /* assume block is non-empty */
        assert (pp->next != pp->pos);
    }
    (*is->method->code_item)(ISAMC_DECODE, pp->decodeClientData, dst, &src);
    pp->offset = src - pp->buf; 
    return 1;
}

int isc_read_islast (ISAMC_PP pp)
{
    return pp->offset >= pp->size;
}

int isc_numkeys (ISAMC_PP pp)
{
    return 1;
}

