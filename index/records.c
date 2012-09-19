/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

/*
 *  Format of first block (assumes a 512 block size)
 *      next       (8 bytes)
 *      ref_count  (2 bytes)
 *      block      (500 bytes)
 *
 *  Format of subsequent blocks
 *      next  (8 bytes)
 *      block (502 bytes)
 *
 *  Format of each record
 *      sysno
 *      (length, data) - pairs
 *      length = 0 if same as previous
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <yaz/yaz-util.h>
#include <idzebra/bfile.h>
#include "recindex.h"

#if HAVE_BZLIB_H
#include <bzlib.h>
#endif
#if HAVE_ZLIB_H
#include <zlib.h>
#endif

#define REC_BLOCK_TYPES 2
#define REC_HEAD_MAGIC "recindex"
#define REC_VERSION 5

struct records_info {
    int rw;
    int compression_method;

    recindex_t recindex;

    char *data_fname[REC_BLOCK_TYPES];
    BFile data_BFile[REC_BLOCK_TYPES];

    char *tmp_buf;
    int tmp_size;

    struct record_cache_entry *record_cache;
    int cache_size;
    int cache_cur;
    int cache_max;

    int compression_chunk_size;

    Zebra_mutex mutex;

    struct records_head {
        char magic[8];
	char version[4];
        zint block_size[REC_BLOCK_TYPES];
        zint block_free[REC_BLOCK_TYPES];
        zint block_last[REC_BLOCK_TYPES];
        zint block_used[REC_BLOCK_TYPES];
        zint block_move[REC_BLOCK_TYPES];

        zint total_bytes;
        zint index_last;
        zint index_free;
        zint no_records;

    } head;
};

enum recordCacheFlag { recordFlagNop, recordFlagWrite, recordFlagNew,
                       recordFlagDelete };

struct record_cache_entry {
    Record rec;
    enum recordCacheFlag flag;
};

struct record_index_entry {
    zint next;         /* first block of record info / next free entry */
    int size;          /* size of record or 0 if free entry */
};

Record rec_cp(Record rec);

/* Modify argument to if below: 1=normal, 0=sysno testing */
#if 1
/* If this is used sysno are not converted (no testing) */
#define FAKE_OFFSET 0
#define USUAL_RANGE 6000000000LL

#else
/* Use a fake > 2^32 offset so we can test for proper 64-bit handling */
#define FAKE_OFFSET 6000000000LL
#define USUAL_RANGE 2000000000LL
#endif

static zint rec_sysno_to_ext(zint sysno)
{
    assert(sysno >= 0 && sysno <= USUAL_RANGE);
    return sysno + FAKE_OFFSET;
}

zint rec_sysno_to_int(zint sysno)
{
    assert(sysno >= FAKE_OFFSET && sysno <= FAKE_OFFSET + USUAL_RANGE);
    return sysno - FAKE_OFFSET;
}

static void rec_tmp_expand(Records p, int size)
{
    if (p->tmp_size < size + 2048 ||
        p->tmp_size < p->head.block_size[REC_BLOCK_TYPES-1]*2)
    {
        xfree(p->tmp_buf);
        p->tmp_size = size + (int)
	        	(p->head.block_size[REC_BLOCK_TYPES-1])*2 + 2048;
        p->tmp_buf = (char *) xmalloc(p->tmp_size);
    }
}

