/* $Id: kcompare.c,v 1.48 2004-08-04 09:00:00 adam Exp $
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "index.h"

#if IT_KEY_NEW
#define INT_CODEC_NEW 1
#else
#define INT_CODEC_NEW 0
#endif

#define CODEC_INLINE inline
void key_logdump_txt (int logmask, const void *p, const char *txt)
{
    struct it_key key;
    if (p)
    {
#if IT_KEY_NEW
	char formstr[128];
	int i;

        memcpy (&key, p, sizeof(key));
	assert(key.len > 0 && key.len <= IT_KEY_LEVEL_MAX);
	*formstr = '\0';
	for (i = 0; i<key.len; i++)
	{
	    sprintf(formstr + strlen(formstr), ZINT_FORMAT, key.mem[i]);
	    if (i)
		strcat(formstr, " ");
	}
        logf (logmask, "%s %s", formstr, txt);
#else
/* !IT_KEY_NEW */
        memcpy (&key, p, sizeof(key));
        logf (logmask, "%7d:%-4d %s", key.sysno, key.seqno, txt);
#endif
    }
    else
        logf(logmask, " (null) %s",txt);
}

void key_logdump (int logmask, const void *p)
{
    key_logdump_txt(logmask,  p, "");
}

int key_compare_it (const void *p1, const void *p2)
{
#if IT_KEY_NEW
    int i, l = ((struct it_key *) p1)->len;
    if (((struct it_key *) p2)->len > l)
	l = ((struct it_key *) p2)->len;
    assert (l <= 4 && l > 0);
    for (i = 0; i < l; i++)
    {
	if (((struct it_key *) p1)->mem[i] != ((struct it_key *) p2)->mem[i])
	{
	    if (((struct it_key *) p1)->mem[i] > ((struct it_key *) p2)->mem[i])
		return l-i;
	    else
		return i-l;
	}
    }
#else
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
#endif
    return 0;
}

char *key_print_it (const void *p, char *buf)
{
#if IT_KEY_NEW
    strcpy(buf, "");
#else
    const struct it_key *i = p;
    sprintf (buf, "%d:%d", i->sysno, i->seqno);
#endif
    return buf;
}

int key_compare (const void *p1, const void *p2)
{
    struct it_key i1, i2;
#if IT_KEY_NEW
    int i, l;
#endif
    memcpy (&i1, p1, sizeof(i1));
    memcpy (&i2, p2, sizeof(i2));
#if IT_KEY_NEW
    l = i1.len;
    if (i2.len > l)
	l = i2.len;
    assert (l <= 4 && l > 0);
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
#else
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
#endif
    return 0;
}

int key_get_seq(const void *p)
{
    struct it_key k;
    memcpy (&k, p, sizeof(k));
#if IT_KEY_NEW
    return k.mem[k.len-1];
#else
    return k.seqno;
#endif
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

#if IT_KEY_NEW
void key_init(struct it_key *key)
{
    int i;
    key->len = 0;
    for (i = 0; i<IT_KEY_LEVEL_MAX; i++)
	key->mem[i] = 0;
}
#endif

void iscz1_reset (void *vp)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
#if IT_KEY_NEW
    int i;
    p->key.len = 0;
    for (i = 0; i< IT_KEY_LEVEL_MAX; i++)
	p->key.mem[i] = 0;
#else
    p->key.sysno = 0;
    p->key.seqno = 0;
#endif
}

void iscz1_stop (void *p)
{
    xfree (p);
}

#if INT_CODEC_NEW
/* small encoder that works with unsigneds of any length */
static CODEC_INLINE void iscz1_encode_int (zint d, char **dst)
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
#else
/* ! INT_CODEC_NEW */

/* old encoder that works with unsigneds up to 30 bits */
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

/* old decoder that works with unsigneds up to 30 bits */
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

void iscz1_encode (void *vp, char **dst, const char **src)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
    struct it_key tkey;
#if IT_KEY_NEW
    zint d;
    int i;
#else
    int d;
#endif

    /*   1
	 3, 2, 9, 12
	 3, 2, 10, 2
	 4, 1
	 
	 if diff is 0, then there is more ...
	 if diff is non-zero, then _may_ be more
    */
    memcpy (&tkey, *src, sizeof(struct it_key));
#if IT_KEY_NEW
    /* deal with leader + delta encoding .. */
    d = 0;
    assert(tkey.len > 0 && tkey.len <= 4);
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
#else
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
#endif
}

void iscz1_decode (void *vp, char **dst, const char **src)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
#if IT_KEY_NEW
    int i;
#else
    int d;
#endif
    
#if IT_KEY_NEW
    int leader = iscz1_decode_int ((unsigned char **) src);
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
#else
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
#endif
}

ISAMS_M *key_isams_m (Res res, ISAMS_M *me)
{
    isams_getmethod (me);

    me->compare_item = key_compare;
    me->log_item = key_logdump_txt;

    me->codec.start = iscz1_start;
    me->codec.decode = iscz1_decode;
    me->codec.encode = iscz1_encode;
    me->codec.stop = iscz1_stop;
    me->codec.reset = iscz1_reset;

    me->debug = atoi(res_get_def (res, "isamsDebug", "0"));

    return me;
}

ISAMC_M *key_isamc_m (Res res, ISAMC_M *me)
{
    isc_getmethod (me);

    me->compare_item = key_compare;
    me->log_item = key_logdump_txt;

    me->codec.start = iscz1_start;
    me->codec.decode = iscz1_decode;
    me->codec.encode = iscz1_encode;
    me->codec.stop = iscz1_stop;
    me->codec.reset = iscz1_reset;

    me->debug = atoi(res_get_def (res, "isamcDebug", "0"));

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

