/*
 *  Copyright (c) 2000-2002, Index Data.
 *  See the file LICENSE for details.
 *
 *  $Id: isamb.c,v 1.14 2002-05-06 17:45:21 adam Exp $
 */
#include <yaz/xmalloc.h>
#include <yaz/log.h>
#include <isamb.h>
#include <assert.h>

struct ISAMB_head {
    int first_block;
    int last_block;
    int block_size;
    int block_max;
};

#define ISAMB_DATA_OFFSET 3

#define DST_ITEM_MAX 256

/* approx 2*4 K + max size of item */
#define DST_BUF_SIZE 8448


struct ISAMB_file {
    BFile bf;
    int head_dirty;
    struct ISAMB_head head;
};

struct ISAMB_s {
    BFiles bfs;
    ISAMC_M method;

    struct ISAMB_file *file;
    int no_cat;
};

struct ISAMB_block {
    ISAMB_P pos;
    int cat;
    int size;
    int leaf;
    int dirty;
    int offset;
    char *bytes;
    unsigned char *buf;
    void *decodeClientData;
};

struct ISAMB_PP_s {
    ISAMB isamb;
    ISAMB_P pos;
    int level;
    int total_size;
    int no_blocks;
    struct ISAMB_block **block;
};

void encode_ptr (char **dst, int pos)
{
    memcpy (*dst, &pos, sizeof(pos));
    (*dst) += sizeof(pos);
}

void decode_ptr (char **src, int *pos)
{
    memcpy (pos, *src, sizeof(*pos));
    (*src) += sizeof(*pos);
}

ISAMB isamb_open (BFiles bfs, const char *name, int writeflag, ISAMC_M method)
{
    ISAMB isamb = xmalloc (sizeof(*isamb));
    int i, b_size = 64;

    isamb->bfs = bfs;
    isamb->method = (ISAMC_M) xmalloc (sizeof(*method));
    memcpy (isamb->method, method, sizeof(*method));
    isamb->no_cat = 3;

    isamb->file = xmalloc (sizeof(*isamb->file) * isamb->no_cat);
    for (i = 0; i<isamb->no_cat; i++)
    {
        char fname[DST_BUF_SIZE];
        isamb->file[i].head_dirty = 0;
        sprintf (fname, "%s%c", name, i+'A');
        isamb->file[i].bf = bf_open (bfs, fname, b_size, writeflag);
    
        if (!bf_read (isamb->file[i].bf, 0, 0, sizeof(struct ISAMB_head),
                 &isamb->file[i].head))
	{
            isamb->file[i].head.first_block = 1;
            isamb->file[i].head.last_block = 1;
            isamb->file[i].head.block_size = b_size;
            isamb->file[i].head.block_max = b_size - ISAMB_DATA_OFFSET;
	}
        assert (isamb->file[i].head.block_size >= ISAMB_DATA_OFFSET);
        isamb->file[i].head_dirty = 0;
        b_size = b_size * 4;
    }
    return isamb;
}

void isamb_close (ISAMB isamb)
{
    int i;
    for (i = 0; i<isamb->no_cat; i++)
    {
        if (isamb->file[i].head_dirty)
            bf_write (isamb->file[i].bf, 0, 0,
                      sizeof(struct ISAMB_head), &isamb->file[i].head);
    }
    xfree (isamb->file);
    xfree (isamb->method);
    xfree (isamb);
}

struct ISAMB_block *open_block (ISAMB b, ISAMC_P pos)
{
    int cat = pos&3;
    struct ISAMB_block *p;
    if (!pos)
        return 0;
    p = xmalloc (sizeof(*p));
    p->pos = pos;
    p->cat = pos & 3;
    p->buf = xmalloc (b->file[cat].head.block_size);
    if (!bf_read (b->file[cat].bf, pos/4, 0, 0, p->buf))
    {
        yaz_log (LOG_FATAL, "read failure for pos=%ld block=%ld",
                 (long) pos, (long) pos/4);
        abort();
    }
    p->bytes = p->buf + ISAMB_DATA_OFFSET;
    p->leaf = p->buf[0];
    p->size = p->buf[1] + 256 * p->buf[2] - ISAMB_DATA_OFFSET;
    p->offset = 0;
    p->dirty = 0;
    p->decodeClientData = (*b->method->code_start)(ISAMC_DECODE);
    return p;
}