static ZEBRA_RES rec_release_blocks(Records p, zint sysno)
{
    struct record_index_entry entry;
    zint freeblock;
    char block_and_ref[sizeof(zint) + sizeof(short)];
    int dst_type;
    int first = 1;

    if (recindex_read_indx(p->recindex, sysno, &entry, sizeof(entry), 1) != 1)
        return ZEBRA_FAIL;

    freeblock = entry.next;
    assert(freeblock > 0);
    dst_type = CAST_ZINT_TO_INT(freeblock & 7);
    assert(dst_type < REC_BLOCK_TYPES);
    freeblock = freeblock / 8;
    while (freeblock)
    {
        if (bf_read(p->data_BFile[dst_type], freeblock, 0,
		     first ? sizeof(block_and_ref) : sizeof(zint),
		     block_and_ref) != 1)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "read in rec_del_single");
	    return ZEBRA_FAIL;
        }
	if (first)
	{
	    short ref;
	    memcpy(&ref, block_and_ref + sizeof(freeblock), sizeof(ref));
	    --ref;
	    memcpy(block_and_ref + sizeof(freeblock), &ref, sizeof(ref));
	    if (ref)
	    {
                /* there is still a reference to this block.. */
		if (bf_write(p->data_BFile[dst_type], freeblock, 0,
			      sizeof(block_and_ref), block_and_ref))
		{
		    yaz_log(YLOG_FATAL|YLOG_ERRNO, "write in rec_del_single");
		    return ZEBRA_FAIL;
		}
		return ZEBRA_OK;
	    }
            /* the list of blocks can all be removed (ref == 0) */
            first = 0;
	}

        if (bf_write(p->data_BFile[dst_type], freeblock, 0, sizeof(freeblock),
                      &p->head.block_free[dst_type]))
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "write in rec_del_single");
	    return ZEBRA_FAIL;
        }
        p->head.block_free[dst_type] = freeblock;
        memcpy(&freeblock, block_and_ref, sizeof(freeblock));

        p->head.block_used[dst_type]--;
    }
    p->head.total_bytes -= entry.size;
    return ZEBRA_OK;
}

static ZEBRA_RES rec_delete_single(Records p, Record rec)
{
    struct record_index_entry entry;

    /* all data in entry must be reset, since it's written verbatim */
    memset(&entry, '\0', sizeof(entry));
    if (rec_release_blocks(p, rec_sysno_to_int(rec->sysno)) != ZEBRA_OK)
	return ZEBRA_FAIL;

    entry.next = p->head.index_free;
    entry.size = 0;
    p->head.index_free = rec_sysno_to_int(rec->sysno);
    recindex_write_indx(p->recindex, rec_sysno_to_int(rec->sysno), &entry, sizeof(entry));
    return ZEBRA_OK;
}

static ZEBRA_RES rec_write_tmp_buf(Records p, int size, zint *sysnos)
{
    struct record_index_entry entry;
    int no_written = 0;
    char *cptr = p->tmp_buf;
    zint block_prev = -1, block_free;
    int dst_type = 0;
    int i;

    /* all data in entry must be reset, since it's written verbatim */
    memset(&entry, '\0', sizeof(entry));

    for (i = 1; i<REC_BLOCK_TYPES; i++)
        if (size >= p->head.block_move[i])
            dst_type = i;
    while (no_written < size)
    {
        block_free = p->head.block_free[dst_type];
        if (block_free)
        {
            if (bf_read(p->data_BFile[dst_type],
                         block_free, 0, sizeof(*p->head.block_free),
                         &p->head.block_free[dst_type]) != 1)
            {
                yaz_log(YLOG_FATAL|YLOG_ERRNO, "read in %s at free block "
			 ZINT_FORMAT,
			 p->data_fname[dst_type], block_free);
		return ZEBRA_FAIL;
            }
        }
        else
            block_free = p->head.block_last[dst_type]++;
        if (block_prev == -1)
        {
            entry.next = block_free*8 + dst_type;
            entry.size = size;
            p->head.total_bytes += size;
	    while (*sysnos > 0)
	    {
		recindex_write_indx(p->recindex, *sysnos, &entry, sizeof(entry));
		sysnos++;
	    }
        }
        else
        {
            memcpy(cptr, &block_free, sizeof(block_free));
            bf_write(p->data_BFile[dst_type], block_prev, 0, 0, cptr);
            cptr = p->tmp_buf + no_written;
        }
        block_prev = block_free;
        no_written += CAST_ZINT_TO_INT(p->head.block_size[dst_type])
            - sizeof(zint);
        p->head.block_used[dst_type]++;
    }
    assert(block_prev != -1);
    block_free = 0;
    memcpy(cptr, &block_free, sizeof(block_free));
    bf_write(p->data_BFile[dst_type], block_prev, 0,
              sizeof(block_free) + (p->tmp_buf+size) - cptr, cptr);
    return ZEBRA_OK;
}

