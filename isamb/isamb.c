/*
 *  Copyright (c) 1995-1998, Index Data.
 *  See the file LICENSE for details.
 *
 *  $Id: isamb.c,v 1.6 2002-04-17 09:03:38 adam Exp $
 */
#include <yaz/xmalloc.h>
#include <yaz/log.h>
#include <isamb.h>
#include <assert.h>

struct ISAMB_head {
    int first_block;
    int last_block;
    int block_size;
};

#define ISAMB_DATA_OFFSET 3

#define DST_BUF_SIZE 4500
#define DST_ITEM_MAX 256

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
    int pos;
    int cat;
    int size;
    int leaf;
    int dirty;
    int offset;
    unsigned char *bytes;
    void *decodeClientData;
};

struct ISAMB_PP_s {
    ISAMB isamb;
    int level;
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
    isamb->no_cat = 2;

    isamb->file = xmalloc (sizeof(*isamb->file) * isamb->no_cat);
    for (i = 0; i<isamb->no_cat; i++)
    {
        char fname[DST_BUF_SIZE];
        isamb->file[i].head.first_block = 1;
        isamb->file[i].head.last_block = 1;
        isamb->file[i].head.block_size = b_size;
        b_size = b_size * 4;
        isamb->file[i].head_dirty = 0;
        sprintf (fname, "%s-%d", name, i);
        isamb->file[i].bf =
            bf_open (bfs, fname, isamb->file[i].head.block_size, writeflag);
    
        bf_read (isamb->file[i].bf, 0, 0, sizeof(struct ISAMB_head),
                 &isamb->file[i].head);
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
    p->bytes = xmalloc (b->file[cat].head.block_size);
    bf_read (b->file[cat].bf, pos/4, 0, 0, p->bytes);
    p->leaf = p->bytes[0];
    p->size = p->bytes[1] + 256 * p->bytes[2];
    p->offset = ISAMB_DATA_OFFSET;
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
    p->bytes = xmalloc (b->file[cat].head.block_size);
    memset (p->bytes, 0, b->file[cat].head.block_size);
    p->leaf = leaf;
    p->size = ISAMB_DATA_OFFSET;
    p->dirty = 1;
    p->offset = ISAMB_DATA_OFFSET;
    p->decodeClientData = (*b->method->code_start)(ISAMC_DECODE);
    return p;
}

void close_block (ISAMB b, struct ISAMB_block *p)
{
    if (!p)
        return;
    if (p->dirty)
    {
        p->bytes[0] = p->leaf;
        p->bytes[1] = p->size & 255;
        p->bytes[2] = p->size >> 8;
        bf_write (b->file[p->cat].bf, p->pos/4, 0, 0, p->bytes);
    }
    (*b->method->code_stop)(ISAMC_DECODE, p->decodeClientData);
    xfree (p->bytes);
    xfree (p);
}

void insert_sub (ISAMB b, struct ISAMB_block *p, const void *new_item,
                 struct ISAMB_block **sp,
                 void *sub_item, int *sub_size);

void insert_leaf (ISAMB b, struct ISAMB_block *p, const void *new_item,
                  struct ISAMB_block **sp,
                  void *sub_item, int *sub_size)
{
    char dst_buf[DST_BUF_SIZE];
    char *dst = dst_buf;
    char *src = p->bytes + ISAMB_DATA_OFFSET;
    char *endp = p->bytes + p->size;
    void *c1 = (*b->method->code_start)(ISAMC_DECODE);
    void *c2 = (*b->method->code_start)(ISAMC_ENCODE);
    char *half1 = 0;
    char *half2 = 0;
    char *cut = dst_buf + p->size / 2;
    char cut_item_buf[DST_ITEM_MAX];
    int cut_item_size = 0;

    while (src != endp)
    {
        char file_item_buf[DST_ITEM_MAX];
        char *file_item = file_item_buf;

        (*b->method->code_item)(ISAMC_DECODE, c1, &file_item, &src);
        if (new_item)
        {
            int d = (*b->method->compare_item)(file_item_buf, new_item);
            if (d > 0)
            {
                char *item_ptr = (char*) new_item;
                (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &item_ptr);
                new_item = 0;
                p->dirty = 1;
            }
            else if (d == 0)
            {
                new_item = 0;
            }
        }
        
        if (!half1 && dst > cut)   
        {
            half1 = dst; /* candidate for splitting */

            file_item = file_item_buf;
            (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &file_item);

            cut_item_size = file_item - file_item_buf;
            memcpy (cut_item_buf, file_item_buf, cut_item_size);

            half2 = dst;
        }
        else
        {
            file_item = file_item_buf;
            (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &file_item);
        }
    }
    if (new_item)
    {
        char *item_ptr = (char*) new_item;
        (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &item_ptr);
        new_item = 0;
        p->dirty = 1;
    }
    p->size = dst - dst_buf + ISAMB_DATA_OFFSET;
    if (p->size > b->file[p->cat].head.block_size)
    {
        char *first_dst;
        char *cut_item = cut_item_buf;
 
       /* first half */
        p->size = half1 - dst_buf + ISAMB_DATA_OFFSET;
        memcpy (p->bytes+ISAMB_DATA_OFFSET, dst_buf, half1 - dst_buf);

        /* second half */
        *sp = new_block (b, 1, p->cat);

        (*b->method->code_reset)(c2);

        first_dst = (*sp)->bytes + ISAMB_DATA_OFFSET;

        (*b->method->code_item)(ISAMC_ENCODE, c2, &first_dst, &cut_item);

        memcpy (first_dst, half2, dst - half2);

        (*sp)->size = (first_dst - (char*) (*sp)->bytes) + (dst - half2);
        (*sp)->dirty = 1;
        p->dirty = 1;
        memcpy (sub_item, cut_item_buf, cut_item_size);
        *sub_size = cut_item_size;

        yaz_log (LOG_LOG, "l split %d / %d", p->size, (*sp)->size);

    }
    else
    {
        assert (p->size > ISAMB_DATA_OFFSET);
        assert (p->size <= b->file[p->cat].head.block_size);
        memcpy (p->bytes+ISAMB_DATA_OFFSET, dst_buf, dst - dst_buf);
        *sp = 0;
    }
    (*b->method->code_stop)(ISAMC_DECODE, c1);
    (*b->method->code_stop)(ISAMC_ENCODE, c2);
}

void insert_int (ISAMB b, struct ISAMB_block *p, const void *new_item,
                 struct ISAMB_block **sp,
                 void *split_item, int *split_size)
{
    char *startp = p->bytes + ISAMB_DATA_OFFSET;
    char *src = startp;
    char *endp = p->bytes + p->size;
    int pos;
    struct ISAMB_block *sub_p1 = 0, *sub_p2 = 0;
    char sub_item[DST_ITEM_MAX];
    int sub_size;

    *sp = 0;

    decode_ptr (&src, &pos);
    while (src != endp)
    {
        int item_len;
        int d;
        decode_ptr (&src, &item_len);
        d = (*b->method->compare_item)(src, new_item);
        if (d > 0)
        {
            sub_p1 = open_block (b, pos);
            assert (sub_p1);
            insert_sub (b, sub_p1, new_item, &sub_p2, 
                        sub_item, &sub_size);
            break;
        }
        src += item_len;
        decode_ptr (&src, &pos);
    }
    if (!sub_p1)
    {
        sub_p1 = open_block (b, pos);
        assert (sub_p1);
        insert_sub (b, sub_p1, new_item, &sub_p2, 
                    sub_item, &sub_size);
    }
    if (sub_p2)
    {
        char dst_buf[DST_BUF_SIZE];
        char *dst = dst_buf;

        assert (sub_size < 20);

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
        p->size = dst - dst_buf + ISAMB_DATA_OFFSET;
        if (p->size <= b->file[p->cat].head.block_size)
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
            memcpy (p->bytes + ISAMB_DATA_OFFSET, dst_buf, p_new_size);
            p_new_size += ISAMB_DATA_OFFSET;

            decode_ptr (&src, split_size);
            memcpy (split_item, src, *split_size);
            src += *split_size;

            *sp = new_block (b, 0, p->cat);
            (*sp)->size = endp - src;
            memcpy ((*sp)->bytes+ISAMB_DATA_OFFSET, src, (*sp)->size);
            (*sp)->size += ISAMB_DATA_OFFSET;

            yaz_log (LOG_LOG, "i split %d -> %d %d",
                     p->size, p_new_size, (*sp)->size);
            p->size = p_new_size;
        }
        p->dirty = 1;
        close_block (b, sub_p2);
    }
    close_block (b, sub_p1);
}