struct ISAMB_block *new_block (ISAMB b, int leaf, int cat)
{
    struct ISAMB_block *p;
    int block_no;
    
    p = xmalloc (sizeof(*p));
    block_no = b->file[cat].head.last_block++;
    p->cat = cat;
    p->pos = block_no * 4 + cat;
    p->cat = cat;
    b->file[cat].head_dirty = 1;
    p->buf = xmalloc (b->file[cat].head.block_size);
    memset (p->buf, 0, b->file[cat].head.block_size);
    p->bytes = p->buf + ISAMB_DATA_OFFSET;
    p->leaf = leaf;
    p->size = 0;
    p->dirty = 1;
    p->offset = 0;
    p->decodeClientData = (*b->method->code_start)(ISAMC_DECODE);
    return p;
}

struct ISAMB_block *new_leaf (ISAMB b, int cat)
{
    return new_block (b, 1, cat);
}


struct ISAMB_block *new_int (ISAMB b, int cat)
{
    return new_block (b, 0, cat);
}

void close_block (ISAMB b, struct ISAMB_block *p)
{
    if (!p)
        return;
    if (p->dirty)
    {
        int size = p->size + ISAMB_DATA_OFFSET;
        p->buf[0] = p->leaf;
        p->buf[1] = size & 255;
        p->buf[2] = size >> 8;
        bf_write (b->file[p->cat].bf, p->pos/4, 0, 0, p->buf);
    }
    (*b->method->code_stop)(ISAMC_DECODE, p->decodeClientData);
    xfree (p->buf);
    xfree (p);
}

int insert_sub (ISAMB b, struct ISAMB_block **p,
                void *new_item, int *mode,
                ISAMC_I stream,
                struct ISAMB_block **sp,
                void *sub_item, int *sub_size,
                void *max_item);

int insert_int (ISAMB b, struct ISAMB_block *p, void *lookahead_item,
                int *mode,
                ISAMC_I stream, struct ISAMB_block **sp,
                void *split_item, int *split_size)
{
    char *startp = p->bytes;
    char *src = startp;
    char *endp = p->bytes + p->size;
    int pos;
    struct ISAMB_block *sub_p1 = 0, *sub_p2 = 0;
    char sub_item[DST_ITEM_MAX];
    int sub_size;
    int more;

    *sp = 0;

    decode_ptr (&src, &pos);
    while (src != endp)
    {
        int item_len;
        int d;
        decode_ptr (&src, &item_len);
        d = (*b->method->compare_item)(src, lookahead_item);
        if (d > 0)
        {
            sub_p1 = open_block (b, pos);
            assert (sub_p1);
            more = insert_sub (b, &sub_p1, lookahead_item, mode,
                               stream, &sub_p2, 
                               sub_item, &sub_size, src);
            break;
        }
        src += item_len;
        decode_ptr (&src, &pos);
    }
    if (!sub_p1)
    {
        sub_p1 = open_block (b, pos);
        assert (sub_p1);
        more = insert_sub (b, &sub_p1, lookahead_item, mode, stream, &sub_p2, 
                           sub_item, &sub_size, 0);
    }
    if (sub_p2)
    {
        /* there was a split - must insert pointer in this one */
        char dst_buf[DST_BUF_SIZE];
        char *dst = dst_buf;

        assert (sub_size < 30 && sub_size > 1);

        memcpy (dst, startp, src - startp);
                
        dst += src - startp;

        encode_ptr (&dst, sub_size);      /* sub length and item */
        memcpy (dst, sub_item, sub_size);
        dst += sub_size;

        encode_ptr (&dst, sub_p2->pos);   /* pos */

        if (endp - src)                   /* remaining data */
        {
            memcpy (dst, src, endp - src);
            dst += endp - src;
        }
        p->size = dst - dst_buf;
        if (p->size <= b->file[p->cat].head.block_max)
        {
            memcpy (startp, dst_buf, dst - dst_buf);
        }
        else
        {
            int p_new_size;
            char *half;
            src = dst_buf;
            endp = dst;

            half = src + b->file[p->cat].head.block_size/2;
            decode_ptr (&src, &pos);
            while (src <= half)
            {
                decode_ptr (&src, split_size);
                src += *split_size;
                decode_ptr (&src, &pos);
            }
            p_new_size = src - dst_buf;
            memcpy (p->bytes, dst_buf, p_new_size);

            decode_ptr (&src, split_size);
            memcpy (split_item, src, *split_size);
            src += *split_size;

            *sp = new_int (b, p->cat);
            (*sp)->size = endp - src;
            memcpy ((*sp)->bytes, src, (*sp)->size);

            p->size = p_new_size;
        }
        p->dirty = 1;
        close_block (b, sub_p2);
    }
    close_block (b, sub_p1);
    return more;
}