int rec_check_compression_method(int compression_method)
{
    switch(compression_method)
    {
    case REC_COMPRESS_ZLIB:
#if HAVE_ZLIB_H
        return 1;
#else
        return 0;
#endif
    case REC_COMPRESS_BZIP2:
#if HAVE_BZLIB_H
        return 1;
#else
        return 0;
#endif
    case REC_COMPRESS_NONE:
        return 1;
    }
    return 0;
}

Records rec_open(BFiles bfs, int rw, int compression_method)
{
    Records p;
    int i, r;
    int version;
    ZEBRA_RES ret = ZEBRA_OK;

    p = (Records) xmalloc(sizeof(*p));
    memset(&p->head, '\0', sizeof(p->head));
    p->compression_method = compression_method;
    p->rw = rw;
    p->tmp_size = 4096;
    p->tmp_buf = (char *) xmalloc(p->tmp_size);
    p->compression_chunk_size = 0;
    if (compression_method == REC_COMPRESS_BZIP2)
        p->compression_chunk_size = 90000;
    p->recindex = recindex_open(bfs, rw, 0 /* 1=isamb for recindex */);
    r = recindex_read_head(p->recindex, p->tmp_buf);
    switch (r)
    {
    case 0:
        memcpy(p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic));
	sprintf(p->head.version, "%3d", REC_VERSION);
        p->head.index_free = 0;
        p->head.index_last = 1;
        p->head.no_records = 0;
        p->head.total_bytes = 0;
        for (i = 0; i<REC_BLOCK_TYPES; i++)
        {
            p->head.block_free[i] = 0;
            p->head.block_last[i] = 1;
            p->head.block_used[i] = 0;
        }
        p->head.block_size[0] = 256;
        p->head.block_move[0] = 0;
        for (i = 1; i<REC_BLOCK_TYPES; i++)
        {
            p->head.block_size[i] = p->head.block_size[i-1] * 8;
            p->head.block_move[i] = p->head.block_size[i] * 2;
        }
        if (rw)
	{
            if (recindex_write_head(p->recindex,
                                    &p->head, sizeof(p->head)) != ZEBRA_OK)
		ret = ZEBRA_FAIL;
	}
        break;
    case 1:
        memcpy(&p->head, p->tmp_buf, sizeof(p->head));
        if (memcmp(p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic)))
        {
            yaz_log(YLOG_FATAL, "file %s has bad format",
                    recindex_get_fname(p->recindex));
	    ret = ZEBRA_FAIL;
        }
	version = atoi(p->head.version);
	if (version != REC_VERSION)
	{
	    yaz_log(YLOG_FATAL, "file %s is version %d, but version"
		  " %d is required",
                    recindex_get_fname(p->recindex), version, REC_VERSION);
	    ret = ZEBRA_FAIL;
	}
        break;
    }
    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        char str[80];
        sprintf(str, "recd%c", i + 'A');
        p->data_fname[i] = (char *) xmalloc(strlen(str)+1);
        strcpy(p->data_fname[i], str);
        p->data_BFile[i] = NULL;
    }
    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (!(p->data_BFile[i] =
              bf_open(bfs, p->data_fname[i],
                      CAST_ZINT_TO_INT(p->head.block_size[i]), rw)))
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "bf_open %s", p->data_fname[i]);
	    ret = ZEBRA_FAIL;
            break;
        }
    }
    p->cache_max = 400;
    p->cache_cur = 0;
    p->record_cache = (struct record_cache_entry *)
	xmalloc(sizeof(*p->record_cache)*p->cache_max);
    zebra_mutex_init(&p->mutex);
    if (ret == ZEBRA_FAIL)
	rec_close(&p);
    return p;
}

static void rec_encode_unsigned(unsigned n, unsigned char *buf, int *len)
{
    (*len) = 0;
    while (n > 127)
    {
	buf[*len] = 128 + (n & 127);
	n = n >> 7;
	(*len)++;
    }
    buf[*len] = n;
    (*len)++;
}

static void rec_decode_unsigned(unsigned *np, unsigned char *buf, int *len)
{
    unsigned n = 0;
    unsigned w = 1;
    (*len) = 0;

    while (buf[*len] > 127)
    {
	n += w*(buf[*len] & 127);
	w = w << 7;
	(*len)++;
    }
    n += w * buf[*len];
    (*len)++;
    *np = n;
}

