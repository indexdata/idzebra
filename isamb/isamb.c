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
#include <string.h>
#include <yaz/log.h>
#include <yaz/xmalloc.h>
#include <idzebra/isamb.h>
#include <assert.h>

#ifndef ISAMB_DEBUG
#define ISAMB_DEBUG 0
#endif


#define ISAMB_MAJOR_VERSION 3
#define ISAMB_MINOR_VERSION_NO_ROOT 0
#define ISAMB_MINOR_VERSION_WITH_ROOT 1

struct ISAMB_head {
    zint first_block;
    zint last_block;
    zint free_list;
    zint no_items;
    int block_size;
    int block_max;
    int block_offset;
};

/* if 1, upper nodes items are encoded; 0 if not encoded */
#define INT_ENCODE 1

/* maximum size of encoded buffer */
#define DST_ITEM_MAX 5000

/* max page size for _any_ isamb use */
#define ISAMB_MAX_PAGE 32768

#define ISAMB_MAX_LEVEL 10
/* approx 2*max page + max size of item */
#define DST_BUF_SIZE (2*ISAMB_MAX_PAGE+DST_ITEM_MAX+100)

/* should be maximum block size of multiple thereof */
#define ISAMB_CACHE_ENTRY_SIZE ISAMB_MAX_PAGE

/* CAT_MAX: _must_ be power of 2 */
#define CAT_MAX 4
#define CAT_MASK (CAT_MAX-1)
/* CAT_NO: <= CAT_MAX */
#define CAT_NO 4

/* Smallest block size */
#define ISAMB_MIN_SIZE 32
/* Size factor */
#define ISAMB_FAC_SIZE 4

/* ISAMB_PTR_CODEC = 1 var, =0 fixed */
#define ISAMB_PTR_CODEC 1

struct ISAMB_cache_entry {
    ISAM_P pos;
    unsigned char *buf;
    int dirty;
    int hits;
    struct ISAMB_cache_entry *next;
};

struct ISAMB_file {
    BFile bf;
    int head_dirty;
    struct ISAMB_head head;
    struct ISAMB_cache_entry *cache_entries;
};

struct ISAMB_s {
    BFiles bfs;
    ISAMC_M *method;

    struct ISAMB_file *file;
    int no_cat;
    int cache; /* 0 = no cache, 1 = use cache, -1 = dummy isam (for testing only) */
    int log_io;        /* log level for bf_read/bf_write calls */
    int log_freelist;  /* log level for freelist handling */
    zint skipped_numbers; /* on a leaf node */
    zint returned_numbers;
    zint skipped_nodes[ISAMB_MAX_LEVEL]; /* [0]=skipped leaves, 1 = higher etc */
    zint accessed_nodes[ISAMB_MAX_LEVEL]; /* nodes we did not skip */
    zint number_of_int_splits;
    zint number_of_leaf_splits;
    int enable_int_count; /* whether we count nodes (or not) */
    int cache_size; /* size of blocks to cache (if cache=1) */
    int minor_version;
    zint root_ptr;
};

struct ISAMB_block {
    ISAM_P pos;
    int cat;
    int size;
    int leaf;
    int dirty;
    int deleted;
    int offset;
    zint no_items;  /* number of nodes in this + children */
    char *bytes;
    char *cbuf;
    unsigned char *buf;
    void *decodeClientData;
    int log_rw;
};

struct ISAMB_PP_s {
    ISAMB isamb;
    ISAM_P pos;
    int level;
    int maxlevel; /* total depth */
    zint total_size;
    zint no_blocks;
    zint skipped_numbers; /* on a leaf node */
    zint returned_numbers;
    zint skipped_nodes[ISAMB_MAX_LEVEL]; /* [0]=skipped leaves, 1 = higher etc */
    zint accessed_nodes[ISAMB_MAX_LEVEL]; /* nodes we did not skip */
    struct ISAMB_block **block;
    int scope;  /* on what level we forward */
};


#define encode_item_len encode_ptr
#if ISAMB_PTR_CODEC
static void encode_ptr(char **dst, zint pos)
{
    unsigned char *bp = (unsigned char*) *dst;

    while (pos > 127)
    {
        *bp++ = (unsigned char) (128 | (pos & 127));
        pos = pos >> 7;
    }
    *bp++ = (unsigned char) pos;
    *dst = (char *) bp;
}
#else
static void encode_ptr(char **dst, zint pos)
{
    memcpy(*dst, &pos, sizeof(pos));
    (*dst) += sizeof(pos);
}
#endif

#define decode_item_len decode_ptr
#if ISAMB_PTR_CODEC
static void decode_ptr(const char **src, zint *pos)
{
    zint d = 0;
    unsigned char c;
    unsigned r = 0;

    while (((c = *(const unsigned char *)((*src)++)) & 128))
    {
        d += ((zint) (c & 127) << r);
	r += 7;
    }
    d += ((zint) c << r);
    *pos = d;
}
#else
static void decode_ptr(const char **src, zint *pos)
{
    memcpy(pos, *src, sizeof(*pos));
    (*src) += sizeof(*pos);
}
#endif


void isamb_set_int_count(ISAMB b, int v)
{
    b->enable_int_count = v;
}

void isamb_set_cache_size(ISAMB b, int v)
{
    b->cache_size = v;
}