void insert_sub (ISAMB b, struct ISAMB_block *p, const void *new_item,
                struct ISAMB_block **sp,
                void *sub_item, int *sub_size)
{
    if (p->leaf)
        insert_leaf (b, p, new_item, sp, sub_item, sub_size);
    else
        insert_int (b, p, new_item, sp, sub_item, sub_size);
}

int insert_flat (ISAMB b, const void *new_item, ISAMC_P *posp)
{
    struct ISAMB_block *p = 0;
    char *src = 0, *endp = 0;
    char dst_buf[DST_BUF_SIZE], *dst = dst_buf;
    int new_size;
    ISAMB_P pos = *posp;
    void *c1 = (*b->method->code_start)(ISAMC_DECODE);
    void *c2 = (*b->method->code_start)(ISAMC_ENCODE);
    
    if (pos)
    {
        p = open_block (b, pos);
        if (!p)
            return -1;
        src = p->bytes + ISAMB_DATA_OFFSET;
        endp = p->bytes + p->size;

    }
    while (p && src != endp)
    {
        char file_item_buf[DST_ITEM_MAX];
        char *file_item = file_item_buf;

        (*b->method->code_item)(ISAMC_DECODE, c1, &file_item, &src);
        if (new_item)
        {
            int d = (*b->method->compare_item)(file_item_buf, new_item);
            if (d > 0)
            {
                char *item_ptr = (char*) new_item;
                (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &item_ptr);
                new_item = 0;
                p->dirty = 1;
            }
            else if (d == 0)
            {
                new_item = 0;
            }
        }
        file_item = file_item_buf;
        (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &file_item);
    }
    if (new_item)
    {
        char *item_ptr = (char*) new_item;
        (*b->method->code_item)(ISAMC_ENCODE, c2, &dst, &item_ptr);
        new_item = 0;
        if (p)
            p->dirty = 1;
    }
    new_size = dst - dst_buf + ISAMB_DATA_OFFSET;
    if (p && new_size > b->file[p->cat].head.block_size)
    {
        yaz_log (LOG_LOG, "resize %d -> %d", p->size, new_size);
        close_block (b, p);
        /* delete it too!! */
        p = 0; /* make a new one anyway */
    }
    if (!p)
    {   /* must create a new one */
        int i;
        for (i = 0; i < b->no_cat; i++)
            if (new_size <= b->file[i].head.block_size)
                break;
        p = new_block (b, 1, i);
    }
    memcpy (p->bytes+ISAMB_DATA_OFFSET, dst_buf, dst - dst_buf);
    p->size = new_size;
    *posp = p->pos;
    (*b->method->code_stop)(ISAMC_DECODE, c1);
    (*b->method->code_stop)(ISAMC_ENCODE, c2);
    close_block (b, p);
    return 0;
}