static void rec_encode_zint(zint n, unsigned char *buf, int *len)
{
    (*len) = 0;
    while (n > 127)
    {
	buf[*len] = (unsigned) (128 + (n & 127));
	n = n >> 7;
	(*len)++;
    }
    buf[*len] = (unsigned) n;
    (*len)++;
}

static void rec_decode_zint(zint *np, unsigned char *buf, int *len)
{
    zint  n = 0;
    zint w = 1;
    (*len) = 0;

    while (buf[*len] > 127)
    {
	n += w*(buf[*len] & 127);
	w = w << 7;
	(*len)++;
    }
    n += w * buf[*len];
    (*len)++;
    *np = n;
}

static void rec_cache_flush_block1(Records p, Record rec, Record last_rec,
				   char **out_buf, int *out_size,
				   int *out_offset)
{
    int i;
    int len;

    for (i = 0; i<REC_NO_INFO; i++)
    {
	if (*out_offset + CAST_ZINT_TO_INT(rec->size[i]) + 20 > *out_size)
	{
	    int new_size = *out_offset + rec->size[i] + 65536;
	    char *np = (char *) xmalloc(new_size);
	    if (*out_offset)
		memcpy(np, *out_buf, *out_offset);
	    xfree(*out_buf);
	    *out_size = new_size;
	    *out_buf = np;
	}
	if (i == 0)
	{
	    rec_encode_zint(rec_sysno_to_int(rec->sysno),
			    (unsigned char *) *out_buf + *out_offset, &len);
	    (*out_offset) += len;
	}
	if (rec->size[i] == 0)
	{
	    rec_encode_unsigned(1, (unsigned char *) *out_buf + *out_offset,
				&len);
	    (*out_offset) += len;
	}
	else if (last_rec && rec->size[i] == last_rec->size[i] &&
		 !memcmp(rec->info[i], last_rec->info[i], rec->size[i]))
	{
	    rec_encode_unsigned(0, (unsigned char *) *out_buf + *out_offset,
				&len);
	    (*out_offset) += len;
	}
	else
	{
	    rec_encode_unsigned(rec->size[i]+1,
				(unsigned char *) *out_buf + *out_offset,
				&len);
	    (*out_offset) += len;
	    memcpy(*out_buf + *out_offset, rec->info[i], rec->size[i]);
	    (*out_offset) += rec->size[i];
	}
    }
}

static ZEBRA_RES rec_flush_shared(Records p, short ref_count, zint *sysnos,
                                  char *out_buf, int out_offset)
{
    ZEBRA_RES ret = ZEBRA_OK;
    if (ref_count)
    {
        int i;
	unsigned int csize = 0;  /* indicate compression "not performed yet" */
	char compression_method = p->compression_method;
	switch (compression_method)
	{
        case REC_COMPRESS_ZLIB:
#if HAVE_ZLIB_H
	    csize = out_offset + (out_offset >> 6) + 620;
            while (1)
            {
                int r;
                uLongf destLen = csize;
                rec_tmp_expand(p, csize);
                r = compress((Bytef *) p->tmp_buf+sizeof(zint)+sizeof(short)+
                             sizeof(char),
                             &destLen, (const Bytef *) out_buf, out_offset);
                csize = destLen;
                if (r == Z_OK)
                {
                    break;
                }
                if (r != Z_MEM_ERROR)
                {
                    yaz_log(YLOG_WARN, "compress error: %d", r);
                    csize = 0;
                    break;
                }
                csize = csize * 2;
            }
#endif
            break;
	case REC_COMPRESS_BZIP2:
#if HAVE_BZLIB_H
	    csize = out_offset + (out_offset >> 6) + 620;
	    rec_tmp_expand(p, csize);
#ifdef BZ_CONFIG_ERROR
	    i = BZ2_bzBuffToBuffCompress
#else
	    i = bzBuffToBuffCompress
#endif
			 	    (p->tmp_buf+sizeof(zint)+sizeof(short)+
				      sizeof(char),
				      &csize, out_buf, out_offset, 1, 0, 30);
	    if (i != BZ_OK)
	    {
		yaz_log(YLOG_WARN, "bzBuffToBuffCompress error code=%d", i);
		csize = 0;
	    }
#endif
	    break;
	case REC_COMPRESS_NONE:
	    break;
	}
	if (!csize)
	{
	    /* either no compression or compression not supported ... */
	    csize = out_offset;
	    rec_tmp_expand(p, csize);
	    memcpy(p->tmp_buf + sizeof(zint) + sizeof(short) + sizeof(char),
		    out_buf, out_offset);
	    csize = out_offset;
	    compression_method = REC_COMPRESS_NONE;
	}
	memcpy(p->tmp_buf + sizeof(zint), &ref_count, sizeof(ref_count));
	memcpy(p->tmp_buf + sizeof(zint)+sizeof(short),
		&compression_method, sizeof(compression_method));

	/* -------- compression */
	if (rec_write_tmp_buf(p, csize + sizeof(short) + sizeof(char), sysnos)
	    != ZEBRA_OK)
	    ret = ZEBRA_FAIL;
    }
    return ret;
}

