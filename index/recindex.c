/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.c,v $
 * Revision 1.25  1999-07-06 12:28:04  adam
 * Updated record index structure. Format includes version ID. Compression
 * algorithm ID is stored for each record block.
 *
 * Revision 1.24  1999/06/25 13:48:02  adam
 * Updated MSVC project files.
 * Added BZIP2 record compression (not very well tested).
 *
 * Revision 1.23  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.22  1999/02/18 12:49:34  adam
 * Changed file naming scheme for register files as well as record
 * store/index files.
 *
 * Revision 1.21  1999/02/02 14:51:03  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.20  1998/01/12 15:04:08  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.19  1997/09/17 12:19:16  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.18  1997/07/15 16:28:42  adam
 * Bug fix: storeData didn't work with files with multiple records.
 * Bug fix: fixed memory management with records; not really well
 *  thought through.
 *
 * Revision 1.17  1997/02/12 20:39:46  adam
 * Implemented options -f <n> that limits the log to the first <n>
 * records.
 * Changed some log messages also.
 *
 * Revision 1.16  1996/06/04 10:19:00  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.15  1996/05/13  14:23:06  adam
 * Work on compaction of set/use bytes in dictionary.
 *
 * Revision 1.14  1996/02/01  20:48:15  adam
 * The total size of records are always checked in rec_cache_insert to
 * reduce memory usage.
 *
 * Revision 1.13  1995/12/11  09:12:49  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.12  1995/12/07  17:38:47  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.11  1995/12/06  13:58:26  adam
 * Improved flushing of records - all flushes except the last one
 * don't write the last accessed. Also flush takes place if record
 * info occupy more than about 256k.
 *
 * Revision 1.10  1995/12/06  12:41:24  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.9  1995/11/30  08:34:33  adam
 * Started work on commit facility.
 * Changed a few malloc/free to xmalloc/xfree.
 *
 * Revision 1.8  1995/11/28  14:26:21  adam
 * Bug fix: recordId with constant wasn't right.
 * Bug fix: recordId dictionary entry wasn't deleted when needed.
 *
 * Revision 1.7  1995/11/28  09:09:43  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.6  1995/11/25  10:24:06  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.5  1995/11/22  17:19:18  adam
 * Record management uses the bfile system.
 *
 * Revision 1.4  1995/11/20  16:59:46  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.3  1995/11/16  15:34:55  adam
 * Uses new record management system in both indexer and server.
 *
 * Revision 1.2  1995/11/15  19:13:08  adam
 * Work on record management.
 *
 * Revision 1.1  1995/11/15  14:46:20  adam
 * Started work on better record management system.
 *
 */


