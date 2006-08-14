/* $Id: kcompare.c,v 1.46.2.3 2006-08-14 10:38:58 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "index.h"

#define INT_CODEC_NEW 0
#ifdef __GNUC__
#define CODEC_INLINE inline
#else
#define CODEC_INLINE
#endif

void key_logdump_txt (int logmask, const void *p, const char *txt)
{
    struct it_key key;
    if (p)
    {
        memcpy (&key, p, sizeof(key));
        logf (logmask, "%7d:%-4d %s", key.sysno, key.seqno,txt);
    }
    else
        logf(logmask, " (null) %s",txt);
}

void key_logdump (int logmask, const void *p)
{
    key_logdump_txt(logmask,p,"");
}

int key_compare_it (const void *p1, const void *p2)
{
    if (((struct it_key *) p1)->sysno != ((struct it_key *) p2)->sysno)
    {
        if (((struct it_key *) p1)->sysno > ((struct it_key *) p2)->sysno)
            return 2;
        else
            return -2;
    }
    if (((struct it_key *) p1)->seqno != ((struct it_key *) p2)->seqno)
    {
        if (((struct it_key *) p1)->seqno > ((struct it_key *) p2)->seqno)
            return 1;
        else
            return -1;
    }
    return 0;
}

char *key_print_it (const void *p, char *buf)
{
    const struct it_key *i = p;
    sprintf (buf, "%d:%d", i->sysno, i->seqno);
    return buf;
}

int key_compare (const void *p1, const void *p2)
{
    struct it_key i1, i2;
    memcpy (&i1, p1, sizeof(i1));
    memcpy (&i2, p2, sizeof(i2));
    if (i1.sysno != i2.sysno)
    {
        if (i1.sysno > i2.sysno)
            return 2;
        else
            return -2;
    }
    if (i1.seqno != i2.seqno)
    {
        if (i1.seqno > i2.seqno)
            return 1;
        else
            return -1;
    }
    return 0;
}

int key_get_seq(const void *p)
{
    struct it_key k;
    memcpy (&k, p, sizeof(k));
    return k.seqno;
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

void *iscz1_code_start (int mode)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *)
	xmalloc (sizeof(*p));
    p->key.sysno = 0;
    p->key.seqno = 0;
    return p;
}

void iscz1_code_reset (void *vp)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
    p->key.sysno = 0;
    p->key.seqno = 0;
}

void iscz1_code_stop (int mode, void *p)
{
    xfree (p);
}

#if INT_CODEC_NEW 
static CODEC_INLINE void iscz1_encode_int (unsigned d, char **dst)
{
    unsigned char *bp = (unsigned char*) *dst;

    while (d > 127)
    {
        *bp++ = 128 | (d & 127);
	d = d >> 7;
    }
    *bp++ = d;
    *dst = (char *) bp;
}

static CODEC_INLINE int iscz1_decode_int (unsigned char **src)
{
    unsigned d = 0;
    unsigned char c;
    unsigned r = 0;

    while (((c = *(*src)++) & 128))
    {
        d += ((c&127) << r);
	r += 7;
    }
    d += (c << r);
    return d;
}
#else
/* ! INT_CODEC_NEW */

static CODEC_INLINE void iscz1_encode_int (unsigned d, char **dst)
{
    unsigned char *bp = (unsigned char*) *dst;

    if (d <= 63)
        *bp++ = d;
    else if (d <= 16383)
    {
        *bp++ = 64 | (d>>8);
       *bp++ = d & 255;
    }
    else if (d <= 4194303)
    {
        *bp++ = 128 | (d>>16);
        *bp++ = (d>>8) & 255;
        *bp++ = d & 255;
    }
    else
    {
        *bp++ = 192 | (d>>24);
        *bp++ = (d>>16) & 255;
        *bp++ = (d>>8) & 255;
        *bp++ = d & 255;
    }
    *dst = (char *) bp;
}

