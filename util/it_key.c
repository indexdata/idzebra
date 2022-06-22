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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <yaz/xmalloc.h>
#include <it_key.h>

#ifdef __GNUC__
#define CODEC_INLINE inline
#else
#define CODEC_INLINE
#endif

void key_logdump_txt(int logmask, const void *p, const char *txt)
{
    struct it_key key;
    if (!txt)
        txt = "(none)";
    if (p)
    {
        char formstr[128];
        int i;

        memcpy (&key, p, sizeof(key));
        assert(key.len > 0 && key.len <= IT_KEY_LEVEL_MAX);
        *formstr = '\0';
        for (i = 0; i<key.len; i++)
        {
            if (i)
                strcat(formstr, ".");
            sprintf(formstr + strlen(formstr), ZINT_FORMAT, key.mem[i]);
        }
        yaz_log(logmask, "%s %s", formstr, txt);
    }
    else
        yaz_log(logmask, " (no key) %s",txt);
}

void key_logdump(int logmask, const void *p)
{
    key_logdump_txt(logmask,  p, "");
}

char *key_print_it (const void *p, char *buf)
{
    strcpy(buf, "");
    return buf;
}

int key_compare (const void *p1, const void *p2)
{
    struct it_key i1, i2;
    int i, l;
    memcpy (&i1, p1, sizeof(i1));
    memcpy (&i2, p2, sizeof(i2));
    l = i1.len;
    if (i2.len > l)
        l = i2.len;
    assert (l <= IT_KEY_LEVEL_MAX && l > 0);
    for (i = 0; i < l; i++)
    {
        if (i1.mem[i] != i2.mem[i])
        {
            if (i1.mem[i] > i2.mem[i])
                return l-i;
            else
                return i-l;
        }
    }
    return 0;
}

zint key_get_seq(const void *p)
{
    struct it_key k;
    memcpy (&k, p, sizeof(k));
    return k.mem[k.len-1];
}

zint key_get_segment(const void *p)
{
    struct it_key k;
    memcpy (&k, p, sizeof(k));
    return k.mem[k.len-2];
}

int key_qsort_compare (const void *p1, const void *p2)
{
    int r;
    size_t l;
    char *cp1 = *(char **) p1;
    char *cp2 = *(char **) p2;

    if ((r = strcmp (cp1, cp2)))
        return r;
    l = strlen(cp1)+1;
    if ((r = key_compare (cp1+l+1, cp2+l+1)))
        return r;
    return cp1[l] - cp2[l];
}

struct iscz1_code_info {
    struct it_key key;
};

void *iscz1_start (void)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *)
        xmalloc (sizeof(*p));
    iscz1_reset(p);
    return p;
}

void key_init(struct it_key *key)
{
    int i;
    key->len = 0;
    for (i = 0; i < IT_KEY_LEVEL_MAX; i++)
        key->mem[i] = 0;
}

void iscz1_reset (void *vp)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
    int i;
    p->key.len = 0;
    for (i = 0; i < IT_KEY_LEVEL_MAX; i++)
        p->key.mem[i] = 0;
}

void iscz1_stop (void *p)
{
    xfree (p);
}

/* small encoder that works with unsigneds of any length */
static CODEC_INLINE void iscz1_encode_int (zint d, char **dst)
{
    unsigned char *bp = (unsigned char*) *dst;

    while (d > 127)
    {
        *bp++ = (unsigned) (128 | (d & 127));
        d = d >> 7;
    }
    *bp++ = (unsigned) d;
    *dst = (char *) bp;
}

/* small decoder that works with unsigneds of any length */
static CODEC_INLINE zint iscz1_decode_int (unsigned char **src)
{
    zint d = 0;
    unsigned char c;
    unsigned r = 0;

    while (((c = *(*src)++) & 128))
    {
        d += ((zint) (c&127) << r);
        r += 7;
    }
    d += ((zint) c << r);
    return d;
}

void iscz1_encode (void *vp, char **dst, const char **src)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
    struct it_key tkey;
    zint d;
    int i;

    /*   1
         3, 2, 9, 12
         3, 2, 10, 2
         4, 1

         if diff is 0, then there is more ...
         if diff is non-zero, then _may_ be more
    */
    memcpy (&tkey, *src, sizeof(struct it_key));

    /* deal with leader + delta encoding .. */
    d = 0;
    assert(tkey.len > 0 && tkey.len <= IT_KEY_LEVEL_MAX);
    for (i = 0; i < tkey.len; i++)
    {
        d = tkey.mem[i] - p->key.mem[i];
        if (d || i == tkey.len-1)
        {  /* all have been equal until now, now make delta .. */
            p->key.mem[i] = tkey.mem[i];
            if (d > 0)
            {
                iscz1_encode_int (i + (tkey.len << 3) + 64, dst);
                i++;
                iscz1_encode_int (d, dst);
            }
            else
            {
                iscz1_encode_int (i + (tkey.len << 3), dst);
                }
            break;
        }
    }
    /* rest uses absolute encoding ... */
    for (; i < tkey.len; i++)
    {
        iscz1_encode_int (tkey.mem[i], dst);
        p->key.mem[i] = tkey.mem[i];
    }
    (*src) += sizeof(struct it_key);
}

void iscz1_decode (void *vp, char **dst, const char **src)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
    int i;

    int leader = (int) iscz1_decode_int ((unsigned char **) src);
    i = leader & 7;
    if (leader & 64)
        p->key.mem[i] += iscz1_decode_int ((unsigned char **) src);
    else
        p->key.mem[i] = iscz1_decode_int ((unsigned char **) src);
    p->key.len = (leader >> 3) & 7;
    while (++i < p->key.len)
        p->key.mem[i] = iscz1_decode_int ((unsigned char **) src);
    memcpy (*dst, &p->key, sizeof(struct it_key));
    (*dst) += sizeof(struct it_key);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