/*
 *  Format of first block
 *      next       (4 bytes)
 *      ref_count  (4 bytes)
 *      block      (504 bytes)
 *
 *  Format of subsequent blocks 
 *      next  (4 bytes)
 *      block (508 bytes)
 *
 *  Format of each record
 *      sysno
 *      (length, data) - pairs
 *      length = 0 if same as previous
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "recindxp.h"

#if HAVE_BZLIB_H
#include <bzlib.h>
#endif
static void rec_write_head (Records p)
{
    int r;

    assert (p);
    assert (p->index_BFile);

    r = bf_write (p->index_BFile, 0, 0, sizeof(p->head), &p->head);    
    if (r)
    {
        logf (LOG_FATAL|LOG_ERRNO, "write head of %s", p->index_fname);
        exit (1);
    }
}

static void rec_tmp_expand (Records p, int size)
{
    if (p->tmp_size < size + 2048 ||
        p->tmp_size < p->head.block_size[REC_BLOCK_TYPES-1]*2)
    {
        xfree (p->tmp_buf);
        p->tmp_size = size + p->head.block_size[REC_BLOCK_TYPES-1]*2 + 2048;
        p->tmp_buf = (char *) xmalloc (p->tmp_size);
    }
}

static int read_indx (Records p, int sysno, void *buf, int itemsize, 
                      int ignoreError)
{
    int r;
    int pos = (sysno-1)*itemsize;

    r = bf_read (p->index_BFile, 1+pos/128, pos%128, itemsize, buf);
    if (r != 1 && !ignoreError)
    {
        logf (LOG_FATAL|LOG_ERRNO, "read in %s at pos %ld",
              p->index_fname, (long) pos);
        exit (1);
    }
    return r;
}

static void write_indx (Records p, int sysno, void *buf, int itemsize)
{
    int pos = (sysno-1)*itemsize;

    bf_write (p->index_BFile, 1+pos/128, pos%128, itemsize, buf);
}

static void rec_release_blocks (Records p, int sysno)
{
    struct record_index_entry entry;
    int freeblock;
    char block_and_ref[sizeof(short) + sizeof(int)];
    int dst_type;
    int first = 1;

    if (read_indx (p, sysno, &entry, sizeof(entry), 1) != 1)
        return ;

    freeblock = entry.next;
    assert (freeblock > 0);
    dst_type = freeblock & 7;
    assert (dst_type < REC_BLOCK_TYPES);
    freeblock = freeblock / 8;
    while (freeblock)
    {
        if (bf_read (p->data_BFile[dst_type], freeblock, 0,
		     sizeof(block_and_ref), block_and_ref) != 1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "read in rec_del_single");
            exit (1);
        }
	if (first)
	{
	    short ref;
	    memcpy (&ref, block_and_ref + sizeof(int), sizeof(ref));
	    --ref;
	    memcpy (block_and_ref + sizeof(int), &ref, sizeof(ref));
	    if (ref)
	    {
		if (bf_write (p->data_BFile[dst_type], freeblock, 0,
			      sizeof(block_and_ref), block_and_ref))
		{
		    logf (LOG_FATAL|LOG_ERRNO, "write in rec_del_single");
		    exit (1);
		}
		return;
	    }
	    first = 0;
	}
	
        if (bf_write (p->data_BFile[dst_type], freeblock, 0, sizeof(freeblock),
                      &p->head.block_free[dst_type]))
        {
            logf (LOG_FATAL|LOG_ERRNO, "write in rec_del_single");
            exit (1);
        }
        p->head.block_free[dst_type] = freeblock;
        memcpy (&freeblock, block_and_ref, sizeof(int));

        p->head.block_used[dst_type]--;
    }
    p->head.total_bytes -= entry.size;
}

static void rec_delete_single (Records p, Record rec)
{
    struct record_index_entry entry;

    rec_release_blocks (p, rec->sysno);

    entry.next = p->head.index_free;
    entry.size = 0;
    p->head.index_free = rec->sysno;
    write_indx (p, rec->sysno, &entry, sizeof(entry));
}

static void rec_write_tmp_buf (Records p, int size, int *sysnos)
{
    struct record_index_entry entry;
    int no_written = 0;
    char *cptr = p->tmp_buf;
    int block_prev = -1, block_free;
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
            if (bf_read (p->data_BFile[dst_type],
                         block_free, 0, sizeof(*p->head.block_free),
                         &p->head.block_free[dst_type]) != 1)
            {
                logf (LOG_FATAL|LOG_ERRNO, "read in %s at free block %d",
                      p->data_fname[dst_type], block_free);
		exit (1);
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
		write_indx (p, *sysnos, &entry, sizeof(entry));
		sysnos++;
	    }
        }
        else
        {
            memcpy (cptr, &block_free, sizeof(int));
            bf_write (p->data_BFile[dst_type], block_prev, 0, 0, cptr);
            cptr = p->tmp_buf + no_written;
        }
        block_prev = block_free;
        no_written += p->head.block_size[dst_type] - sizeof(int);
        p->head.block_used[dst_type]++;
    }
    assert (block_prev != -1);
    block_free = 0;
    memcpy (cptr, &block_free, sizeof(int));
    bf_write (p->data_BFile[dst_type], block_prev, 0,
              sizeof(int) + (p->tmp_buf+size) - cptr, cptr);
}

Records rec_open (BFiles bfs, int rw, int compression_method)
{
    Records p;
    int i, r;
    int version;

    p = (Records) xmalloc (sizeof(*p));
    p->compression_method = compression_method;
    p->rw = rw;
    p->tmp_size = 1024;
    p->tmp_buf = (char *) xmalloc (p->tmp_size);
    p->index_fname = "reci";
    p->index_BFile = bf_open (bfs, p->index_fname, 128, rw);
    if (p->index_BFile == NULL)
    {
        logf (LOG_FATAL|LOG_ERRNO, "open %s", p->index_fname);
        exit (1);
    }
    r = bf_read (p->index_BFile, 0, 0, 0, p->tmp_buf);
    switch (r)
    {
    case 0:
        memcpy (p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic));
	sprintf (p->head.version, "%3d", REC_VERSION);
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
            rec_write_head (p);
        break;
    case 1:
        memcpy (&p->head, p->tmp_buf, sizeof(p->head));
        if (memcmp (p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic)))
        {
            logf (LOG_FATAL, "file %s has bad format", p->index_fname);
            exit (1);
        }
	version = atoi (p->head.version);
	if (version != REC_VERSION)
	{
	    logf (LOG_FATAL, "file %s is version %d, but version"
		  " %d is required", p->index_fname, version, REC_VERSION);
	    exit (1);
	}
        break;
    }
    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        char str[80];
        sprintf (str, "recd%c", i + 'A');
        p->data_fname[i] = (char *) xmalloc (strlen(str)+1);
        strcpy (p->data_fname[i], str);
        p->data_BFile[i] = NULL;
    }
    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (!(p->data_BFile[i] = bf_open (bfs, p->data_fname[i],
                                          p->head.block_size[i],
                                          rw)))
        {
            logf (LOG_FATAL|LOG_ERRNO, "bf_open %s", p->data_fname[i]);
            exit (1);
        }
    }
    p->cache_max = 400;
    p->cache_cur = 0;
    p->record_cache = (struct record_cache_entry *)
	xmalloc (sizeof(*p->record_cache)*p->cache_max);
    return p;
}

static void rec_encode_unsigned (unsigned n, unsigned char *buf, int *len)
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
static void rec_cache_flush_block1 (Records p, Record rec, Record last_rec,
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
	    char *np = (char *) xmalloc (new_size);
	    if (*out_offset)
		memcpy (np, *out_buf, *out_offset);
	    xfree (*out_buf);
	    *out_size = new_size;
	    *out_buf = np;
	}
	if (i == 0)
	{
	    rec_encode_unsigned (rec->sysno, *out_buf + *out_offset, &len);
	    (*out_offset) += len;
	}
	if (rec->size[i] == 0)
	{
	    rec_encode_unsigned (1, *out_buf + *out_offset, &len);
	    (*out_offset) += len;
	}
	else if (last_rec && rec->size[i] == last_rec->size[i] &&
		 !memcmp (rec->info[i], last_rec->info[i], rec->size[i]))
	{
	    rec_encode_unsigned (0, *out_buf + *out_offset, &len);
	    (*out_offset) += len;
	}
	else
	{
	    rec_encode_unsigned (rec->size[i]+1, *out_buf + *out_offset, &len);
	    (*out_offset) += len;
	    memcpy (*out_buf + *out_offset, rec->info[i], rec->size[i]);
	    (*out_offset) += rec->size[i];
	}
    }
}

static void rec_write_multiple (Records p, int saveCount)
{
    int i;
    short ref_count = 0;
    char compression_method;
    Record last_rec = 0;
    int out_size = 1000;
    int out_offset = 0;
    char *out_buf = (char *) xmalloc (out_size);
    int *sysnos = (int *) xmalloc (sizeof(*sysnos) * (p->cache_cur + 1));
    int *sysnop = sysnos;

    for (i = 0; i<p->cache_cur - saveCount; i++)
    {
        struct record_cache_entry *e = p->record_cache + i;
        switch (e->flag)
        {
        case recordFlagNew:
            rec_cache_flush_block1 (p, e->rec, last_rec, &out_buf,
				    &out_size, &out_offset);
	    *sysnop++ = e->rec->sysno;
	    ref_count++;
	    e->flag = recordFlagNop;
	    last_rec = e->rec;
            break;
        case recordFlagWrite:
	    rec_release_blocks (p, e->rec->sysno);
            rec_cache_flush_block1 (p, e->rec, last_rec, &out_buf,
				    &out_size, &out_offset);
	    *sysnop++ = e->rec->sysno;
	    ref_count++;
	    e->flag = recordFlagNop;
	    last_rec = e->rec;
            break;
        case recordFlagDelete:
            rec_delete_single (p, e->rec);
	    e->flag = recordFlagNop;
            break;
	default:
	    break;
        }
        rec_rm (&e->rec);
    }
    *sysnop = -1;
    if (ref_count)
    {
	int csize = 0;  /* indicate compression "not performed yet" */
	compression_method = p->compression_method;
	switch (compression_method)
	{
	case REC_COMPRESS_BZIP2:
#if HAVE_BZLIB_H	
	    csize = out_offset + (out_offset >> 6) + 620;
	    rec_tmp_expand (p, csize);
	    i = bzBuffToBuffCompress (p->tmp_buf+sizeof(int)+sizeof(short)+
				      sizeof(char),
				      &csize, out_buf, out_offset, 1, 0, 30);
	    if (i != BZ_OK)
	    {
		logf (LOG_WARN, "bzBuffToBuffCompress error code=%d", i);
		csize = 0;
	    }
	    logf (LOG_LOG, "compress %4d %5d %5d", ref_count, out_offset,
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
	    rec_tmp_expand (p, csize);
	    memcpy (p->tmp_buf + sizeof(int) + sizeof(short) + sizeof(char),
		    out_buf, out_offset);
	    csize = out_offset;
	    compression_method = REC_COMPRESS_NONE;
	}
	memcpy (p->tmp_buf + sizeof(int), &ref_count, sizeof(ref_count));
	memcpy (p->tmp_buf + sizeof(int)+sizeof(short),
		&compression_method, sizeof(compression_method));
		
	/* -------- compression */
	rec_write_tmp_buf (p, csize + sizeof(short) + sizeof(char), sysnos);
    }
    xfree (out_buf);
    xfree (sysnos);
}