static ZEBRA_RES rec_write_multiple(Records p, int saveCount)
{
    int i;
    short ref_count = 0;
    Record last_rec = 0;
    int out_size = 1000;
    int out_offset = 0;
    char *out_buf = (char *) xmalloc(out_size);
    zint *sysnos = (zint *) xmalloc(sizeof(*sysnos) * (p->cache_cur + 1));
    zint *sysnop = sysnos;
    ZEBRA_RES ret = ZEBRA_OK;

    for (i = 0; i<p->cache_cur - saveCount; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        switch (e->flag)
        {
        case recordFlagNew:
            rec_cache_flush_block1(p, e->rec, last_rec, &out_buf,
				    &out_size, &out_offset);
	    *sysnop++ = rec_sysno_to_int(e->rec->sysno);
	    ref_count++;
	    e->flag = recordFlagNop;
	    last_rec = e->rec;
            break;
        case recordFlagWrite:
	    if (rec_release_blocks(p, rec_sysno_to_int(e->rec->sysno))
		!= ZEBRA_OK)
		ret = ZEBRA_FAIL;

            rec_cache_flush_block1(p, e->rec, last_rec, &out_buf,
				    &out_size, &out_offset);
	    *sysnop++ = rec_sysno_to_int(e->rec->sysno);
	    ref_count++;
	    e->flag = recordFlagNop;
	    last_rec = e->rec;
            break;
        case recordFlagDelete:
            if (rec_delete_single(p, e->rec) != ZEBRA_OK)
		ret = ZEBRA_FAIL;

	    e->flag = recordFlagNop;
            break;
        case recordFlagNop:
	    break;
	default:
            break;
        }
    }

    *sysnop = -1;
    rec_flush_shared(p, ref_count, sysnos, out_buf, out_offset);
    xfree(out_buf);
    xfree(sysnos);
    return ret;
}

static ZEBRA_RES rec_cache_flush(Records p, int saveCount)
{
    int i, j;
    ZEBRA_RES ret;

    if (saveCount >= p->cache_cur)
        saveCount = 0;

    ret = rec_write_multiple(p, saveCount);

    for (i = 0; i<p->cache_cur - saveCount; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        rec_free(&e->rec);
    }
    /* i still being used ... */
    for (j = 0; j<saveCount; j++, i++)
        memcpy(p->record_cache+j, p->record_cache+i,
                sizeof(*p->record_cache));
    p->cache_cur = saveCount;
    return ret;
}

static Record *rec_cache_lookup(Records p, zint sysno,
				enum recordCacheFlag flag)
{
    int i;
    for (i = 0; i<p->cache_cur; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        if (e->rec->sysno == sysno)
        {
            if (flag != recordFlagNop && e->flag == recordFlagNop)
                e->flag = flag;
            return &e->rec;
        }
    }
    return NULL;
}

static ZEBRA_RES rec_cache_insert(Records p, Record rec, enum recordCacheFlag flag)
{
    struct record_cache_entry *e;
    ZEBRA_RES ret = ZEBRA_OK;

    if (p->cache_cur == p->cache_max)
        ret = rec_cache_flush(p, 1);
    else if (p->cache_cur > 0)
    {
        int i, j;
        int used = 0;
        for (i = 0; i<p->cache_cur; i++)
        {
            Record r = (p->record_cache + i)->rec;
            for (j = 0; j<REC_NO_INFO; j++)
                used += r->size[j];
        }
        if (used > p->compression_chunk_size)
            ret = rec_cache_flush(p, 1);
    }
    assert(p->cache_cur < p->cache_max);

    e = p->record_cache + (p->cache_cur)++;
    e->flag = flag;
    e->rec = rec_cp(rec);
    return ret;
}