int insert_leaf (ISAMB b, struct ISAMB_block **sp1, void *lookahead_item,
                 int *lookahead_mode, ISAMC_I stream, struct ISAMB_block **sp2,
                 void *sub_item, int *sub_size,
                 void *max_item)
{
    struct ISAMB_block *p = *sp1;
    char *src = 0, *endp = 0;
    char dst_buf[DST_BUF_SIZE], *dst = dst_buf;
    int new_size;
    void *c1 = (*b->method->code_start)(ISAMC_DECODE);
    void *c2 = (*b->method->code_start)(ISAMC_ENCODE);
    int more = 1;
    int quater = b->file[b->no_cat-1].head.block_max / 4;
    char *cut = dst_buf + quater * 2;
    char *maxp = dst_buf + b->file[b->no_cat-1].head.block_max;
    char *half1 = 0;
    char *half2 = 0;
    char cut_item_buf[DST_ITEM_MAX];
    int cut_item_size = 0;

    if (p && p->size)
    {
        char file_item_buf[DST_ITEM_MAX];
        char *file_item = file_item_buf;
            
        src = p->bytes;
        endp = p->bytes + p->size;
        (*b->method->code_item)(ISAMC_DECODE, c1, &file_item, &src);
        while (1)
        {
            char *dst_item = 0;
            char *dst_0 = dst;
            char *lookahead_next;
            int d = -1;
            
            if (lookahead_item)
                d = (*b->method->compare_item)(file_item_buf, lookahead_item);
            
            if (d > 0)
            {
                dst_item = lookahead_item;
                assert (*lookahead_mode);
            }
            else
                dst_item = file_item_buf;
            if (!*lookahead_mode && d == 0)
            {
                p->dirty = 1;
            }
            else if (!half1 && dst > cut)
            {
                char *dst_item_0 = dst_item;
                half1 = dst; /* candidate for splitting */
                
                (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &dst_item);
                
                cut_item_size = dst_item - dst_item_0;
                memcpy (cut_item_buf, dst_item_0, cut_item_size);
                
                half2 = dst;
            }
            else
                (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &dst_item);
            if (d > 0)  
            {
                if (dst > maxp)
                {
                    dst = dst_0;
                    lookahead_item = 0;
                }
                else
                {
                    lookahead_next = lookahead_item;
                    if (!(*stream->read_item)(stream->clientData,
                                              &lookahead_next,
                                              lookahead_mode))
                    {
                        lookahead_item = 0;
                        more = 0;
                    }
                    if (max_item &&
                        (*b->method->compare_item)(max_item, lookahead_item) <= 0)
                    {
			yaz_log (LOG_LOG, "max_item 1");
                        lookahead_item = 0;
                    }
                    
                    p->dirty = 1;
                }
            }
            else if (d == 0)
            {
                lookahead_next = lookahead_item;
                if (!(*stream->read_item)(stream->clientData,
                                          &lookahead_next, lookahead_mode))
                {
                    lookahead_item = 0;
                    more = 0;
                }
                if (src == endp)
                    break;
                file_item = file_item_buf;
                (*b->method->code_item)(ISAMC_DECODE, c1, &file_item, &src);
            }
            else
            {
                if (src == endp)
                    break;
                file_item = file_item_buf;
                (*b->method->code_item)(ISAMC_DECODE, c1, &file_item, &src);
            }
        }
    }
    maxp = dst_buf + b->file[b->no_cat-1].head.block_max + quater;
    while (lookahead_item)
    {
        char *dst_item = lookahead_item;
        char *dst_0 = dst;
        
        if (max_item &&
            (*b->method->compare_item)(max_item, lookahead_item) <= 0)
        {
 	    yaz_log (LOG_LOG, "max_item 2");
            break;
        }
        if (!*lookahead_mode)
        {
            yaz_log (LOG_WARN, "Inconsistent register (2)");
            abort();
        }
        else if (!half1 && dst > cut)   
        {
            char *dst_item_0 = dst_item;
            half1 = dst; /* candidate for splitting */
            
            (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &dst_item);
            
            cut_item_size = dst_item - dst_item_0;
            memcpy (cut_item_buf, dst_item_0, cut_item_size);
            
            half2 = dst;
        }
        else
            (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &dst_item);

        if (dst > maxp)
        {
            dst = dst_0;
            break;
        }
        if (p)
            p->dirty = 1;
        dst_item = lookahead_item;
        if (!(*stream->read_item)(stream->clientData, &dst_item,
                                  lookahead_mode))
        {
            lookahead_item = 0;
            more = 0;
        }
    }
    new_size = dst - dst_buf;
    if (p && p->cat != b->no_cat-1 && 
        new_size > b->file[p->cat].head.block_max)
    {
        /* non-btree block will be removed */
        close_block (b, p);
        /* delete it too!! */
        p = 0; /* make a new one anyway */
    }
    if (!p)
    {   /* must create a new one */
        int i;
        for (i = 0; i < b->no_cat; i++)
            if (new_size <= b->file[i].head.block_max)
                break;
        if (i == b->no_cat)
            i = b->no_cat - 1;
        p = new_leaf (b, i);
    }
    if (new_size > b->file[p->cat].head.block_max)
    {
        char *first_dst;
        char *cut_item = cut_item_buf;

        assert (half1);
        assert (half2);

       /* first half */
        p->size = half1 - dst_buf;
        memcpy (p->bytes, dst_buf, half1 - dst_buf);

        /* second half */
        *sp2 = new_leaf (b, p->cat);

        (*b->method->code_reset)(c2);

        first_dst = (*sp2)->bytes;

        (*b->method->code_item)(ISAMC_ENCODE, c2, &first_dst, &cut_item);

        memcpy (first_dst, half2, dst - half2);

        (*sp2)->size = (first_dst - (*sp2)->bytes) + (dst - half2);
        (*sp2)->dirty = 1;
        p->dirty = 1;
        memcpy (sub_item, cut_item_buf, cut_item_size);
        *sub_size = cut_item_size;
    }
    else
    {
        memcpy (p->bytes, dst_buf, dst - dst_buf);
        p->size = new_size;
    }
    (*b->method->code_stop)(ISAMC_DECODE, c1);
    (*b->method->code_stop)(ISAMC_ENCODE, c2);
    *sp1 = p;
    return more;
}