ISAMB isamb_open2(BFiles bfs, const char *name, int writeflag, ISAMC_M *method,
                  int cache, int no_cat, int *sizes, int use_root_ptr)
{
    ISAMB isamb = xmalloc(sizeof(*isamb));
    int i;

    assert(no_cat <= CAT_MAX);

    isamb->bfs = bfs;
    isamb->method = (ISAMC_M *) xmalloc(sizeof(*method));
    memcpy(isamb->method, method, sizeof(*method));
    isamb->no_cat = no_cat;
    isamb->log_io = 0;
    isamb->log_freelist = 0;
    isamb->cache = cache;
    isamb->skipped_numbers = 0;
    isamb->returned_numbers = 0;
    isamb->number_of_int_splits = 0;
    isamb->number_of_leaf_splits = 0;
    isamb->enable_int_count = 1;
    isamb->cache_size = 40;

    if (use_root_ptr)
        isamb->minor_version = ISAMB_MINOR_VERSION_WITH_ROOT;
    else
        isamb->minor_version = ISAMB_MINOR_VERSION_NO_ROOT;

    isamb->root_ptr = 0;

    for (i = 0; i<ISAMB_MAX_LEVEL; i++)
	isamb->skipped_nodes[i] = isamb->accessed_nodes[i] = 0;

    if (cache == -1)
    {
        yaz_log(YLOG_WARN, "isamb_open %s. Degraded TEST mode", name);
    }
    else
    {
        assert(cache == 0 || cache == 1);
    }
    isamb->file = xmalloc(sizeof(*isamb->file) * isamb->no_cat);

    for (i = 0; i < isamb->no_cat; i++)
    {
        isamb->file[i].bf = 0;
        isamb->file[i].head_dirty = 0;
        isamb->file[i].cache_entries = 0;
    }

    for (i = 0; i < isamb->no_cat; i++)
    {
        char fname[DST_BUF_SIZE];
	char hbuf[DST_BUF_SIZE];

        sprintf(fname, "%s%c", name, i+'A');
        if (cache)
            isamb->file[i].bf = bf_open(bfs, fname, ISAMB_CACHE_ENTRY_SIZE,
                                        writeflag);
        else
            isamb->file[i].bf = bf_open(bfs, fname, sizes[i], writeflag);

        if (!isamb->file[i].bf)
        {
            isamb_close(isamb);
            return 0;
        }

        /* fill-in default values (for empty isamb) */
	isamb->file[i].head.first_block = ISAMB_CACHE_ENTRY_SIZE/sizes[i]+1;
	isamb->file[i].head.last_block = isamb->file[i].head.first_block;
	isamb->file[i].head.block_size = sizes[i];
        assert(sizes[i] <= ISAMB_CACHE_ENTRY_SIZE);
#if ISAMB_PTR_CODEC
	if (i == isamb->no_cat-1 || sizes[i] > 128)
	    isamb->file[i].head.block_offset = 8;
	else
	    isamb->file[i].head.block_offset = 4;
#else
	isamb->file[i].head.block_offset = 11;
#endif
	isamb->file[i].head.block_max =
	    sizes[i] - isamb->file[i].head.block_offset;
	isamb->file[i].head.free_list = 0;
	if (bf_read(isamb->file[i].bf, 0, 0, 0, hbuf))
	{
	    /* got header assume "isamb"major minor len can fit in 16 bytes */
	    zint zint_tmp;
	    int major, minor, len, pos = 0;
	    int left;
	    const char *src = 0;
	    if (memcmp(hbuf, "isamb", 5))
	    {
		yaz_log(YLOG_WARN, "bad isamb header for file %s", fname);
                isamb_close(isamb);
		return 0;
	    }
	    if (sscanf(hbuf+5, "%d %d %d", &major, &minor, &len) != 3)
	    {
		yaz_log(YLOG_WARN, "bad isamb header for file %s", fname);
                isamb_close(isamb);
		return 0;
	    }
	    if (major != ISAMB_MAJOR_VERSION)
	    {
		yaz_log(YLOG_WARN, "bad major version for file %s %d, must be %d",
                        fname, major, ISAMB_MAJOR_VERSION);
                isamb_close(isamb);
		return 0;
	    }
	    for (left = len - sizes[i]; left > 0; left = left - sizes[i])
	    {
		pos++;
		if (!bf_read(isamb->file[i].bf, pos, 0, 0, hbuf + pos*sizes[i]))
		{
		    yaz_log(YLOG_WARN, "truncated isamb header for "
                            "file=%s len=%d pos=%d",
                            fname, len, pos);
                    isamb_close(isamb);
		    return 0;
		}
	    }
	    src = hbuf + 16;
	    decode_ptr(&src, &isamb->file[i].head.first_block);
	    decode_ptr(&src, &isamb->file[i].head.last_block);
	    decode_ptr(&src, &zint_tmp);
	    isamb->file[i].head.block_size = (int) zint_tmp;
	    decode_ptr(&src, &zint_tmp);
	    isamb->file[i].head.block_max = (int) zint_tmp;
	    decode_ptr(&src, &isamb->file[i].head.free_list);
            if (isamb->minor_version >= ISAMB_MINOR_VERSION_WITH_ROOT)
                decode_ptr(&src, &isamb->root_ptr);
	}
        assert(isamb->file[i].head.block_size >= isamb->file[i].head.block_offset);
        /* must rewrite the header if root ptr is in use (bug #1017) */
        if (use_root_ptr && writeflag)
            isamb->file[i].head_dirty = 1;
        else
            isamb->file[i].head_dirty = 0;
        assert(isamb->file[i].head.block_size == sizes[i]);
    }
#if ISAMB_DEBUG
    yaz_log(YLOG_WARN, "isamb debug enabled. Things will be slower than usual");
#endif
    return isamb;
}

ISAMB isamb_open(BFiles bfs, const char *name, int writeflag, ISAMC_M *method,
		 int cache)
{
    int sizes[CAT_NO];
    int i, b_size = ISAMB_MIN_SIZE;

    for (i = 0; i<CAT_NO; i++)
    {
        sizes[i] = b_size;
        b_size = b_size * ISAMB_FAC_SIZE;
    }
    return isamb_open2(bfs, name, writeflag, method, cache,
                       CAT_NO, sizes, 0);
}

static void flush_blocks(ISAMB b, int cat)
{
    while (b->file[cat].cache_entries)
    {
        struct ISAMB_cache_entry *ce_this = b->file[cat].cache_entries;
        b->file[cat].cache_entries = ce_this->next;

        if (ce_this->dirty)
        {
            yaz_log(b->log_io, "bf_write: flush_blocks");
            bf_write(b->file[cat].bf, ce_this->pos, 0, 0, ce_this->buf);
        }
        xfree(ce_this->buf);
        xfree(ce_this);
    }
}

