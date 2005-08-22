/* $Id: recindex.c,v 1.46 2005-08-22 08:18:43 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#define RIDX_CHUNK 128

/*
 *  Format of first block
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <yaz/yaz-util.h>
#include "recindxp.h"

#if HAVE_BZLIB_H
#include <bzlib.h>
#endif

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

static SYSNO rec_sysno_to_ext(SYSNO sysno)
{
    assert(sysno >= 0 && sysno <= USUAL_RANGE);
    return sysno + FAKE_OFFSET;
}

SYSNO rec_sysno_to_int(SYSNO sysno)
{
    assert(sysno >= FAKE_OFFSET && sysno <= FAKE_OFFSET + USUAL_RANGE);
    return sysno - FAKE_OFFSET;
}

static void rec_write_head(Records p)
{
    int r;

    assert(p);
    assert(p->index_BFile);

    r = bf_write(p->index_BFile, 0, 0, sizeof(p->head), &p->head);    
    if (r)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "write head of %s", p->index_fname);
        exit(1);
    }
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

static int read_indx(Records p, SYSNO sysno, void *buf, int itemsize, 
                      int ignoreError)
{
    int r;
    zint pos = (sysno-1)*itemsize;
    int off = (int) (pos%RIDX_CHUNK);
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
        exit(1);
    }
    return r;
}

static void write_indx(Records p, SYSNO sysno, void *buf, int itemsize)
{
    zint pos = (sysno-1)*itemsize;
    int off = (int) (pos%RIDX_CHUNK);
    int sz1 = RIDX_CHUNK - off;    /* sz1 is size of buffer to read.. */

    if (sz1 > itemsize)
	sz1 = itemsize;  /* no more than itemsize bytes */

    bf_write(p->index_BFile, 1+pos/RIDX_CHUNK, off, sz1, buf);
    if (sz1 < itemsize)   /* boundary? must write second part */
	bf_write(p->index_BFile, 2+pos/RIDX_CHUNK, 0, itemsize - sz1,
		(char*) buf + sz1);
}

static void rec_release_blocks(Records p, SYSNO sysno)
{
    struct record_index_entry entry;
    zint freeblock;
    char block_and_ref[sizeof(zint) + sizeof(short)];
    int dst_type;
    int first = 1;

    if (read_indx(p, sysno, &entry, sizeof(entry), 1) != 1)
        return ;

    freeblock = entry.next;
    assert(freeblock > 0);
    dst_type = (int) (freeblock & 7);
    assert(dst_type < REC_BLOCK_TYPES);
    freeblock = freeblock / 8;
    while (freeblock)
    {
        if (bf_read(p->data_BFile[dst_type], freeblock, 0,
		     first ? sizeof(block_and_ref) : sizeof(zint),
		     block_and_ref) != 1)
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "read in rec_del_single");
            exit(1);
        }
	if (first)
	{
	    short ref;
	    memcpy(&ref, block_and_ref + sizeof(freeblock), sizeof(ref));
	    --ref;
	    memcpy(block_and_ref + sizeof(freeblock), &ref, sizeof(ref));
	    if (ref)
	    {
		if (bf_write(p->data_BFile[dst_type], freeblock, 0,
			      sizeof(block_and_ref), block_and_ref))
		{
		    yaz_log(YLOG_FATAL|YLOG_ERRNO, "write in rec_del_single");
		    exit(1);
		}
		return;
	    }
	    first = 0;
	}
	
        if (bf_write(p->data_BFile[dst_type], freeblock, 0, sizeof(freeblock),
                      &p->head.block_free[dst_type]))
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "write in rec_del_single");
            exit(1);
        }
        p->head.block_free[dst_type] = freeblock;
        memcpy(&freeblock, block_and_ref, sizeof(freeblock));

        p->head.block_used[dst_type]--;
    }
    p->head.total_bytes -= entry.size;
}

static void rec_delete_single(Records p, Record rec)
{
    struct record_index_entry entry;

    rec_release_blocks(p, rec_sysno_to_int(rec->sysno));

    entry.next = p->head.index_free;
    entry.size = 0;
    p->head.index_free = rec_sysno_to_int(rec->sysno);
    write_indx(p, rec_sysno_to_int(rec->sysno), &entry, sizeof(entry));
}