int insert_sub (ISAMB b, struct ISAMB_block **p, void *new_item,
                int *mode,
                ISAMC_I stream,
                struct ISAMB_block **sp,
                void *sub_item, int *sub_size,
                void *max_item)
{
    if (!*p || (*p)->leaf)
        return insert_leaf (b, p, new_item, mode, stream, sp, sub_item, 
                            sub_size, max_item);
    else
        return insert_int (b, *p, new_item, mode, stream, sp, sub_item,
                           sub_size);
}

int isamb_merge (ISAMB b, ISAMC_P pos, ISAMC_I stream)
{
    char item_buf[DST_ITEM_MAX];
    char *item_ptr;
    int i_mode;
    int more;

    item_ptr = item_buf;
    more = (*stream->read_item)(stream->clientData, &item_ptr, &i_mode);
    while (more)
    {
        struct ISAMB_block *p = 0, *sp = 0;
        char sub_item[DST_ITEM_MAX];
        int sub_size;
        
        if (pos)
            p = open_block (b, pos);
        more = insert_sub (b, &p, item_buf, &i_mode, stream, &sp,
                            sub_item, &sub_size, 0);
        if (sp)
        {    /* increase level of tree by one */
            struct ISAMB_block *p2 = new_int (b, p->cat);
            char *dst = p2->bytes + p2->size;
            
            encode_ptr (&dst, p->pos);
            assert (sub_size < 20);
            encode_ptr (&dst, sub_size);
            memcpy (dst, sub_item, sub_size);
            dst += sub_size;
            encode_ptr (&dst, sp->pos);
            
            p2->size = dst - p2->bytes;
            pos = p2->pos;  /* return new super page */
            close_block (b, sp);
            close_block (b, p2);
        }
        else
            pos = p->pos;   /* return current one (again) */
        close_block (b, p);
    }
    return pos;
}