static int cache_block(ISAMB b, ISAM_P pos, unsigned char *userbuf, int wr)
{
    int cat = (int) (pos&CAT_MASK);
    int off = (int) (((pos/CAT_MAX) &
                      (ISAMB_CACHE_ENTRY_SIZE / b->file[cat].head.block_size - 1))
                     * b->file[cat].head.block_size);
    zint norm = pos / (CAT_MASK*ISAMB_CACHE_ENTRY_SIZE / b->file[cat].head.block_size);
    int no = 0;
    struct ISAMB_cache_entry **ce, *ce_this = 0, **ce_last = 0;

    if (!b->cache)
        return 0;

    assert(ISAMB_CACHE_ENTRY_SIZE >= b->file[cat].head.block_size);
    for (ce = &b->file[cat].cache_entries; *ce; ce = &(*ce)->next, no++)
    {
        ce_last = ce;
        if ((*ce)->pos == norm)
        {
            ce_this = *ce;
            *ce = (*ce)->next;   /* remove from list */

            ce_this->next = b->file[cat].cache_entries;  /* move to front */
            b->file[cat].cache_entries = ce_this;

            if (wr)
            {
                memcpy(ce_this->buf + off, userbuf,
                       b->file[cat].head.block_size);
                ce_this->dirty = 1;
            }
            else
                memcpy(userbuf, ce_this->buf + off,
                       b->file[cat].head.block_size);
            return 1;
        }
    }
    if (no >= b->cache_size)
    {
        assert(ce_last && *ce_last);
        ce_this = *ce_last;
        *ce_last = 0;  /* remove the last entry from list */
        if (ce_this->dirty)
        {
            yaz_log(b->log_io, "bf_write: cache_block");
            bf_write(b->file[cat].bf, ce_this->pos, 0, 0, ce_this->buf);
        }
        xfree(ce_this->buf);
        xfree(ce_this);
    }
    ce_this = xmalloc(sizeof(*ce_this));
    ce_this->next = b->file[cat].cache_entries;
    b->file[cat].cache_entries = ce_this;
    ce_this->buf = xmalloc(ISAMB_CACHE_ENTRY_SIZE);
    ce_this->pos = norm;
    yaz_log(b->log_io, "bf_read: cache_block");
    if (!bf_read(b->file[cat].bf, norm, 0, 0, ce_this->buf))
        memset(ce_this->buf, 0, ISAMB_CACHE_ENTRY_SIZE);
    if (wr)
    {
        memcpy(ce_this->buf + off, userbuf, b->file[cat].head.block_size);
        ce_this->dirty = 1;
    }
    else
    {
        ce_this->dirty = 0;
        memcpy(userbuf, ce_this->buf + off, b->file[cat].head.block_size);
    }
    return 1;
}


void isamb_close(ISAMB isamb)
{
    int i;
    for (i = 0; isamb->accessed_nodes[i]; i++)
        yaz_log(YLOG_DEBUG, "isamb_close  level leaf-%d: "ZINT_FORMAT" read, "
                ZINT_FORMAT" skipped",
                i, isamb->accessed_nodes[i], isamb->skipped_nodes[i]);
    yaz_log(YLOG_DEBUG, "isamb_close returned "ZINT_FORMAT" values, "
            "skipped "ZINT_FORMAT,
            isamb->skipped_numbers, isamb->returned_numbers);

    for (i = 0; i<isamb->no_cat; i++)
    {
        flush_blocks(isamb, i);
        if (isamb->file[i].head_dirty)
	{
	    char hbuf[DST_BUF_SIZE];
	    int major = ISAMB_MAJOR_VERSION;
	    int len = 16;
	    char *dst = hbuf + 16;
	    int pos = 0, left;
	    int b_size = isamb->file[i].head.block_size;

	    encode_ptr(&dst, isamb->file[i].head.first_block);
	    encode_ptr(&dst, isamb->file[i].head.last_block);
	    encode_ptr(&dst, isamb->file[i].head.block_size);
	    encode_ptr(&dst, isamb->file[i].head.block_max);
	    encode_ptr(&dst, isamb->file[i].head.free_list);

            if (isamb->minor_version >= ISAMB_MINOR_VERSION_WITH_ROOT)
                encode_ptr(&dst, isamb->root_ptr);

	    memset(dst, '\0', b_size); /* ensure no random bytes are written */

	    len = dst - hbuf;

	    /* print exactly 16 bytes (including trailing 0) */
	    sprintf(hbuf, "isamb%02d %02d %02d\r\n", major,
                    isamb->minor_version, len);

            bf_write(isamb->file[i].bf, pos, 0, 0, hbuf);

	    for (left = len - b_size; left > 0; left = left - b_size)
	    {
		pos++;
		bf_write(isamb->file[i].bf, pos, 0, 0, hbuf + pos*b_size);
	    }
	}
        if (isamb->file[i].bf)
            bf_close (isamb->file[i].bf);
    }
    xfree(isamb->file);
    xfree(isamb->method);
    xfree(isamb);
}

/* open_block: read one block at pos.
   Decode leading sys bytes .. consisting of
   Offset:Meaning
   0: leader byte, != 0 leaf, == 0, non-leaf
   1-2: used size of block
   3-7*: number of items and all children

   * Reserve 5 bytes for large block sizes. 1 for small ones .. Number
   of items. We can thus have at most 2^40 nodes.
*/
static struct ISAMB_block *open_block(ISAMB b, ISAM_P pos)
{
    int cat = (int) (pos&CAT_MASK);
    const char *src;
    int offset = b->file[cat].head.block_offset;
    struct ISAMB_block *p;
    if (!pos)
        return 0;
    p = xmalloc(sizeof(*p));
    p->pos = pos;
    p->cat = (int) (pos & CAT_MASK);
    p->buf = xmalloc(b->file[cat].head.block_size);
    p->cbuf = 0;

    if (!cache_block (b, pos, p->buf, 0))
    {
        yaz_log(b->log_io, "bf_read: open_block");
        if (bf_read(b->file[cat].bf, pos/CAT_MAX, 0, 0, p->buf) != 1)
        {
            yaz_log(YLOG_FATAL, "isamb: read fail for pos=%ld block=%ld",
                    (long) pos, (long) pos/CAT_MAX);
            zebra_exit("isamb:open_block");
        }
    }
    p->bytes = (char *)p->buf + offset;
    p->leaf = p->buf[0];
    p->size = (p->buf[1] + 256 * p->buf[2]) - offset;
    if (p->size < 0)
    {
        yaz_log(YLOG_FATAL, "Bad block size %d in pos=" ZINT_FORMAT "\n",
                p->size, pos);
    }
    assert(p->size >= 0);
    src = (char*) p->buf + 3;
    decode_ptr(&src, &p->no_items);

    p->offset = 0;
    p->dirty = 0;
    p->deleted = 0;
    p->decodeClientData = (*b->method->codec.start)();
    return p;
}

struct ISAMB_block *new_block(ISAMB b, int leaf, int cat)
{
    struct ISAMB_block *p;

    p = xmalloc(sizeof(*p));
    p->buf = xmalloc(b->file[cat].head.block_size);