static void rec_cache_flush (Records p, int saveCount)
{
    int i, j;

    if (saveCount >= p->cache_cur)
        saveCount = 0;

    rec_write_multiple (p, saveCount);
    for (j = 0; j<saveCount; j++, i++)
        memcpy (p->record_cache+j, p->record_cache+i,
                sizeof(*p->record_cache));
    p->cache_cur = saveCount;
}

static Record *rec_cache_lookup (Records p, int sysno,
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

static void rec_cache_insert (Records p, Record rec, enum recordCacheFlag flag)
{
    struct record_cache_entry *e;

    if (p->cache_cur == p->cache_max)
        rec_cache_flush (p, 1);
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
            rec_cache_flush (p, 1);
    }
    assert (p->cache_cur < p->cache_max);

    e = p->record_cache + (p->cache_cur)++;
    e->flag = flag;
    e->rec = rec_cp (rec);
}

void rec_close (Records *pp)
{
    Records p = *pp;
    int i;

    assert (p);

    rec_cache_flush (p, 0);
    xfree (p->record_cache);

    if (p->rw)
        rec_write_head (p);

    if (p->index_BFile)
        bf_close (p->index_BFile);

    for (i = 0; i<REC_BLOCK_TYPES; i++)
    {
        if (p->data_BFile[i])
            bf_close (p->data_BFile[i]);
        xfree (p->data_fname[i]);
    }
    xfree (p->tmp_buf);
    xfree (p);
    *pp = NULL;
}


