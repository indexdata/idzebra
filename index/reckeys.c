/* $Id: reckeys.c,v 1.2 2005-11-09 08:27:28 adam Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "index.h"
#include "reckeys.h"

struct zebra_rec_keys_t_ {
    size_t buf_used;
    size_t buf_max;
    size_t fetch_offset;
    char *buf;
    void *encode_handle;
    void *decode_handle;
    char owner_of_buffer;
};

zebra_rec_keys_t zebra_rec_keys_open()
{
    zebra_rec_keys_t p = xmalloc(sizeof(*p));
    p->buf_used = 0;
    p->buf_max = 0;
    p->fetch_offset = 0;
    p->buf = 0;
    p->owner_of_buffer = 1;
    p->encode_handle = iscz1_start();
    p->decode_handle = iscz1_start(); 
    return p;
}
    
void zebra_rec_keys_set_buf(zebra_rec_keys_t p, char *buf, size_t sz,
			    int copy_buf)
{
    if (p->owner_of_buffer)
	xfree(p->buf);
    p->buf_used = sz;
    p->buf_max = sz;
    if (!copy_buf)
    {
	p->buf = buf;
    }
    else
    {
	if (!sz)
	    p->buf = 0;
	else
	{
	    p->buf = xmalloc(sz);
	    memcpy(p->buf, buf, sz);
	}
    }
    p->owner_of_buffer = copy_buf;
}
	
void zebra_rec_keys_get_buf(zebra_rec_keys_t p, char **buf, size_t *sz)
{
    *buf = p->buf;
    *sz = p->buf_used;

    p->buf = 0;
    p->buf_max = 0;
    p->buf_used = 0;
}

void zebra_rec_keys_close(zebra_rec_keys_t p)
{
    if (!p)
	return;
    
    if (p->owner_of_buffer)
	xfree(p->buf);
    if (p->encode_handle)
	iscz1_stop(p->encode_handle);
    if (p->decode_handle)
	iscz1_stop(p->decode_handle);
    xfree(p);
}

void zebra_rec_keys_write(zebra_rec_keys_t keys, 
			  int reg_type,
			  const char *str, size_t slen,
			  const struct it_key *key)
{
    char *dst;
    const char *src = (char*) key;
    
    assert(keys->owner_of_buffer);

    if (keys->buf_used+1024 > keys->buf_max)
    {
        char *b = (char *) xmalloc (keys->buf_max += 128000);
        if (keys->buf_used > 0)
            memcpy (b, keys->buf, keys->buf_used);
        xfree (keys->buf);
        keys->buf = b;
    }
    dst = keys->buf + keys->buf_used;

    iscz1_encode(keys->encode_handle, &dst, &src);

#if REG_TYPE_PREFIX
    *dst++ = reg_type;
#endif
    memcpy (dst, str, slen);
    dst += slen;
    *dst++ = '\0';
    keys->buf_used = dst - keys->buf;
}

void zebra_rec_keys_reset(zebra_rec_keys_t keys)
{
    assert(keys);
    keys->buf_used = 0;
    
    iscz1_reset(keys->encode_handle);
}

int zebra_rec_keys_rewind(zebra_rec_keys_t keys)
{
    assert(keys);
    iscz1_reset(keys->decode_handle);
    keys->fetch_offset = 0;
    if (keys->buf_used == 0)
	return 0;
    return 1;
}

int zebra_rec_keys_empty(zebra_rec_keys_t keys)
{
    if (keys->buf_used == 0)
	return 1;
    return 0;
}

int zebra_rec_keys_read(zebra_rec_keys_t keys,
			const char **str, size_t *slen,
			struct it_key *key)
{
    assert(keys);
    if (keys->fetch_offset == keys->buf_used)
	return 0;
    else
    {
	const char *src = keys->buf + keys->fetch_offset;
	char *dst = (char*) key;
	
	assert (keys->fetch_offset < keys->buf_used);

	/* store the destination key */
	iscz1_decode(keys->decode_handle, &dst, &src);
	
	/* store pointer to string and length of it */
	*str = src;
	*slen = strlen(src);
	src += *slen + 1;
	
	keys->fetch_offset = src - keys->buf;
    }
    return 1;
}