    if (!b->file[cat].head.free_list)
    {
        zint block_no;
        block_no = b->file[cat].head.last_block++;
        p->pos = block_no * CAT_MAX + cat;
        if (b->log_freelist)
            yaz_log(b->log_freelist, "got block "
                    ZINT_FORMAT " from last %d:" ZINT_FORMAT, p->pos,
                    cat, p->pos/CAT_MAX);
    }
    else
    {
        p->pos = b->file[cat].head.free_list;
        assert((p->pos & CAT_MASK) == cat);
        if (!cache_block(b, p->pos, p->buf, 0))
        {
            yaz_log(b->log_io, "bf_read: new_block");
            if (!bf_read(b->file[cat].bf, p->pos/CAT_MAX, 0, 0, p->buf))
            {
                yaz_log(YLOG_FATAL, "isamb: read fail for pos=%ld block=%ld",
                        (long) p->pos/CAT_MAX, (long) p->pos/CAT_MAX);
                zebra_exit("isamb:new_block");
            }
        }
        if (b->log_freelist)
            yaz_log(b->log_freelist, "got block "
                    ZINT_FORMAT " from freelist %d:" ZINT_FORMAT, p->pos,
                    cat, p->pos/CAT_MAX);
        memcpy(&b->file[cat].head.free_list, p->buf, sizeof(zint));
    }
    p->cat = cat;
    b->file[cat].head_dirty = 1;
    memset(p->buf, 0, b->file[cat].head.block_size);
    p->bytes = (char*)p->buf + b->file[cat].head.block_offset;
    p->leaf = leaf;
    p->size = 0;
    p->dirty = 1;
    p->deleted = 0;
    p->offset = 0;
    p->no_items = 0;
    p->decodeClientData = (*b->method->codec.start)();
    return p;
}

struct ISAMB_block *new_leaf(ISAMB b, int cat)
{
    return new_block(b, 1, cat);
}


struct ISAMB_block *new_int(ISAMB b, int cat)
{
    return new_block(b, 0, cat);
}

static void check_block(ISAMB b, struct ISAMB_block *p)
{
    assert(b); /* mostly to make the compiler shut up about unused b */
    if (p->leaf)
    {
        ;
    }
    else
    {
        /* sanity check */
        char *startp = p->bytes;
        const char *src = startp;
        char *endp = p->bytes + p->size;
        ISAM_P pos;
	void *c1 = (*b->method->codec.start)();

        decode_ptr(&src, &pos);
        assert((pos&CAT_MASK) == p->cat);
        while (src != endp)
        {
#if INT_ENCODE
	    char file_item_buf[DST_ITEM_MAX];
	    char *file_item = file_item_buf;
	    (*b->method->codec.reset)(c1);
	    (*b->method->codec.decode)(c1, &file_item, &src);
#else
            zint item_len;
            decode_item_len(&src, &item_len);
            assert(item_len > 0 && item_len < 128);
            src += item_len;
#endif
            decode_ptr(&src, &pos);
	    if ((pos&CAT_MASK) != p->cat)
	    {
		assert((pos&CAT_MASK) == p->cat);
	    }
        }
	(*b->method->codec.stop)(c1);
    }
}

void close_block(ISAMB b, struct ISAMB_block *p)
{
    if (!p)
        return;
    if (p->deleted)
    {
        yaz_log(b->log_freelist, "release block " ZINT_FORMAT " from freelist %d:" ZINT_FORMAT,
                p->pos, p->cat, p->pos/CAT_MAX);
        memcpy(p->buf, &b->file[p->cat].head.free_list, sizeof(zint));
        b->file[p->cat].head.free_list = p->pos;
        b->file[p->cat].head_dirty = 1;
        if (!cache_block(b, p->pos, p->buf, 1))
        {
            yaz_log(b->log_io, "bf_write: close_block (deleted)");
            bf_write(b->file[p->cat].bf, p->pos/CAT_MAX, 0, 0, p->buf);
        }
    }
    else if (p->dirty)
    {
	int offset = b->file[p->cat].head.block_offset;
        int size = p->size + offset;
	char *dst =  (char*)p->buf + 3;
        assert(p->size >= 0);

	/* memset becuase encode_ptr usually does not write all bytes */
	memset(p->buf, 0, b->file[p->cat].head.block_offset);
        p->buf[0] = p->leaf;
        p->buf[1] = size & 255;
        p->buf[2] = size >> 8;
	encode_ptr(&dst, p->no_items);
        check_block(b, p);
        if (!cache_block(b, p->pos, p->buf, 1))
        {
            yaz_log(b->log_io, "bf_write: close_block");
            bf_write(b->file[p->cat].bf, p->pos/CAT_MAX, 0, 0, p->buf);
        }
    }
    (*b->method->codec.stop)(p->decodeClientData);
    xfree(p->buf);
    xfree(p);
}

int insert_sub(ISAMB b, struct ISAMB_block **p,
               void *new_item, int *mode,
               ISAMC_I *stream,
               struct ISAMB_block **sp,
               void *sub_item, int *sub_size,
               const void *max_item);