ZEBRA_RES rec_close(Records *pp)
{
    Records p = *pp;
    int i;
    ZEBRA_RES ret = ZEBRA_OK;

    if (!p)
	return ret;

    zebra_mutex_destroy(&p->mutex);
    if (rec_cache_flush(p, 0) != ZEBRA_OK)
	ret = ZEBRA_FAIL;

    xfree(p->record_cache);

    if (p->rw)
    {
        if (recindex_write_head(p->recindex, &p->head, sizeof(p->head)) != ZEBRA_OK)
	    ret = ZEBRA_FAIL;
    }

    recindex_close(p->recindex);

    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (p->data_BFile[i])
            bf_close(p->data_BFile[i]);
        xfree(p->data_fname[i]);
    }
    xfree(p->tmp_buf);
    xfree(p);
    *pp = NULL;
    return ret;
}

static Record rec_get_int(Records p, zint sysno)
{
    int i, in_size, r;
    Record rec, *recp;
    struct record_index_entry entry;
    zint freeblock;
    int dst_type;
    char *nptr, *cptr;
    char *in_buf = 0;
    char *bz_buf = 0;
    char compression_method;

    assert(sysno > 0);
    assert(p);

    if ((recp = rec_cache_lookup(p, sysno, recordFlagNop)))
        return rec_cp(*recp);

    if (recindex_read_indx(p->recindex, rec_sysno_to_int(sysno), &entry, sizeof(entry), 1) < 1)
        return NULL;       /* record is not there! */

    if (!entry.size)
        return NULL;       /* record is deleted */

    dst_type = (int) (entry.next & 7);
    assert(dst_type < REC_BLOCK_TYPES);
    freeblock = entry.next / 8;

    assert(freeblock > 0);

    rec_tmp_expand(p, entry.size);

    cptr = p->tmp_buf;
    r = bf_read(p->data_BFile[dst_type], freeblock, 0, 0, cptr);
    if (r < 0)
	return 0;
    memcpy(&freeblock, cptr, sizeof(freeblock));

    while (freeblock)
    {
        zint tmp;

        cptr += p->head.block_size[dst_type] - sizeof(freeblock);

        memcpy(&tmp, cptr, sizeof(tmp));
        r = bf_read(p->data_BFile[dst_type], freeblock, 0, 0, cptr);
	if (r < 0)
	    return 0;
        memcpy(&freeblock, cptr, sizeof(freeblock));
        memcpy(cptr, &tmp, sizeof(tmp));
    }

    rec = (Record) xmalloc(sizeof(*rec));
    rec->sysno = sysno;
    memcpy(&compression_method, p->tmp_buf + sizeof(zint) + sizeof(short),
	    sizeof(compression_method));
    in_buf = p->tmp_buf + sizeof(zint) + sizeof(short) + sizeof(char);
    in_size = entry.size - sizeof(short) - sizeof(char);
    switch (compression_method)
    {
    case REC_COMPRESS_ZLIB:
#if HAVE_ZLIB_H
        if (1)
        {
            unsigned int bz_size = entry.size * 20 + 100;
            while (1)
            {
                uLongf destLen = bz_size;
                bz_buf = (char *) xmalloc(bz_size);
                i = uncompress((Bytef *) bz_buf, &destLen,
                               (const Bytef *) in_buf, in_size);
                if (i == Z_OK)
                {
                    bz_size = destLen;
                    break;
                }
                yaz_log(YLOG_LOG, "failed");
                xfree(bz_buf);
                bz_size *= 2;
            }
            in_buf = bz_buf;
            in_size = bz_size;
        }
#else
        yaz_log(YLOG_FATAL, "cannot decompress record(s) in ZLIB format");
        return 0;
#endif
        break;
    case REC_COMPRESS_BZIP2:
#if HAVE_BZLIB_H
        if (1)
        {
            unsigned int bz_size = entry.size * 20 + 100;
            while (1)
            {
                bz_buf = (char *) xmalloc(bz_size);
#ifdef BZ_CONFIG_ERROR
                i = BZ2_bzBuffToBuffDecompress
#else
                    i = bzBuffToBuffDecompress
#endif
                    (bz_buf, &bz_size, in_buf, in_size, 0, 0);
                if (i == BZ_OK)
                    break;
                yaz_log(YLOG_LOG, "failed");
                xfree(bz_buf);
                bz_size *= 2;
            }
            in_buf = bz_buf;
            in_size = bz_size;
        }
#else
	yaz_log(YLOG_FATAL, "cannot decompress record(s) in BZIP2 format");
	return 0;
#endif
	break;
    case REC_COMPRESS_NONE:
	break;
    }
    for (i = 0; i<REC_NO_INFO; i++)
	rec->info[i] = 0;

    nptr = in_buf;                /* skip ref count */
    while (nptr < in_buf + in_size)
    {
	zint this_sysno;
	int len;
	rec_decode_zint(&this_sysno, (unsigned char *) nptr, &len);
	nptr += len;

	for (i = 0; i < REC_NO_INFO; i++)
	{
	    unsigned int this_size;
	    rec_decode_unsigned(&this_size, (unsigned char *) nptr, &len);
	    nptr += len;

	    if (this_size == 0)
		continue;
	    rec->size[i] = this_size-1;

	    if (rec->size[i])
	    {
		rec->info[i] = nptr;
		nptr += rec->size[i];
	    }
	    else
		rec->info[i] = NULL;
	}
	if (this_sysno == rec_sysno_to_int(sysno))
	    break;
    }
    for (i = 0; i<REC_NO_INFO; i++)
    {
	if (rec->info[i] && rec->size[i])
	{
	    char *np = xmalloc(rec->size[i]+1);
	    memcpy(np, rec->info[i], rec->size[i]);
            np[rec->size[i]] = '\0';
	    rec->info[i] = np;
	}
	else
	{
	    assert(rec->info[i] == 0);
	    assert(rec->size[i] == 0);
	}
    }
    xfree(bz_buf);
    if (rec_cache_insert(p, rec, recordFlagNop) != ZEBRA_OK)
	return 0;
    return rec;
}

