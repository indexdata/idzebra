/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kcompare.c,v $
 * Revision 1.19  1996-12-11 12:08:00  adam
 * Added better compression.
 *
 * Revision 1.18  1996/10/29 14:09:44  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.17  1996/06/04 10:18:58  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.16  1996/05/13  14:23:05  adam
 * Work on compaction of set/use bytes in dictionary.
 *
 * Revision 1.15  1995/11/20  16:59:46  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.14  1995/10/30  15:08:08  adam
 * Bug fixes.
 *
 * Revision 1.13  1995/10/27  14:00:11  adam
 * Implemented detection of database availability.
 *
 * Revision 1.12  1995/10/17  18:02:08  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.11  1995/10/06  16:33:37  adam
 * Use attribute mappings.
 *
 * Revision 1.10  1995/09/29  14:01:41  adam
 * Bug fixes.
 *
 * Revision 1.9  1995/09/28  12:10:32  adam
 * Bug fixes. Field prefix used in queries.
 *
 * Revision 1.8  1995/09/28  09:19:42  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.7  1995/09/27  12:22:28  adam
 * More work on extract in record control.
 * Field name is not in isam keys but in prefix in dictionary words.
 *
 * Revision 1.6  1995/09/14  07:48:23  adam
 * Record control management.
 *
 * Revision 1.5  1995/09/11  13:09:34  adam
 * More work on relevance feedback.
 *
 * Revision 1.4  1995/09/08  14:52:27  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.3  1995/09/07  13:58:36  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 * Result-set references.
 *
 * Revision 1.2  1995/09/06  16:11:17  adam
 * Option: only one word key per file.
 *
 * Revision 1.1  1995/09/04  09:10:36  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "index.h"

void key_logdump (int logmask, const void *p)
{
    struct it_key key;

    memcpy (&key, p, sizeof(key));
    logf (logmask, "%7d s=%-4d", key.sysno, key.seqno);
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
#if IT_KEY_HAVE_SEQNO
    if (i1.seqno != i2.seqno)
    {
        if (i1.seqno > i2.seqno)
            return 1;
        else
            return -1;
    }
#else
    if (i1.freq != i2.freq)
    {
        if (i1.freq > i2.freq)
            return 1;
        else
            return -1;
    }
#endif
    return 0;
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

static void *iscz1_code_start (int mode)
{
    struct iscz1_code_info *p = xmalloc (sizeof(*p));
    p->key.sysno = 0;
    p->key.seqno = 0;
    return p;
}

static void iscz1_code_stop (int mode, void *p)
{
    xfree (p);
}

void iscz1_encode_int (unsigned d, char **dst)
{
    unsigned char *bp = (unsigned char*) *dst;

    if (d <= 63)
        *bp++ = d;
    else if (d <= 16383)
    {
        *bp++ = 64 + (d>>8);
       *bp++ = d & 255;
    }
    else if (d <= 4194303)
    {
        *bp++ = 128 + (d>>16);
        *bp++ = (d>>8) & 255;
        *bp++ = d & 255;
    }
    else
    {
        *bp++ = 192 + (d>>24);
        *bp++ = (d>>16) & 255;
        *bp++ = (d>>8) & 255;
        *bp++ = d & 255;
    }
    *dst = (char *) bp;
}

int iscz1_decode_int (unsigned char **src)
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
    c = ((c & 63) << 8) + *(*src)++;
    c = (c << 8) + *(*src)++;
    c = (c << 8) + *(*src)++;
    return c;
}
#if 1
static void iscz1_code_item (int mode, void *vp, char **dst, char **src)
{
    struct iscz1_code_info *p = vp;
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
#else
static void iscz1_code_item (int mode, void *vp, char **dst, char **src)
{
    struct iscz1_code_info *p = vp;
    struct it_key tkey;
    int d;

    if (mode == ISAMC_ENCODE)
    {
        memcpy (&tkey, *src, sizeof(struct it_key));
        d = tkey.sysno - p->key.sysno;
        iscz1_encode_int (d, dst);
        if (d)
        {
            p->key.sysno = tkey.sysno;
            p->key.seqno = 0;
        }
        iscz1_encode_int (tkey.seqno - p->key.seqno, dst);
        p->key.seqno = tkey.seqno;
        (*src) += sizeof(struct it_key);
    }
    else
    {
        d = iscz1_decode_int ((unsigned char **) src);
        if (d)
        {
            p->key.sysno += d;
            p->key.seqno = 0;
        }
        d = iscz1_decode_int ((unsigned char **) src);
        p->key.seqno += d;
        memcpy (*dst, &p->key, sizeof(struct it_key));
        (*dst) += sizeof(struct it_key);
    }
}
#endif

ISAMC_M key_isamc_m (void)
{
    static ISAMC_M me = NULL;

    if (me)
        return me;

    me = isc_getmethod ();

    me->compare_item = key_compare;

    me->code_start = iscz1_code_start;
    me->code_item = iscz1_code_item;
    me->code_stop = iscz1_code_stop;

    me->debug = atoi(res_get_def (common_resource, "isamcDebug", "0"));

    logf (LOG_LOG, "ISAMC system active");
    return me;
}