int insert_int(ISAMB b, struct ISAMB_block *p, void *lookahead_item,
               int *mode,
               ISAMC_I *stream, struct ISAMB_block **sp,
               void *split_item, int *split_size, const void *last_max_item)
{
    char *startp = p->bytes;
    const char *src = startp;
    char *endp = p->bytes + p->size;
    ISAM_P pos;
    struct ISAMB_block *sub_p1 = 0, *sub_p2 = 0;
    char sub_item[DST_ITEM_MAX];
    int sub_size;
    int more = 0;
    zint diff_terms = 0;
    void *c1 = (*b->method->codec.start)();

    *sp = 0;

    assert(p->size >= 0);
    decode_ptr(&src, &pos);
    while (src != endp)
    {
        int d;
        const char *src0 = src;
#if INT_ENCODE
	char file_item_buf[DST_ITEM_MAX];
	char *file_item = file_item_buf;
	(*b->method->codec.reset)(c1);
        (*b->method->codec.decode)(c1, &file_item, &src);
	d = (*b->method->compare_item)(file_item_buf, lookahead_item);
        if (d > 0)
        {
            sub_p1 = open_block(b, pos);
            assert(sub_p1);
	    diff_terms -= sub_p1->no_items;
            more = insert_sub(b, &sub_p1, lookahead_item, mode,
                              stream, &sub_p2,
                              sub_item, &sub_size, file_item_buf);
	    diff_terms += sub_p1->no_items;
            src = src0;
            break;
        }
#else
        zint item_len;
        decode_item_len(&src, &item_len);
        d = (*b->method->compare_item)(src, lookahead_item);
        if (d > 0)
        {
            sub_p1 = open_block(b, pos);
            assert(sub_p1);
	    diff_terms -= sub_p1->no_items;
            more = insert_sub(b, &sub_p1, lookahead_item, mode,
                              stream, &sub_p2,
                              sub_item, &sub_size, src);
	    diff_terms += sub_p1->no_items;
            src = src0;
            break;
        }
        src += item_len;
#endif
        decode_ptr(&src, &pos);
    }
    if (!sub_p1)
    {
	/* we reached the end. So lookahead > last item */
        sub_p1 = open_block(b, pos);
        assert(sub_p1);
	diff_terms -= sub_p1->no_items;
        more = insert_sub(b, &sub_p1, lookahead_item, mode, stream, &sub_p2,
                          sub_item, &sub_size, last_max_item);
	diff_terms += sub_p1->no_items;
    }
    if (sub_p2)
	diff_terms += sub_p2->no_items;
    if (diff_terms)
    {
	p->dirty = 1;
	p->no_items += diff_terms;
    }
    if (sub_p2)
    {
        /* there was a split - must insert pointer in this one */
        char dst_buf[DST_BUF_SIZE];
        char *dst = dst_buf;
#if INT_ENCODE
	const char *sub_item_ptr = sub_item;
#endif
        assert(sub_size < DST_ITEM_MAX && sub_size > 1);

        memcpy(dst, startp, src - startp);

        dst += src - startp;

#if INT_ENCODE
	(*b->method->codec.reset)(c1);
        (*b->method->codec.encode)(c1, &dst, &sub_item_ptr);
#else
        encode_item_len(&dst, sub_size);      /* sub length and item */
        memcpy(dst, sub_item, sub_size);
        dst += sub_size;
#endif

        encode_ptr(&dst, sub_p2->pos);   /* pos */

        if (endp - src)                   /* remaining data */
        {
            memcpy(dst, src, endp - src);
            dst += endp - src;
        }
        p->size = dst - dst_buf;
        assert(p->size >= 0);

        if (p->size <= b->file[p->cat].head.block_max)
        {
	    /* it fits OK in this block */
            memcpy(startp, dst_buf, dst - dst_buf);

	    close_block(b, sub_p2);
        }
        else
        {
	    /* must split _this_ block as well .. */
	    struct ISAMB_block *sub_p3;
#if INT_ENCODE
	    char file_item_buf[DST_ITEM_MAX];
	    char *file_item = file_item_buf;
#else
	    zint split_size_tmp;
#endif
	    zint no_items_first_half = 0;
            int p_new_size;
            const char *half;
            src = dst_buf;
            endp = dst;

            b->number_of_int_splits++;

	    p->dirty = 1;
	    close_block(b, sub_p2);

            half = src + b->file[p->cat].head.block_size/2;
            decode_ptr(&src, &pos);

            if (b->enable_int_count)
            {
                /* read sub block so we can get no_items for it */
                sub_p3 = open_block(b, pos);
                no_items_first_half += sub_p3->no_items;
                close_block(b, sub_p3);
            }

            while (src <= half)
            {
#if INT_ENCODE
	        file_item = file_item_buf;
		(*b->method->codec.reset)(c1);
		(*b->method->codec.decode)(c1, &file_item, &src);
#else
                decode_item_len(&src, &split_size_tmp);
		*split_size = (int) split_size_tmp;
                src += *split_size;
#endif
                decode_ptr(&src, &pos);

                if (b->enable_int_count)
                {
                    /* read sub block so we can get no_items for it */
                    sub_p3 = open_block(b, pos);
                    no_items_first_half += sub_p3->no_items;
                    close_block(b, sub_p3);
                }
            }
	    /*  p is first half */
            p_new_size = src - dst_buf;
            memcpy(p->bytes, dst_buf, p_new_size);

#if INT_ENCODE
	    file_item = file_item_buf;
	    (*b->method->codec.reset)(c1);
	    (*b->method->codec.decode)(c1, &file_item, &src);
	    *split_size = file_item - file_item_buf;
	    memcpy(split_item, file_item_buf, *split_size);
#else
	    decode_item_len(&src, &split_size_tmp);
	    *split_size = (int) split_size_tmp;
            memcpy(split_item, src, *split_size);
            src += *split_size;
#endif
	    /*  *sp is second half */
            *sp = new_int(b, p->cat);
            (*sp)->size = endp - src;
            memcpy((*sp)->bytes, src, (*sp)->size);

            p->size = p_new_size;

	    /*  adjust no_items in first&second half */
	    (*sp)->no_items = p->no_items - no_items_first_half;
	    p->no_items = no_items_first_half;
        }
	p->dirty = 1;
    }
    close_block(b, sub_p1);
    (*b->method->codec.stop)(c1);
    return more;
}