static void rec_write_tmp_buf(Records p, int size, SYSNO *sysnos)
{
    struct record_index_entry entry;
    int no_written = 0;
    char *cptr = p->tmp_buf;
    zint block_prev = -1, block_free;
    int dst_type = 0;
    int i;

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
		exit(1);
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
		write_indx(p, *sysnos, &entry, sizeof(entry));
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
        no_written += (int)(p->head.block_size[dst_type]) - sizeof(zint);
        p->head.block_used[dst_type]++;
    }
    assert(block_prev != -1);
    block_free = 0;
    memcpy(cptr, &block_free, sizeof(block_free));
    bf_write(p->data_BFile[dst_type], block_prev, 0,
              sizeof(block_free) + (p->tmp_buf+size) - cptr, cptr);
}

Records rec_open(BFiles bfs, int rw, int compression_method)
{
    Records p;
    int i, r;
    int version;

    p = (Records) xmalloc(sizeof(*p));
    p->compression_method = compression_method;
    p->rw = rw;
    p->tmp_size = 1024;
    p->tmp_buf = (char *) xmalloc(p->tmp_size);
    p->index_fname = "reci";
    p->index_BFile = bf_open(bfs, p->index_fname, RIDX_CHUNK, rw);
    if (p->index_BFile == NULL)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "open %s", p->index_fname);
        exit(1);
    }
    r = bf_read(p->index_BFile, 0, 0, 0, p->tmp_buf);
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
        p->head.block_size[0] = 128;
        p->head.block_move[0] = 0;
        for (i = 1; i<REC_BLOCK_TYPES; i++)
        {
            p->head.block_size[i] = p->head.block_size[i-1] * 4;
            p->head.block_move[i] = p->head.block_size[i] * 24;
        }
        if (rw)
            rec_write_head(p);
        break;
    case 1:
        memcpy(&p->head, p->tmp_buf, sizeof(p->head));
        if (memcmp(p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic)))
        {
            yaz_log(YLOG_FATAL, "file %s has bad format", p->index_fname);
            exit(1);
        }
	version = atoi(p->head.version);
	if (version != REC_VERSION)
	{
	    yaz_log(YLOG_FATAL, "file %s is version %d, but version"
		  " %d is required", p->index_fname, version, REC_VERSION);
	    exit(1);
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
        if (!(p->data_BFile[i] = bf_open(bfs, p->data_fname[i],
					 (int) (p->head.block_size[i]),
					 rw)))
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "bf_open %s", p->data_fname[i]);
            exit(1);
        }
    }
    p->cache_max = 400;
    p->cache_cur = 0;
    p->record_cache = (struct record_cache_entry *)
	xmalloc(sizeof(*p->record_cache)*p->cache_max);
    zebra_mutex_init(&p->mutex);
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
	if (*out_offset + (int) rec->size[i] + 20 > *out_size)
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

static void rec_write_multiple(Records p, int saveCount)
{
    int i;
    short ref_count = 0;
    char compression_method;
    Record last_rec = 0;
    int out_size = 1000;
    int out_offset = 0;
    char *out_buf = (char *) xmalloc(out_size);
    SYSNO *sysnos = (SYSNO *) xmalloc(sizeof(*sysnos) * (p->cache_cur + 1));
    SYSNO *sysnop = sysnos;

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
	    rec_release_blocks(p, rec_sysno_to_int(e->rec->sysno));
            rec_cache_flush_block1(p, e->rec, last_rec, &out_buf,
				    &out_size, &out_offset);
	    *sysnop++ = rec_sysno_to_int(e->rec->sysno);
	    ref_count++;
	    e->flag = recordFlagNop;
	    last_rec = e->rec;
            break;
        case recordFlagDelete:
            rec_delete_single(p, e->rec);
	    e->flag = recordFlagNop;
            break;
	default:
	    break;
        }
    }

    *sysnop = -1;
    if (ref_count)
    {
	unsigned int csize = 0;  /* indicate compression "not performed yet" */
	compression_method = p->compression_method;
	switch (compression_method)
	{
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
	    yaz_log(YLOG_LOG, "compress %4d %5d %5d", ref_count, out_offset,
		  csize);
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
	rec_write_tmp_buf(p, csize + sizeof(short) + sizeof(char), sysnos);
    }
    xfree(out_buf);
    xfree(sysnos);
}

static void rec_cache_flush(Records p, int saveCount)
{
    int i, j;

    if (saveCount >= p->cache_cur)
        saveCount = 0;

    rec_write_multiple(p, saveCount);

    for (i = 0; i<p->cache_cur - saveCount; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        rec_rm(&e->rec);
    } 
    /* i still being used ... */
    for (j = 0; j<saveCount; j++, i++)
        memcpy(p->record_cache+j, p->record_cache+i,
                sizeof(*p->record_cache));
    p->cache_cur = saveCount;
}

