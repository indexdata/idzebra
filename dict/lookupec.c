/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lookupec.c,v $
 * Revision 1.9  1999-05-26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.8  1999/05/15 14:36:37  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.7  1999/02/02 14:50:26  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.6  1996/02/02 13:43:51  adam
 * The public functions simply use char instead of Dict_char to represent
 * search strings. Dict_char is used internally only.
 *
 * Revision 1.5  1995/01/24  16:01:03  adam
 * Added -ansi to CFLAGS.
 * Use new API of dfa module.
 *
 * Revision 1.4  1994/10/05  12:16:51  adam
 * Pagesize is a resource now.
 *
 * Revision 1.3  1994/09/26  16:31:06  adam
 * Minor changes.
 *
 * Revision 1.2  1994/09/22  14:43:57  adam
 * First functional version of lookup with error correction. A 'range'
 * specified the maximum number of insertions+deletions+substitutions.
 *
 * Revision 1.1  1994/09/22  10:43:44  adam
 * Two versions of depend. Type 1 is the tail-type compatible with
 * all make programs. Type 2 is the GNU make with include facility.
 * Type 2 is default. depend rule chooses current rule.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

typedef unsigned MatchWord;

typedef struct {
    MatchWord *s;
    int m;
} MatchInfo;

#define SH(x) (((x)<<1)+1)

int dict_look_ec (Dict dict, Dict_ptr ptr, MatchInfo *mi, MatchWord *ri_base,
                  int pos, int (*userfunc)(char *), int range,
                  Dict_char *prefix)
{
    int lo, hi;
    void *p;
    short *indxp;
    char *info;
    MatchWord match_mask = 1<<(mi->m-1);

    dict_bf_readp (dict->dbf, ptr, &p);
    lo = 0;
    hi = DICT_nodir(p)-1;
    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));    
    while (lo <= hi)
    {
        if (indxp[-lo] > 0)
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */
            MatchWord *ri = ri_base, sc;
            int i, j;
            info = (char*)p + indxp[-lo];
            for (j=0; ; j++)
            {
                Dict_char ch;

                memcpy (&ch, info+j*sizeof(Dict_char), sizeof(Dict_char));
                prefix[pos+j] = ch;
                if (ch == DICT_EOS)
                {
                    if (ri[range] & match_mask)
                        (*userfunc)((char*) prefix);
                    break;
                }
                if (j+pos >= mi->m+range)
                    break;
                sc = mi->s[ch & 255];
                ri[1+range] = SH(ri[0]) & sc;
                for (i=1; i<=range; i++)
                    ri[i+1+range] = (SH(ri[i]) & sc) | SH(ri[i-1])
                        | SH(ri[i+range]) | ri[i-1];
                ri += 1+range;
                if (!(ri[range] & (1<<(pos+j))))
                    break;
            }
        }
        else
        {
            Dict_char ch;
            MatchWord *ri = ri_base, sc;
            int i;

            /* Dict_ptr             subptr */
            /* Dict_char            sub char */
            /* unsigned char        length of information */
            /* char *               information */
            info = (char*)p - indxp[-lo];
            memcpy (&ch, info+sizeof(Dict_ptr), sizeof(Dict_char));
            prefix[pos] = ch;
            
            sc = mi->s[ch & 255];
            ri[1+range] = SH(ri[0]) & sc;
            for (i=1; i<=range; i++)
                ri[i+1+range] = (SH(ri[i]) & sc) | SH(ri[i-1])
                    | SH(ri[i+range]) | ri[i-1];
            ri += 1+range;
            if (ri[range] & (1<<pos))
            {
                Dict_ptr subptr;
                if (info[sizeof(Dict_ptr)+sizeof(Dict_char)] &&
                    (ri[range] & match_mask))
                {
                    prefix[pos+1] = DICT_EOS;
                    (*userfunc)((char*) prefix);
                }
                memcpy (&subptr, info, sizeof(Dict_ptr));
                if (subptr)
                {
                    dict_look_ec (dict, subptr, mi, ri, pos+1,
                                  userfunc, range, prefix);
                    dict_bf_readp (dict->dbf, ptr, &p);
                    indxp = (short*) ((char*) p + 
                                      DICT_bsize(p)-sizeof(short));
                }
            }
        }
        lo++;
    }
    return 0;
}

static MatchInfo *prepare_match (Dict_char *pattern)
{
    int i;
    MatchWord *s;
    MatchInfo *mi;

    mi = (MatchInfo *) xmalloc (sizeof(*mi));
    mi->m = dict_strlen (pattern);
    mi->s = s = (MatchWord *) xmalloc (sizeof(*s)*256);  /* 256 !!! */
    for (i=0; i<256; i++)
        s[i] = 0;
    for (i=0; pattern[i]; i++)
        s[pattern[i]&255] += 1<<i;
    return mi;
}

int dict_lookup_ec (Dict dict, char *pattern, int range,
                    int (*userfunc)(char *name))
{
    MatchInfo *mi;
    MatchWord *ri;
    int i;
    Dict_char prefix[2048];

    if (!dict->head.root)
        return 0;
    
    mi = prepare_match ((Dict_char*) pattern);

    ri = (MatchWord *) xmalloc ((dict_strlen((Dict_char*) pattern)+range+2)
				* (range+1)*sizeof(*ri));
    for (i=0; i<=range; i++)
        ri[i] = (2<<i)-1;
    
    i = dict_look_ec (dict, dict->head.root, mi, ri, 0, userfunc,
		      range, prefix);
    xfree (ri);
    return i;
}