int insert_leaf(ISAMB b, struct ISAMB_block **sp1, void *lookahead_item,
                int *lookahead_mode, ISAMC_I *stream,
                struct ISAMB_block **sp2,
                void *sub_item, int *sub_size,
                const void *max_item)
{
    struct ISAMB_block *p = *sp1;
    char *endp = 0;
    const char *src = 0;
    char dst_buf[DST_BUF_SIZE], *dst = dst_buf;
    int new_size;
    void *c1 = (*b->method->codec.start)();
    void *c2 = (*b->method->codec.start)();
    int more = 1;
    int quater = b->file[b->no_cat-1].head.block_max / 4;
    char *mid_cut = dst_buf + quater * 2;
    char *tail_cut = dst_buf + quater * 3;
    char *maxp = dst_buf + b->file[b->no_cat-1].head.block_max;
    char *half1 = 0;
    char *half2 = 0;
    char cut_item_buf[DST_ITEM_MAX];
    int cut_item_size = 0;
    int no_items = 0;    /* number of items (total) */
    int no_items_1 = 0;  /* number of items (first half) */
    int inserted_dst_bytes = 0;

    if (p && p->size)
    {
        char file_item_buf[DST_ITEM_MAX];
        char *file_item = file_item_buf;

        src = p->bytes;
        endp = p->bytes + p->size;
        (*b->method->codec.decode)(c1, &file_item, &src);
        while (1)
        {
            const char *dst_item = 0; /* resulting item to be inserted */
            char *lookahead_next;
	    char *dst_0 = dst;
            int d = -1;

            if (lookahead_item)
                d = (*b->method->compare_item)(file_item_buf, lookahead_item);

	    /* d now holds comparison between existing file item and
	       lookahead item
	       d = 0: equal
               d > 0: lookahead before file
	       d < 0: lookahead after file
	    */
            if (d > 0)
            {
		/* lookahead must be inserted */
                dst_item = lookahead_item;
		/* if this is not an insertion, it's really bad .. */
                if (!*lookahead_mode)
                {
                    yaz_log(YLOG_WARN, "isamb: Inconsistent register (1)");
                    assert(*lookahead_mode);
                }
            }
            else if (d == 0 && *lookahead_mode == 2)
            {
                /* For mode == 2, we insert the new key anyway - even
                   though the comparison is 0. */
                dst_item = lookahead_item;
                p->dirty = 1;
            }
            else
                dst_item = file_item_buf;

            if (d == 0 && !*lookahead_mode)
            {
                /* it's a deletion and they match so there is nothing
                   to be inserted anyway .. But mark the thing dirty
                   (file item was part of input.. The item will not be
                   part of output */
                p->dirty = 1;
            }
            else if (!half1 && dst > mid_cut)
            {
		/* we have reached the splitting point for the first time */
                const char *dst_item_0 = dst_item;
                half1 = dst; /* candidate for splitting */

		/* encode the resulting item */
                (*b->method->codec.encode)(c2, &dst, &dst_item);

                cut_item_size = dst_item - dst_item_0;
		assert(cut_item_size > 0);
                memcpy(cut_item_buf, dst_item_0, cut_item_size);

                half2 = dst;
		no_items_1 = no_items;
		no_items++;
            }
            else
	    {
		/* encode the resulting item */
                (*b->method->codec.encode)(c2, &dst, &dst_item);
		no_items++;
	    }

	    /* now move "pointers" .. result has been encoded .. */
            if (d > 0)
            {
		/* we must move the lookahead pointer */

		inserted_dst_bytes += (dst - dst_0);
                if (inserted_dst_bytes >=  quater)
		    /* no more room. Mark lookahead as "gone".. */
                    lookahead_item = 0;
                else
                {
		    /* move it really.. */
                    lookahead_next = lookahead_item;
                    if (!(*stream->read_item)(stream->clientData,
                                              &lookahead_next,
                                              lookahead_mode))
                    {
			/* end of stream reached: no "more" and no lookahead */
                        lookahead_item = 0;
                        more = 0;
                    }
                    if (lookahead_item && max_item &&
                        (*b->method->compare_item)(max_item, lookahead_item) <= 0)
                    {
			/* the lookahead goes beyond what we allow in this
			   leaf. Mark it as "gone" */
                        lookahead_item = 0;
                    }

                    p->dirty = 1;
                }
            }
            else if (d == 0)
            {
		/* exact match .. move both pointers */

                lookahead_next = lookahead_item;
                if (!(*stream->read_item)(stream->clientData,
                                          &lookahead_next, lookahead_mode))
                {
                    lookahead_item = 0;
                    more = 0;
                }
                if (src == endp)
                    break;  /* end of file stream reached .. */
                file_item = file_item_buf; /* move file pointer */
                (*b->method->codec.decode)(c1, &file_item, &src);
            }
            else
            {
		/* file pointer must be moved */
                if (src == endp)
                    break;
                file_item = file_item_buf;
                (*b->method->codec.decode)(c1, &file_item, &src);
            }
        }
    }

    /* this loop runs when we are "appending" to a leaf page. That is
       either it's empty (new) or all file items have been read in
       previous loop */

    maxp = dst_buf + b->file[b->no_cat-1].head.block_max + quater;
    while (lookahead_item)
    {
        char *dst_item;
	const char *src = lookahead_item;
        char *dst_0 = dst;

	/* if we have a lookahead item, we stop if we exceed the value of it */
        if (max_item &&
            (*b->method->compare_item)(max_item, lookahead_item) <= 0)
        {
            /* stop if we have reached the value of max item */
            break;
        }
        if (!*lookahead_mode)
        {
	    /* this is append. So a delete is bad */
            yaz_log(YLOG_WARN, "isamb: Inconsistent register (2)");
            assert(*lookahead_mode);
        }
        else if (!half1 && dst > tail_cut)
        {
            const char *src_0 = src;
            half1 = dst; /* candidate for splitting */

            (*b->method->codec.encode)(c2, &dst, &src);

            cut_item_size = src - src_0;
	    assert(cut_item_size > 0);
            memcpy(cut_item_buf, src_0, cut_item_size);

	    no_items_1 = no_items;
            half2 = dst;
        }
        else
            (*b->method->codec.encode)(c2, &dst, &src);

        if (dst > maxp)
        {
            dst = dst_0;
            break;
        }
	no_items++;
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
        p->deleted = 1;
        close_block(b, p);
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
        p = new_leaf(b, i);
    }
    if (new_size > b->file[p->cat].head.block_max)
    {
        char *first_dst;
        const char *cut_item = cut_item_buf;

        assert(half1);
        assert(half2);

	assert(cut_item_size > 0);

	/* first half */
        p->size = half1 - dst_buf;
	assert(p->size <=  b->file[p->cat].head.block_max);
        memcpy(p->bytes, dst_buf, half1 - dst_buf);
	p->no_items = no_items_1;

        /* second half */
        *sp2 = new_leaf(b, p->cat);

        (*b->method->codec.reset)(c2);

        b->number_of_leaf_splits++;

        first_dst = (*sp2)->bytes;

        (*b->method->codec.encode)(c2, &first_dst, &cut_item);

        memcpy(first_dst, half2, dst - half2);

        (*sp2)->size = (first_dst - (*sp2)->bytes) + (dst - half2);
	assert((*sp2)->size <=  b->file[p->cat].head.block_max);
	(*sp2)->no_items = no_items - no_items_1;
        (*sp2)->dirty = 1;
        p->dirty = 1;
        memcpy(sub_item, cut_item_buf, cut_item_size);
        *sub_size = cut_item_size;
    }
    else
    {
        memcpy(p->bytes, dst_buf, dst - dst_buf);
        p->size = new_size;
	p->no_items = no_items;
    }
    (*b->method->codec.stop)(c1);
    (*b->method->codec.stop)(c2);
    *sp1 = p;
    return more;
}

int insert_sub(ISAMB b, struct ISAMB_block **p, void *new_item,
               int *mode,
               ISAMC_I *stream,
               struct ISAMB_block **sp,
               void *sub_item, int *sub_size,
               const void *max_item)
{
    if (!*p || (*p)->leaf)
        return insert_leaf(b, p, new_item, mode, stream, sp, sub_item,
                           sub_size, max_item);
    else
        return insert_int(b, *p, new_item, mode, stream, sp, sub_item,
                          sub_size, max_item);
}