Record rec_get(Records p, zint sysno)
{
    Record rec;
    zebra_mutex_lock(&p->mutex);

    rec = rec_get_int(p, sysno);
    zebra_mutex_unlock(&p->mutex);
    return rec;
}

Record rec_get_root(Records p)
{
    return rec_get(p, rec_sysno_to_ext(1));
}

Record rec_get_next(Records p, Record rec)
{
    Record next = 0;
    zint next_sysno_int = rec_sysno_to_int(rec->sysno);

    while (!next)
    {
         ++next_sysno_int;
        if (next_sysno_int == p->head.index_last)
            break;
        next = rec_get(p, rec_sysno_to_ext(next_sysno_int));
    }
    return next;
}

static Record rec_new_int(Records p)
{
    int i;
    zint sysno;
    Record rec;

    assert(p);
    rec = (Record) xmalloc(sizeof(*rec));
    if (1 || p->head.index_free == 0)
        sysno = (p->head.index_last)++;
    else
    {
        struct record_index_entry entry;

        if (recindex_read_indx(p->recindex, p->head.index_free, &entry, sizeof(entry), 0) < 1)
	{
	    xfree(rec);
	    return 0;
	}
        sysno = p->head.index_free;
        p->head.index_free = entry.next;
    }
    (p->head.no_records)++;
    rec->sysno = rec_sysno_to_ext(sysno);
    for (i = 0; i < REC_NO_INFO; i++)
    {
        rec->info[i] = NULL;
        rec->size[i] = 0;
    }
    rec_cache_insert(p, rec, recordFlagNew);
    return rec;
}

Record rec_new(Records p)
{
    Record rec;
    zebra_mutex_lock(&p->mutex);

    rec = rec_new_int(p);
    zebra_mutex_unlock(&p->mutex);
    return rec;
}