Record rec_get (Records p, int sysno)
{
    int i, in_size;
    Record rec, *recp;
    struct record_index_entry entry;
    int freeblock, dst_type;
    char *nptr, *cptr;
    char *in_buf = 0;
    char *bz_buf = 0;
    int bz_size;
    char compression_method;

    assert (sysno > 0);
    assert (p);

    if ((recp = rec_cache_lookup (p, sysno, recordFlagNop)))
        return rec_cp (*recp);

    if (!read_indx (p, sysno, &entry, sizeof(entry), 1))
        return NULL;       /* record is not there! */

    if (!entry.size)
        return NULL;       /* record is deleted */

    dst_type = entry.next & 7;
    assert (dst_type < REC_BLOCK_TYPES);
    freeblock = entry.next / 8;

    assert (freeblock > 0);
    
    rec = (Record) xmalloc (sizeof(*rec));
    rec_tmp_expand (p, entry.size);

    cptr = p->tmp_buf;
    bf_read (p->data_BFile[dst_type], freeblock, 0, 0, cptr);
    memcpy (&freeblock, cptr, sizeof(freeblock));

    while (freeblock)
    {
        int tmp;

        cptr += p->head.block_size[dst_type] - sizeof(freeblock);
        
        memcpy (&tmp, cptr, sizeof(tmp));
        bf_read (p->data_BFile[dst_type], freeblock, 0, 0, cptr);
        memcpy (&freeblock, cptr, sizeof(freeblock));
        memcpy (cptr, &tmp, sizeof(tmp));
    }

    rec->sysno = sysno;
    memcpy (&compression_method, p->tmp_buf + sizeof(int) + sizeof(short),
	    sizeof(compression_method));
    in_buf = p->tmp_buf + sizeof(int) + sizeof(short) + sizeof(char);
    in_size = entry.size - sizeof(short) - sizeof(char);
    switch (compression_method)
    {
    case REC_COMPRESS_BZIP2:
#if HAVE_BZLIB_H
	bz_size = entry.size * 30+100;
	bz_buf = (char *) xmalloc (bz_size);
	i = bzBuffToBuffDecompress (bz_buf, &bz_size, in_buf, in_size, 0, 0);
	logf (LOG_LOG, "decompress %5d %5d", in_size, bz_size);
	if (i != BZ_OK)
	{
	    logf (LOG_FATAL, "bzBuffToBuffDecompress error code=%d", i);
	    exit (1);
	}
	in_buf = bz_buf;
	in_size = bz_size;
#else
	logf (LOG_FATAL, "cannot decompress record(s) in BZIP2 format");
	exit (1);
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
	int this_sysno;
	int len;
	rec_decode_unsigned (&this_sysno, nptr, &len);
	nptr += len;

	for (i = 0; i < REC_NO_INFO; i++)
	{
	    int this_size;
	    rec_decode_unsigned (&this_size, nptr, &len);
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
	if (this_sysno == sysno)
	    break;
    }
    for (i = 0; i<REC_NO_INFO; i++)
    {
	if (rec->info[i] && rec->size[i])
	{
	    char *np = xmalloc (rec->size[i]);
	    memcpy (np, rec->info[i], rec->size[i]);
	    rec->info[i] = np;
	}
	else
	{
	    assert (rec->info[i] == 0);
	    assert (rec->size[i] == 0);
	}
    }
    xfree (bz_buf);
    rec_cache_insert (p, rec, recordFlagNop);
    return rec;
}

Record rec_new (Records p)
{
    int sysno, i;
    Record rec;

    assert (p);
    rec = (Record) xmalloc (sizeof(*rec));
    if (1 || p->head.index_free == 0)
        sysno = (p->head.index_last)++;
    else
    {
        struct record_index_entry entry;

        read_indx (p, p->head.index_free, &entry, sizeof(entry), 0);
        sysno = p->head.index_free;
        p->head.index_free = entry.next;
    }
    (p->head.no_records)++;
    rec->sysno = sysno;
    for (i = 0; i < REC_NO_INFO; i++)
    {
        rec->info[i] = NULL;
        rec->size[i] = 0;
    }
    rec_cache_insert (p, rec, recordFlagNew);
    return rec;
}

void rec_del (Records p, Record *recpp)
{
    Record *recp;

    (p->head.no_records)--;
    if ((recp = rec_cache_lookup (p, (*recpp)->sysno, recordFlagDelete)))
    {
        rec_rm (recp);
        *recp = *recpp;
    }
    else
    {
        rec_cache_insert (p, *recpp, recordFlagDelete);
        rec_rm (recpp);
    }
    *recpp = NULL;
}

void rec_put (Records p, Record *recpp)
{
    Record *recp;

    if ((recp = rec_cache_lookup (p, (*recpp)->sysno, recordFlagWrite)))
    {
        rec_rm (recp);
        *recp = *recpp;
    }
    else
    {
        rec_cache_insert (p, *recpp, recordFlagWrite);
        rec_rm (recpp);
    }
    *recpp = NULL;
}

void rec_rm (Record *recpp)
{
    int i;

    if (!*recpp)
        return ;
    for (i = 0; i < REC_NO_INFO; i++)
        xfree ((*recpp)->info[i]);
    xfree (*recpp);
    *recpp = NULL;
}

Record rec_cp (Record rec)
{
    Record n;
    int i;

    n = (Record) xmalloc (sizeof(*n));
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
            n->info[i] = (char *) xmalloc (rec->size[i]);
            memcpy (n->info[i], rec->info[i], rec->size[i]);
        }
    return n;
}


char *rec_strdup (const char *s, size_t *len)
{
    char *p;

    if (!s)
    {
        *len = 0;
        return NULL;
    }
    *len = strlen(s)+1;
    p = (char *) xmalloc (*len);
    strcpy (p, s);
    return p;
}