int isamb_unlink(ISAMB b, ISAM_P pos)
{
    struct ISAMB_block *p1;

    if (!pos)
	return 0;
    p1 = open_block(b, pos);
    p1->deleted = 1;
    if (!p1->leaf)
    {
	zint sub_p;
	const char *src = p1->bytes + p1->offset;
#if INT_ENCODE
	void *c1 = (*b->method->codec.start)();
#endif
	decode_ptr(&src, &sub_p);
	isamb_unlink(b, sub_p);

	while (src != p1->bytes + p1->size)
	{
#if INT_ENCODE
	    char file_item_buf[DST_ITEM_MAX];
	    char *file_item = file_item_buf;
	    (*b->method->codec.reset)(c1);
	    (*b->method->codec.decode)(c1, &file_item, &src);
#else
	    zint item_len;
	    decode_item_len(&src, &item_len);
	    src += item_len;
#endif
	    decode_ptr(&src, &sub_p);
	    isamb_unlink(b, sub_p);
	}
#if INT_ENCODE
	(*b->method->codec.stop)(c1);
#endif
    }
    close_block(b, p1);
    return 0;
}

void isamb_merge(ISAMB b, ISAM_P *pos, ISAMC_I *stream)
{
    char item_buf[DST_ITEM_MAX];
    char *item_ptr;
    int i_mode;
    int more;
    int must_delete = 0;

    if (b->cache < 0)
    {
        int more = 1;
        while (more)
        {
            item_ptr = item_buf;
            more =
                (*stream->read_item)(stream->clientData, &item_ptr, &i_mode);
        }
	*pos = 1;
        return;
    }
    item_ptr = item_buf;
    more = (*stream->read_item)(stream->clientData, &item_ptr, &i_mode);
    while (more)
    {
        struct ISAMB_block *p = 0, *sp = 0;
        char sub_item[DST_ITEM_MAX];
        int sub_size;

        if (*pos)
            p = open_block(b, *pos);
        more = insert_sub(b, &p, item_buf, &i_mode, stream, &sp,
                          sub_item, &sub_size, 0);
        if (sp)
        {    /* increase level of tree by one */
            struct ISAMB_block *p2 = new_int(b, p->cat);
            char *dst = p2->bytes + p2->size;
#if INT_ENCODE
	    void *c1 = (*b->method->codec.start)();
	    const char *sub_item_ptr = sub_item;
#endif

            encode_ptr(&dst, p->pos);
	    assert(sub_size < DST_ITEM_MAX && sub_size > 1);
#if INT_ENCODE
	    (*b->method->codec.reset)(c1);
	    (*b->method->codec.encode)(c1, &dst, &sub_item_ptr);
#else
            encode_item_len(&dst, sub_size);
            memcpy(dst, sub_item, sub_size);
            dst += sub_size;
#endif
            encode_ptr(&dst, sp->pos);

            p2->size = dst - p2->bytes;
	    p2->no_items = p->no_items + sp->no_items;
            *pos = p2->pos;  /* return new super page */
            close_block(b, sp);
            close_block(b, p2);
#if INT_ENCODE
	    (*b->method->codec.stop)(c1);
#endif
        }
        else
	{
            *pos = p->pos;   /* return current one (again) */
	}
	if (p->no_items == 0)
	    must_delete = 1;
	else
	    must_delete = 0;
        close_block(b, p);
    }
    if (must_delete)
    {
	isamb_unlink(b, *pos);
	*pos = 0;
    }
}

ISAMB_PP isamb_pp_open_x(ISAMB isamb, ISAM_P pos, int *level, int scope)
{
    ISAMB_PP pp = xmalloc(sizeof(*pp));
    int i;

    assert(pos);

    pp->isamb = isamb;
    pp->block = xmalloc(ISAMB_MAX_LEVEL * sizeof(*pp->block));

    pp->pos = pos;
    pp->level = 0;
    pp->maxlevel = 0;
    pp->total_size = 0;
    pp->no_blocks = 0;
    pp->skipped_numbers = 0;
    pp->returned_numbers = 0;
    pp->scope = scope;
    for (i = 0; i<ISAMB_MAX_LEVEL; i++)
        pp->skipped_nodes[i] = pp->accessed_nodes[i] = 0;
    while (1)
    {
        struct ISAMB_block *p = open_block(isamb, pos);
        const char *src = p->bytes + p->offset;
        pp->block[pp->level] = p;

        pp->total_size += p->size;
        pp->no_blocks++;
        if (p->leaf)
            break;
        decode_ptr(&src, &pos);
        p->offset = src - p->bytes;
        pp->level++;
        pp->accessed_nodes[pp->level]++;
    }
    pp->block[pp->level+1] = 0;
    pp->maxlevel = pp->level;
    if (level)
        *level = pp->level;
    return pp;
}

ISAMB_PP isamb_pp_open(ISAMB isamb, ISAM_P pos, int scope)
{
    return isamb_pp_open_x(isamb, pos, 0, scope);
}

void isamb_pp_close_x(ISAMB_PP pp, zint *size, zint *blocks)
{
    int i;
    if (!pp)
        return;
    yaz_log(YLOG_DEBUG, "isamb_pp_close lev=%d returned "ZINT_FORMAT" values, "
            "skipped "ZINT_FORMAT,
            pp->maxlevel, pp->skipped_numbers, pp->returned_numbers);
    for (i = pp->maxlevel; i>=0; i--)
        if (pp->skipped_nodes[i] || pp->accessed_nodes[i])
            yaz_log(YLOG_DEBUG, "isamb_pp_close  level leaf-%d: "
                    ZINT_FORMAT" read, "ZINT_FORMAT" skipped", i,
                    pp->accessed_nodes[i], pp->skipped_nodes[i]);
    pp->isamb->skipped_numbers += pp->skipped_numbers;
    pp->isamb->returned_numbers += pp->returned_numbers;
    for (i = pp->maxlevel; i>=0; i--)
    {
        pp->isamb->accessed_nodes[i] += pp->accessed_nodes[i];
        pp->isamb->skipped_nodes[i] += pp->skipped_nodes[i];
    }
    if (size)
        *size = pp->total_size;
    if (blocks)
        *blocks = pp->no_blocks;
    for (i = 0; i <= pp->level; i++)
        close_block(pp->isamb, pp->block[i]);
    xfree(pp->block);
    xfree(pp);
}

int isamb_block_info(ISAMB isamb, int cat)
{
    if (cat >= 0 && cat < isamb->no_cat)
        return isamb->file[cat].head.block_size;
    return -1;
}

void isamb_pp_close(ISAMB_PP pp)
{
    isamb_pp_close_x(pp, 0, 0);
}

