/*
 * Copyright (c) 1995-2001, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isams.c,v $
 * Revision 1.2  2001-10-26 20:22:31  adam
 * Less LOG_LOG messages.
 *
 * Revision 1.1  1999/11/30 14:02:45  adam
 * Moved isams.
 *
 * Revision 1.5  1999/07/14 10:59:27  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.4  1999/05/26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.3  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.2  1999/05/15 14:35:48  adam
 * Minor changes.
 *
 * Revision 1.1  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <yaz/log.h>
#include <isams.h>

typedef struct {
    int last_offset;
    int last_block;
} ISAMS_head;

typedef unsigned ISAMS_BLOCK_SIZE;

struct ISAMS_s {
    ISAMS_M method;
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

void isams_getmethod (ISAMS_M m)
{
    m->code_start = NULL;
    m->code_item = NULL;
    m->code_stop = NULL;

    m->compare_item = NULL;

    m->debug = 1;
    m->block_size = 128;
}

ISAMS isams_open (BFiles bfs, const char *name, int writeflag,
		  ISAMS_M method)
{
    ISAMS is = (ISAMS) xmalloc (sizeof(*is));

    is->method = (ISAMS_M) xmalloc (sizeof(*is->method));
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

ISAMS_P isams_merge (ISAMS is, ISAMS_I data)
{
    char i_item[128], *i_item_ptr;
    int i_more, i_mode;
    void *r_clientData;
    int first_block = is->head.last_block;
    int first_offset = is->head.last_offset;
    int count = 0;

    r_clientData = (*is->method->code_start)(ISAMC_ENCODE);

    is->head.last_offset += sizeof(int);
    if (is->head.last_offset > is->block_size)
    {
	if (is->debug > 2)
	    logf (LOG_LOG, "first_block=%d", first_block);
	bf_write(is->bf, is->head.last_block, 0, 0, is->merge_buf);
	(is->head.last_block)++;
	is->head.last_offset -= is->block_size;
	memcpy (is->merge_buf, is->merge_buf + is->block_size,
		is->head.last_offset);
    }
    while (1)
    {
	i_item_ptr = i_item;
	i_more = (*data->read_item)(data->clientData, &i_item_ptr, &i_mode);
	assert (i_mode);
	
	if (!i_more)
	    break;
	else
	{
	    char *r_out_ptr = is->merge_buf + is->head.last_offset;
	    
	    i_item_ptr = i_item;
	    (*is->method->code_item)(ISAMC_ENCODE, r_clientData,
				     &r_out_ptr, &i_item_ptr);
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
    (*is->method->code_stop)(ISAMC_ENCODE, r_clientData);
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

ISAMS_PP isams_pp_open (ISAMS is, ISAMS_P pos)
{
    ISAMS_PP pp = (ISAMS_PP) xmalloc (sizeof(*pp));

    if (is->debug > 1)
	logf (LOG_LOG, "isams: isams_pp_open pos=%ld", (long) pos);
    pp->is = is;
    pp->decodeClientData = (*is->method->code_start)(ISAMC_DECODE);
    pp->numKeys = 0;
    pp->numRead = 0;
    pp->buf = (char *) xmalloc(is->block_size*2);
    pp->block_no = pos/is->block_size;
    pp->block_offset = pos - pp->block_no * is->block_size;
    if (is->debug)
        logf (LOG_LOG, "isams: isams_pp_open off=%d no=%d",
              pp->block_offset, pp->block_no);
    if (pos)
    {
	bf_read (is->bf, pp->block_no, 0, 0, pp->buf);
	bf_read (is->bf, pp->block_no+1, 0, 0, pp->buf + is->block_size);
	memcpy(&pp->numKeys, pp->buf + pp->block_offset, sizeof(int));
        if (is->debug)
	    logf (LOG_LOG, "isams: isams_pp_open numKeys=%d", pp->numKeys);
	pp->block_offset += sizeof(int);
    }
    return pp;
}

void isams_pp_close (ISAMS_PP pp)
{
    (*pp->is->method->code_stop)(ISAMC_DECODE, pp->decodeClientData);
    xfree(pp->buf);
    xfree(pp);
}

int isams_pp_num (ISAMS_PP pp)
{
    return pp->numKeys;
}

int isams_pp_read (ISAMS_PP pp, void *buf)
{
    return isams_read_item (pp, (char **) &buf);
}

int isams_read_item (ISAMS_PP pp, char **dst)
{
    char *src;
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
    (*pp->is->method->code_item)(ISAMC_DECODE, pp->decodeClientData,
				 dst, &src);
    pp->block_offset = src - pp->buf; 
    return 1;
}


