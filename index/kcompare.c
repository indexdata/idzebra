/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kcompare.c,v $
 * Revision 1.11  1995-10-06 16:33:37  adam
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
#include <ctype.h>
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
    const struct it_key *i1 = p1, *i2 = p2;
    if (i1->sysno != i2->sysno)
    {
        if (i1->sysno > i2->sysno)
            return 2;
        else
            return -2;
    }
#if IT_KEY_HAVE_SEQNO
    if (i1->seqno != i2->seqno)
    {
        if (i1->seqno > i2->seqno)
            return 1;
        else
            return -1;
    }
#else
    if (i1->freq != i2->freq)
    {
        if (i1->freq > i2->freq)
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

int index_char_cvt (int c)
{
    return tolower (c);
}

int index_word_prefix (char *string, int attset_ordinal,
                           int local_attribute)
{
    sprintf (string, "%c%04d", attset_ordinal + '0', local_attribute);
    return 5;
}