int isamb_insert_one (ISAMB b, const void *item, ISAMC_P pos)
{

    if ((pos & 3) != b->no_cat-1)
    {
        /* note if pos == 0 we go here too! */
        /* flat insert */
        insert_flat (b, item, &pos);
    }
    else
    {
        /* b-tree insert */
        struct ISAMB_block *p = open_block (b, pos), *sp = 0;
        char sub_item[DST_ITEM_MAX];
        int sub_size;


        insert_sub (b, p, item, &sp, sub_item, &sub_size);
        if (sp)
        {    /* increase level of tree by one */
            struct ISAMB_block *p2 = new_block (b, 0, p->cat);
            char *dst = p2->bytes + p2->size;
            
            encode_ptr (&dst, p->pos);
            assert (sub_size < 20);
            encode_ptr (&dst, sub_size);
            memcpy (dst, sub_item, sub_size);
            dst += sub_size;
            encode_ptr (&dst, sp->pos);
            
            p2->size = dst - (char*) p2->bytes;
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

ISAMB_P isamb_merge (ISAMB b, ISAMB_P pos, ISAMC_I data)
{
    int i_mode;
    char item_buf[DST_ITEM_MAX];
    char *item_ptr = item_buf;
    while ((*data->read_item)(data->clientData, &item_ptr, &i_mode))
    {
        item_ptr = item_buf;
        pos = isamb_insert_one (b, item_buf, pos);
    }
    return pos;
}

ISAMB_PP isamb_pp_open (ISAMB isamb, ISAMB_P pos)
{
    ISAMB_PP pp = xmalloc (sizeof(*pp));

    pp->isamb = isamb;
    pp->block = xmalloc (10 * sizeof(*pp->block));

    pp->level = 0;
    while (1)
    {
        struct ISAMB_block *p = open_block (isamb, pos);
        char *src = p->bytes + p->offset;
        pp->block[pp->level] = p;

        if (p->bytes[0]) /* leaf */
            break;

        decode_ptr (&src, &pos);
        p->offset = src - (char*) p->bytes;
        pp->level++;
    }
    pp->block[pp->level+1] = 0;
    return pp;
}

void isamb_pp_close (ISAMB_PP pp)
{
    int i;
    if (!pp)
        return;
    for (i = 0; i <= pp->level; i++)
        close_block (pp->isamb, pp->block[i]);
    xfree (pp->block);
    xfree (pp);
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
            assert (p->bytes[0] == 0);  /* must be int */
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
            
            if (p->bytes[0]) /* leaf */
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
    assert (p->bytes[0]);
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
