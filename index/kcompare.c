/* $Id: kcompare.c,v 1.40 2002-08-02 19:26:55 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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

void key_logdump (int logmask, const void *p)
{
    struct it_key key;

    memcpy (&key, p, sizeof(key));
    logf (logmask, "%7d s=%-4d", key.sysno, key.seqno);
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

int key_get_pos (const void *p)
{
    struct it_key key;
    memcpy (&key, p, sizeof(key));
    return key.seqno;
}

struct iscz1_code_info {
    struct it_key key;
};

static void *iscz1_code_start (int mode)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *)
	xmalloc (sizeof(*p));
    p->key.sysno = 0;
    p->key.seqno = 0;
    return p;
}

static void iscz1_code_reset (void *vp)
{
    struct iscz1_code_info *p = (struct iscz1_code_info *) vp;
    p->key.sysno = 0;
    p->key.seqno = 0;
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
    if (c&32) /* expand sign bit to high bits */
       c = ((c | 63) << 8) + *(*src)++;
    else
       c = ((c & 63) << 8) + *(*src)++;
    c = (c << 8) + *(*src)++;
    c = (c << 8) + *(*src)++;
    
    return c;
}

static void iscz1_code_item (int mode, void *vp, char **dst, char **src)
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

ISAMS_M key_isams_m (Res res, ISAMS_M me)
{
    isams_getmethod (me);

    me->compare_item = key_compare;

    me->code_start = iscz1_code_start;
    me->code_item = iscz1_code_item;
    me->code_stop = iscz1_code_stop;

    me->debug = atoi(res_get_def (res, "isamsDebug", "0"));

    return me;
}

ISAMC_M key_isamc_m (Res res, ISAMC_M me)
{
    isc_getmethod (me);

    me->compare_item = key_compare;

    me->code_start = iscz1_code_start;
    me->code_item = iscz1_code_item;
    me->code_stop = iscz1_code_stop;
    me->code_reset = iscz1_code_reset;

    me->debug = atoi(res_get_def (res, "isamcDebug", "0"));

    return me;
}

ISAMD_M key_isamd_m (Res res,ISAMD_M me)
{

    me = isamd_getmethod (me);

    me->compare_item = key_compare;

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

/* 
 * $Log: kcompare.c,v $
 * Revision 1.40  2002-08-02 19:26:55  adam
 * Towards GPL
 *
 * Revision 1.39  2002/04/12 14:55:22  adam
 * key_print_it
 *
 * Revision 1.38  2002/04/05 08:46:26  adam
 * Zebra with full functionality
 *
 * Revision 1.37  2001/11/19 23:08:30  adam
 * Added const qualifier for name parameter of key_SU_decode.
 *
 * Revision 1.36  2001/10/15 19:53:43  adam
 * POSIX thread updates. First work on term sets.
 *
 * Revision 1.35  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.34  1999/07/14 13:21:34  heikki
 * Added isam-d files. Compiles (almost) clean. Doesn't work at all
 *
 * Revision 1.33  1999/07/14 10:59:26  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.32  1999/07/13 13:21:15  heikki
 * Managing negative deltas
 *
 * Revision 1.31  1999/07/06 09:37:04  heikki
 * Working on isamh - not ready yet.
 *
 * Revision 1.30  1999/06/30 15:07:23  heikki
 * Adding isamh stuff
 *
 * Revision 1.29  1999/06/30 09:08:23  adam
 * Added coder to reset.
 *
 * Revision 1.28  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.27  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.26  1999/02/02 14:50:54  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.25  1998/06/08 15:26:06  adam
 * Minor changes.
 *
 * Revision 1.24  1998/06/08 14:43:12  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.23  1998/03/05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.22  1997/09/22 12:39:06  adam
 * Added get_pos method for the ranked result sets.
 *
 * Revision 1.21  1997/09/17 12:19:13  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.20  1996/12/23 15:30:44  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.19  1996/12/11 12:08:00  adam
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