static Record *rec_cache_lookup(Records p, SYSNO sysno,
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

static void rec_cache_insert(Records p, Record rec, enum recordCacheFlag flag)
{
    struct record_cache_entry *e;

    if (p->cache_cur == p->cache_max)
        rec_cache_flush(p, 1);
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
        if (used > 90000)
            rec_cache_flush(p, 1);
    }
    assert(p->cache_cur < p->cache_max);

    e = p->record_cache + (p->cache_cur)++;
    e->flag = flag;
    e->rec = rec_cp(rec);
}

void rec_close(Records *pp)
{
    Records p = *pp;
    int i;

    assert(p);

    zebra_mutex_destroy(&p->mutex);
    rec_cache_flush(p, 0);
    xfree(p->record_cache);

    if (p->rw)
        rec_write_head(p);

    if (p->index_BFile)
        bf_close(p->index_BFile);

    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (p->data_BFile[i])
            bf_close(p->data_BFile[i]);
        xfree(p->data_fname[i]);
    }
    xfree(p->tmp_buf);
    xfree(p);
    *pp = NULL;
}

static Record rec_get_int(Records p, SYSNO sysno)
{
    int i, in_size, r;
    Record rec, *recp;
    struct record_index_entry entry;
    zint freeblock;
    int dst_type;
    char *nptr, *cptr;
    char *in_buf = 0;
    char *bz_buf = 0;
#if HAVE_BZLIB_H
    unsigned int bz_size;
#endif
    char compression_method;

    assert(sysno > 0);
    assert(p);

    if ((recp = rec_cache_lookup(p, sysno, recordFlagNop)))
        return rec_cp(*recp);

    if (read_indx(p, rec_sysno_to_int(sysno), &entry, sizeof(entry), 1) < 1)
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
    case REC_COMPRESS_BZIP2:
#if HAVE_BZLIB_H
	bz_size = entry.size * 20 + 100;
	while (1)
	{
	    bz_buf = (char *) xmalloc(bz_size);
#ifdef BZ_CONFIG_ERROR
	    i = BZ2_bzBuffToBuffDecompress
#else
	    i = bzBuffToBuffDecompress
#endif
                (bz_buf, &bz_size, in_buf, in_size, 0, 0);
	    yaz_log(YLOG_LOG, "decompress %5d %5d", in_size, bz_size);
	    if (i == BZ_OK)
		break;
	    yaz_log(YLOG_LOG, "failed");
	    xfree(bz_buf);
            bz_size *= 2;
	}
	in_buf = bz_buf;
	in_size = bz_size;
#else
	yaz_log(YLOG_FATAL, "cannot decompress record(s) in BZIP2 format");
	exit(1);
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
    rec_cache_insert(p, rec, recordFlagNop);
    return rec;
}

Record rec_get(Records p, SYSNO sysno)
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

static Record rec_new_int(Records p)
{
    int i;
    SYSNO sysno;
    Record rec;

    assert(p);
    rec = (Record) xmalloc(sizeof(*rec));
    if (1 || p->head.index_free == 0)
        sysno = (p->head.index_last)++;
    else
    {
        struct record_index_entry entry;

        read_indx(p, p->head.index_free, &entry, sizeof(entry), 0);
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

void rec_del(Records p, Record *recpp)
{
    Record *recp;

    zebra_mutex_lock(&p->mutex);
    (p->head.no_records)--;
    if ((recp = rec_cache_lookup(p, (*recpp)->sysno, recordFlagDelete)))
    {
        rec_rm(recp);
        *recp = *recpp;
    }
    else
    {
        rec_cache_insert(p, *recpp, recordFlagDelete);
        rec_rm(recpp);
    }
    zebra_mutex_unlock(&p->mutex);
    *recpp = NULL;
}

void rec_put(Records p, Record *recpp)
{
    Record *recp;

    zebra_mutex_lock(&p->mutex);
    if ((recp = rec_cache_lookup(p, (*recpp)->sysno, recordFlagWrite)))
    {
        rec_rm(recp);
        *recp = *recpp;
    }
    else
    {
        rec_cache_insert(p, *recpp, recordFlagWrite);
        rec_rm(recpp);
    }
    zebra_mutex_unlock(&p->mutex);
    *recpp = NULL;
}

void rec_rm(Record *recpp)
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
            n->info[i] = (char *) xmalloc(rec->size[i]);
            memcpy(n->info[i], rec->info[i], rec->size[i]);
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

