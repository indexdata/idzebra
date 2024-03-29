/* This file is part of the Zebra server.
   Copyright (C) Index Data

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "reckeys.h"
#include <yaz/nmem.h>
#include <yaz/xmalloc.h>

struct zebra_rec_key_entry {
    char *buf;
    size_t len;
    struct it_key key;
    struct zebra_rec_key_entry *next;
};

struct zebra_rec_keys_t_ {
    size_t buf_used;
    size_t buf_max;
    size_t fetch_offset;
    char *buf;
    void *encode_handle;
    void *decode_handle;
    char owner_of_buffer;
    zint custom_record_id;

    NMEM nmem;
    size_t hash_size;
    struct zebra_rec_key_entry **entries;
};


struct zebra_rec_key_entry **zebra_rec_keys_mk_hash(zebra_rec_keys_t p,
                                                    const char *buf,
                                                    size_t len,
                                                    const struct it_key *key)
{
    unsigned h = 0;
    size_t i;
    int j;
#if 0
    h = key->mem[key->len-1];
#else
    for (i = 0; i<len; i++)
        h = h * 65509 + buf[i];
    for (j = 0; j<key->len; j++)
        h = h * 65509 + CAST_ZINT_TO_INT(key->mem[j]);
#endif
    return &p->entries[h % (unsigned) p->hash_size];
}

static void init_hash(zebra_rec_keys_t p)
{
    p->entries = 0;
    nmem_reset(p->nmem);
    if (p->hash_size)
    {
        size_t i;
        p->entries = nmem_malloc(p->nmem, p->hash_size * sizeof(*p->entries));
        for (i = 0; i<p->hash_size; i++)
            p->entries[i] = 0;
    }
}

zebra_rec_keys_t zebra_rec_keys_open(void)
{
    zebra_rec_keys_t p = xmalloc(sizeof(*p));
    p->buf_used = 0;
    p->buf_max = 0;
    p->fetch_offset = 0;
    p->buf = 0;
    p->owner_of_buffer = 1;
    p->encode_handle = iscz1_start();
    p->decode_handle = iscz1_start();

    p->custom_record_id = 0;
    p->nmem = nmem_create();
    p->hash_size = 32767;
    p->entries = 0;

    init_hash(p);

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
    nmem_destroy(p->nmem);
    xfree(p);
}

int zebra_rec_keys_add_hash(zebra_rec_keys_t keys,
                            const char *str, size_t slen,
                            const struct it_key *key)
{
    struct zebra_rec_key_entry **kep_first
        = zebra_rec_keys_mk_hash(keys, str, slen, key);
    struct zebra_rec_key_entry **kep = kep_first;
    while (*kep)
    {
        struct zebra_rec_key_entry *e = *kep;
        if (slen == e->len && !memcmp(str, e->buf, slen) &&
            !key_compare(key, &e->key))
        {
            *kep = (*kep)->next; /* out of queue */
            e->next = *kep_first; /* move to front */
            *kep_first = e;

            return 0;
        }
        kep = &(*kep)->next;
    }
    *kep = nmem_malloc(keys->nmem, sizeof(**kep));
    (*kep)->next = 0;
    (*kep)->len = slen;
    memcpy(&(*kep)->key, key, sizeof(*key));
    (*kep)->buf = nmem_malloc(keys->nmem, slen);
    memcpy((*kep)->buf, str, slen);
    return 1;
}

void zebra_rec_keys_write(zebra_rec_keys_t keys,
                          const char *str, size_t slen,
                          const struct it_key *key)
{
    char *dst;
    const char *src = (char*) key;

    assert(keys->owner_of_buffer);

    if (key->mem[1]) /* record_id custom */
    {
        keys->custom_record_id = key->mem[1];
    }
#if 1
    if (!zebra_rec_keys_add_hash(keys, str, slen, key))
    {
#if 0
        yaz_log(YLOG_LOG, "dup key slen=%d %.*s "
                "ord=" ZINT_FORMAT " seq=" ZINT_FORMAT,
                slen, slen, str, key->mem[0], key->mem[key->len-1]);
#endif
        return;  /* key already there . Omit it */
    }
#endif
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

    init_hash(keys);
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

zint zebra_rec_keys_get_custom_record_id(zebra_rec_keys_t keys)
{
    return keys->custom_record_id;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