static CODEC_INLINE int iscz1_decode_int (unsigned char **src)
{
    unsigned c = *(*src)++;
    switch (c & 192)
    {
    case 0:
        return c;
    case 64:
        return ((c & 63) << 8) + *(*src)++;
    case 128:
        c = ((c & 63) << 8) + *(*src)++;
        c = (c << 8) + *(*src)++;
        return c;
    }
    if (c&32) /* expand sign bit to high bits */
       c = ((c | 63) << 8) + *(*src)++;
    else
       c = ((c & 63) << 8) + *(*src)++;
    c = (c << 8) + *(*src)++;
    c = (c << 8) + *(*src)++;
    
    return c;
}
#endif

void iscz1_code_item (int mode, void *vp, char **dst, char **src)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
    struct it_key tkey;
    int d;

    if (mode == ISAMC_ENCODE)
    {
        memcpy (&tkey, *src, sizeof(struct it_key));
        d = tkey.sysno - p->key.sysno;
        if (d)
        {
            iscz1_encode_int (2*tkey.seqno + 1, dst);
            iscz1_encode_int (d, dst);
            p->key.sysno += d;
            p->key.seqno = tkey.seqno;
        }
        else
        {
            iscz1_encode_int (2*(tkey.seqno - p->key.seqno), dst);
            p->key.seqno = tkey.seqno;
        }
        (*src) += sizeof(struct it_key);
    }
    else
    {
        d = iscz1_decode_int ((unsigned char **) src);
        if (d & 1)
        {
            p->key.seqno = d>>1;
            p->key.sysno += iscz1_decode_int ((unsigned char **) src);
        }
        else
            p->key.seqno += d>>1;
        memcpy (*dst, &p->key, sizeof(struct it_key));
        (*dst) += sizeof(struct it_key);
    }
}

ISAMS_M *key_isams_m (Res res, ISAMS_M *me)
{
    isams_getmethod (me);

    me->compare_item = key_compare;
    me->log_item = key_logdump_txt;

    me->code_start = iscz1_code_start;
    me->code_item = iscz1_code_item;
    me->code_stop = iscz1_code_stop;

    me->debug = atoi(res_get_def (res, "isamsDebug", "0"));

    return me;
}

ISAMC_M *key_isamc_m (Res res, ISAMC_M *me)
{
    isc_getmethod (me);

    me->compare_item = key_compare;
    me->log_item = key_logdump_txt;

    me->code_start = iscz1_code_start;
    me->code_item = iscz1_code_item;
    me->code_stop = iscz1_code_stop;
    me->code_reset = iscz1_code_reset;

    me->debug = atoi(res_get_def (res, "isamcDebug", "0"));

    return me;
}

ISAMD_M *key_isamd_m (Res res, ISAMD_M *me)
{
    me = isamd_getmethod (me);

    me->compare_item = key_compare;
    me->log_item = key_logdump_txt;

    me->code_start = iscz1_code_start;
    me->code_item = iscz1_code_item;
    me->code_stop = iscz1_code_stop;
    me->code_reset = iscz1_code_reset;

    me->debug = atoi(res_get_def (res, "isamdDebug", "0"));

    return me;
}

int key_SU_encode (int ch, char *out)
{
    int i;
    for (i = 0; ch; i++)
    {
	if (ch >= 64)
	    out[i] = 65 + (ch & 63);
	else
	    out[i] = 1 + ch;
	ch = ch >> 6;
    }
    return i;
    /* in   out
       0     1
       1     2
       63    64
       64    65, 2
       65    66, 2
       127   128, 2
       128   65, 3
       191   128, 3
       192   65, 4
    */
}

int key_SU_decode (int *ch, const unsigned char *out)
{
    int len = 1;
    int fact = 1;
    *ch = 0;
    for (len = 1; *out >= 65; len++, out++)
    {
	*ch += (*out - 65) * fact;
	fact <<= 6;
    }
    *ch += (*out - 1) * fact;
    return len;
}