ZEBRA_RES rec_del(Records p, Record *recpp)
{
    Record *recp;
    ZEBRA_RES ret = ZEBRA_OK;

    zebra_mutex_lock(&p->mutex);
    (p->head.no_records)--;
    if ((recp = rec_cache_lookup(p, (*recpp)->sysno, recordFlagDelete)))
    {
        rec_free(recp);
        *recp = *recpp;
    }
    else
    {
        ret = rec_cache_insert(p, *recpp, recordFlagDelete);
        rec_free(recpp);
    }
    zebra_mutex_unlock(&p->mutex);
    *recpp = NULL;
    return ret;
}

ZEBRA_RES rec_put(Records p, Record *recpp)
{
    Record *recp;
    ZEBRA_RES ret = ZEBRA_OK;

    zebra_mutex_lock(&p->mutex);
    if ((recp = rec_cache_lookup(p, (*recpp)->sysno, recordFlagWrite)))
    {
        rec_free(recp);
        *recp = *recpp;
    }
    else
    {
        ret = rec_cache_insert(p, *recpp, recordFlagWrite);
        rec_free(recpp);
    }
    zebra_mutex_unlock(&p->mutex);
    *recpp = NULL;
    return ret;
}

void rec_free(Record *recpp)
{
    int i;

    if (!*recpp)
        return ;
    for (i = 0; i < REC_NO_INFO; i++)
        xfree((*recpp)->info[i]);
    xfree(*recpp);
    *recpp = NULL;
}

Record rec_cp(Record rec)
{
    Record n;
    int i;

    n = (Record) xmalloc(sizeof(*n));
    n->sysno = rec->sysno;
    for (i = 0; i < REC_NO_INFO; i++)
        if (!rec->info[i])
        {
            n->info[i] = NULL;
            n->size[i] = 0;
        }
        else
        {
            n->size[i] = rec->size[i];
            n->info[i] = (char *) xmalloc(rec->size[i]+1);
            memcpy(n->info[i], rec->info[i], rec->size[i]);
            n->info[i][rec->size[i]] = '\0';
        }
    return n;
}


char *rec_strdup(const char *s, size_t *len)
{
    char *p;

    if (!s)
    {
        *len = 0;
        return NULL;
    }
    *len = strlen(s)+1;
    p = (char *) xmalloc(*len);
    strcpy(p, s);
    return p;
}

void rec_prstat(Records records, int verbose)
{
    int i;
    zint total_bytes = 0;

    yaz_log (YLOG_LOG,
          "Total records                        %8" ZINT_FORMAT0,
          records->head.no_records);

    for (i = 0; i< REC_BLOCK_TYPES; i++)
    {
        yaz_log (YLOG_LOG, "Record blocks of size "ZINT_FORMAT,
              records->head.block_size[i]);
        yaz_log (YLOG_LOG,
          " Used/Total/Bytes used            "
	      ZINT_FORMAT "/" ZINT_FORMAT "/" ZINT_FORMAT,
              records->head.block_used[i], records->head.block_last[i]-1,
              records->head.block_used[i] * records->head.block_size[i]);
        total_bytes +=
            records->head.block_used[i] * records->head.block_size[i];

        yaz_log(YLOG_LOG, " Block Last " ZINT_FORMAT, records->head.block_last[i]);
        if (verbose)
        {   /* analyse free lists */
            zint no_free = 0;
            zint block_free = records->head.block_free[i];
            WRBUF w = wrbuf_alloc();
            while (block_free)
            {
                zint nblock;
                no_free++;
                wrbuf_printf(w, " " ZINT_FORMAT, block_free);
                if (bf_read(records->data_BFile[i],
                            block_free, 0, sizeof(nblock), &nblock) != 1)
                {
                    yaz_log(YLOG_FATAL|YLOG_ERRNO, "read in %s at free block "
                            ZINT_FORMAT,
                            records->data_fname[i], block_free);
                    break;
                }
                block_free = nblock;
            }
            yaz_log (YLOG_LOG,
                     " Number in free list       %8" ZINT_FORMAT0, no_free);
            if (no_free)
                yaz_log(YLOG_LOG, "%s", wrbuf_cstr(w));
            wrbuf_destroy(w);
        }
    }
    yaz_log (YLOG_LOG,
          "Total size of record index in bytes  %8" ZINT_FORMAT0,
          records->head.total_bytes);
    yaz_log (YLOG_LOG,
          "Total size with overhead             %8" ZINT_FORMAT0,
	  total_bytes);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