ISAMB_PP isamb_pp_open_x (ISAMB isamb, ISAMB_P pos, int *level)
{
    ISAMB_PP pp = xmalloc (sizeof(*pp));

    pp->isamb = isamb;
    pp->block = xmalloc (10 * sizeof(*pp->block));

    pp->pos = pos;
    pp->level = 0;
    pp->total_size = 0;
    pp->no_blocks = 0;
    while (1)
    {
        struct ISAMB_block *p = open_block (isamb, pos);
        char *src = p->bytes + p->offset;
        pp->block[pp->level] = p;

        pp->total_size += p->size;
        pp->no_blocks++;

        if (p->leaf)
            break;

        decode_ptr (&src, &pos);
        p->offset = src - p->bytes;
        pp->level++;
    }
    pp->block[pp->level+1] = 0;
    if (level)
        *level = pp->level;
    return pp;
}

ISAMB_PP isamb_pp_open (ISAMB isamb, ISAMB_P pos)
{
    return isamb_pp_open_x (isamb, pos, 0);
}

void isamb_pp_close_x (ISAMB_PP pp, int *size, int *blocks)
{
    int i;
    if (!pp)
        return;
    if (size)
        *size = pp->total_size;
    if (blocks)
        *blocks = pp->no_blocks;
    for (i = 0; i <= pp->level; i++)
        close_block (pp->isamb, pp->block[i]);
    xfree (pp->block);
    xfree (pp);
}

int isamb_block_info (ISAMB isamb, int cat)
{
    if (cat >= 0 && cat < isamb->no_cat)
        return isamb->file[cat].head.block_size;
    return -1;
}

void isamb_pp_close (ISAMB_PP pp)
{
    return isamb_pp_close_x (pp, 0, 0);
}

int isamb_pp_read (ISAMB_PP pp, void *buf)
{
    char *dst = buf;
    char *src;
    struct ISAMB_block *p = pp->block[pp->level];
    if (!p)
        return 0;

    while (p->offset == p->size)
    {
        int pos, item_len;
        while (p->offset == p->size)
        {
            if (pp->level == 0)
                return 0;
            close_block (pp->isamb, pp->block[pp->level]);
            pp->block[pp->level] = 0;
            (pp->level)--;
            p = pp->block[pp->level];
            assert (!p->leaf);  /* must be int */
        }
        src = p->bytes + p->offset;
        
        decode_ptr (&src, &item_len);
        src += item_len;
        decode_ptr (&src, &pos);
        
        p->offset = src - (char*) p->bytes;

        ++(pp->level);
        
        while (1)
        {
            pp->block[pp->level] = p = open_block (pp->isamb, pos);

            pp->total_size += p->size;
            pp->no_blocks++;
            
            if (p->leaf) /* leaf */
            {
                break;
            }
            src = p->bytes + p->offset;
            decode_ptr (&src, &pos);
            p->offset = src - (char*) p->bytes;
            pp->level++;
        }
    }
    assert (p->offset < p->size);
    assert (p->leaf);
    src = p->bytes + p->offset;
    (*pp->isamb->method->code_item)(ISAMC_DECODE, p->decodeClientData,
                                    &dst, &src);
    p->offset = src - (char*) p->bytes;
    return 1;
}

int isamb_pp_num (ISAMB_PP pp)
{
    return 1;
}