/* simple recursive dumper .. */
static void isamb_dump_r(ISAMB b, ISAM_P pos, void (*pr)(const char *str),
                         int level)
{
    char buf[1024];
    char prefix_str[1024];
    if (pos)
    {
	struct ISAMB_block *p = open_block(b, pos);
	sprintf(prefix_str, "%*s " ZINT_FORMAT " cat=%d size=%d max=%d items="
		ZINT_FORMAT, level*2, "",
		pos, p->cat, p->size, b->file[p->cat].head.block_max,
		p->no_items);
	(*pr)(prefix_str);
	sprintf(prefix_str, "%*s " ZINT_FORMAT, level*2, "", pos);
	if (p->leaf)
	{
	    while (p->offset < p->size)
	    {
		const char *src = p->bytes + p->offset;
		char *dst = buf;
		(*b->method->codec.decode)(p->decodeClientData, &dst, &src);
		(*b->method->log_item)(YLOG_DEBUG, buf, prefix_str);
		p->offset = src - (char*) p->bytes;
	    }
	    assert(p->offset == p->size);
	}
	else
	{
	    const char *src = p->bytes + p->offset;
	    ISAM_P sub;

	    decode_ptr(&src, &sub);
	    p->offset = src - (char*) p->bytes;

	    isamb_dump_r(b, sub, pr, level+1);

	    while (p->offset < p->size)
	    {
#if INT_ENCODE
		char file_item_buf[DST_ITEM_MAX];
		char *file_item = file_item_buf;
		void *c1 = (*b->method->codec.start)();
		(*b->method->codec.decode)(c1, &file_item, &src);
		(*b->method->codec.stop)(c1);
		(*b->method->log_item)(YLOG_DEBUG, file_item_buf, prefix_str);
#else
		zint item_len;
		decode_item_len(&src, &item_len);
		(*b->method->log_item)(YLOG_DEBUG, src, prefix_str);
		src += item_len;
#endif
		decode_ptr(&src, &sub);

		p->offset = src - (char*) p->bytes;

		isamb_dump_r(b, sub, pr, level+1);
	    }
	}
	close_block(b, p);
    }
}

void isamb_dump(ISAMB b, ISAM_P pos, void (*pr)(const char *str))
{
    isamb_dump_r(b, pos, pr, 0);
}

int isamb_pp_read(ISAMB_PP pp, void *buf)
{
    return isamb_pp_forward(pp, buf, 0);
}


void isamb_pp_pos(ISAMB_PP pp, double *current, double *total)
{ /* return an estimate of the current position and of the total number of */
  /* occureences in the isam tree, based on the current leaf */
    assert(total);
    assert(current);

    /* if end-of-stream PP may not be leaf */

    *total = (double) (pp->block[0]->no_items);
    *current = (double) pp->returned_numbers;
#if ISAMB_DEBUG
    yaz_log(YLOG_LOG, "isamb_pp_pos returning: cur= %0.1f tot=%0.1f rn="
            ZINT_FORMAT, *current, *total, pp->returned_numbers);
#endif
}

int isamb_pp_forward(ISAMB_PP pp, void *buf, const void *untilb)
{
    char *dst = buf;
    const char *src;
    struct ISAMB_block *p = pp->block[pp->level];
    ISAMB b = pp->isamb;
    if (!p)
        return 0;
 again:
    while (p->offset == p->size)
    {
        ISAM_P pos;
#if INT_ENCODE
	const char *src_0;
	void *c1;
	char file_item_buf[DST_ITEM_MAX];
	char *file_item = file_item_buf;
#else
	zint item_len;
#endif
        while (p->offset == p->size)
        {
            if (pp->level == 0)
                return 0;
            close_block(pp->isamb, pp->block[pp->level]);
            pp->block[pp->level] = 0;
            (pp->level)--;
            p = pp->block[pp->level];
            assert(!p->leaf);
        }

	assert(!p->leaf);
        src = p->bytes + p->offset;

#if INT_ENCODE
	c1 = (*b->method->codec.start)();
	(*b->method->codec.decode)(c1, &file_item, &src);
#else
        decode_ptr(&src, &item_len);
        src += item_len;
#endif
        decode_ptr(&src, &pos);
        p->offset = src - (char*) p->bytes;

	src = p->bytes + p->offset;

	while(1)
	{
	    if (!untilb || p->offset == p->size)
		break;
	    assert(p->offset < p->size);
#if INT_ENCODE
	    src_0 = src;
	    file_item = file_item_buf;
	    (*b->method->codec.reset)(c1);
	    (*b->method->codec.decode)(c1, &file_item, &src);
	    if ((*b->method->compare_item)(untilb, file_item_buf) < pp->scope)
	    {
		src = src_0;
		break;
	    }
#else
	    decode_item_len(&src, &item_len);
	    if ((*b->method->compare_item)(untilb, src) < pp->scope)
		break;
	    src += item_len;
#endif
	    decode_ptr(&src, &pos);
	    p->offset = src - (char*) p->bytes;
	}

	pp->level++;

        while (1)
        {
            pp->block[pp->level] = p = open_block(pp->isamb, pos);

            pp->total_size += p->size;
            pp->no_blocks++;

            if (p->leaf)
            {
                break;
            }

            src = p->bytes + p->offset;
	    while(1)
	    {
		decode_ptr(&src, &pos);
		p->offset = src - (char*) p->bytes;

		if (!untilb || p->offset == p->size)
		    break;
		assert(p->offset < p->size);
#if INT_ENCODE
		src_0 = src;
		file_item = file_item_buf;
		(*b->method->codec.reset)(c1);
		(*b->method->codec.decode)(c1, &file_item, &src);
		if ((*b->method->compare_item)(untilb, file_item_buf) < pp->scope)
		{
		    src = src_0;
		    break;
		}
#else
		decode_ptr(&src, &item_len);
		if ((*b->method->compare_item)(untilb, src) <= pp->scope)
		    break;
		src += item_len;
#endif
	    }
            pp->level++;
        }
#if INT_ENCODE
	(*b->method->codec.stop)(c1);
#endif
    }
    assert(p->offset < p->size);
    assert(p->leaf);
    while(1)
    {
	char *dst0 = dst;
        src = p->bytes + p->offset;
        (*pp->isamb->method->codec.decode)(p->decodeClientData, &dst, &src);
        p->offset = src - (char*) p->bytes;
        if (!untilb || (*pp->isamb->method->compare_item)(untilb, dst0) < pp->scope)
	    break;
	dst = dst0;
	if (p->offset == p->size) goto again;
    }
    pp->returned_numbers++;
    return 1;
}

zint isamb_get_int_splits(ISAMB b)
{
    return b->number_of_int_splits;
}

zint isamb_get_leaf_splits(ISAMB b)
{
    return b->number_of_leaf_splits;
}

zint isamb_get_root_ptr(ISAMB b)
{
    return b->root_ptr;
}

void isamb_set_root_ptr(ISAMB b, zint root_ptr)
{
    b->root_ptr = root_ptr;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

