/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kcompare.c,v $
 * Revision 1.5  1995-09-11 13:09:34  adam
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
#include <ctype.h>
#include <assert.h>

#include "index.h"

void key_logdump (int logmask, const void *p)
{
    struct it_key key;

    memcpy (&key, p, sizeof(key));
#if IT_KEY_HAVE_SEQNO
    logf (logmask, "%7d s=%-3d", key.sysno, key.seqno);
#else
    logf (logmask, "%7d f=%-3d", key.sysno, key.freq);
#endif
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
#if IT_KEY_HAVE_FIELD
    if (i1.field != i2.field)
    {
        if (i1.field > i2.field)
            return 1;
        else
            return -1;
    }
#endif
    return 0;
}

int index_char_cvt (int c)
{
    return tolower (c);
}
