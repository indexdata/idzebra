/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lookupec.c,v $
 * Revision 1.1  1994-09-22 10:43:44  adam
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

int dict_look_ec (Dict dict, MatchInfo *mi, MatchWord *ri_base, int pos,
                  int (*userfunc)(Dict_char *), int range)
{
    Dict_ptr ptr = 1;
    int mid, lo, hi;
    void *p;
    short *indxp;
    char *info;
    MatchWord match_mask = 1<<(mi->m-1);

    dict_bf_readp (dict->dbf, ptr, &p);
    mid = lo = 0;
    hi = DICT_nodir(p)-1;
    indxp = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));    
    while (lo <= hi)
    {
        mid = lo;
        if (indxp[-mid] > 0)
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */
            MatchWord *ri = ri_base, sc;
            int i, j;
            info = (char*)p + indxp[-mid];
            for (j=0; ; j++)
            {
                Dict_char ch;

                memcpy (&ch, info+j*sizeof(Dict_char), sizeof(Dict_char));
                if (j && (ri[-1] & match_mask))
                {
                    if (ch == DICT_EOS)
                        (*userfunc)(info);
                    break;
                }
                if (j > mi->m+range-pos)
                    break;
                if (ch == DICT_EOS)
                    break;
                sc = mi->s[ch & 255];
                ri[1+range] = SH(ri[0]) & sc;
                for (i=1; i<=range; i++)
                    ri[i+1+range] = (SH(ri[i])&sc) | SH(ri[i-1])
                        | SH(ri[i+range]) | ri[i-1];
                ri += 1+range;
            }
        }
#if 0
        else
        {
            Dict_char dc;
            Dict_ptr subptr;

            /* Dict_ptr             subptr */
            /* Dict_char            sub char */
            /* unsigned char        length of information */
            /* char *               information */
            info = (char*)p - indxp[-mid];
            memcpy (&dc, info+sizeof(Dict_ptr), sizeof(Dict_char));
            cmp = dc- *str;
            if (!cmp)
            {
                memcpy (&subptr, info, sizeof(Dict_ptr));
                if (*++str == DICT_EOS)
                    return info+sizeof(Dict_ptr)+sizeof(Dict_char);
                else
                {
                    if (subptr == 0)
                        return NULL;
                    ptr = subptr;
                    dict_bf_readp (dict->dbf, ptr, &p);
                    mid = lo = 0;
                    hi = DICT_nodir(p)-1;
                    indxp = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));
                    continue;
                }
            }

        }
#endif
        lo++;
    }
    return 0;
}

static MatchInfo *prepare_match (Dict_char *pattern)
{
    int i;
    MatchWord *s;
    MatchInfo *mi;

    mi = xmalloc (sizeof(*mi));
    mi->m = dict_strlen (pattern);
    mi->s = s = xmalloc (sizeof(*s)*256);  /* 256 !!! */
    for (i=0; i<256; i++)
        s[i] = 0;
    for (i=0; pattern[i]; i++)
        s[pattern[i]&255] += 1<<i;
    return mi;
}

int dict_lookup_ec (Dict dict, Dict_char *pattern, int range,
                    int (*userfunc)(Dict_char *name))
{
    MatchInfo *mi;
    MatchWord *ri;
    int i;

    if (dict->head.last == 1)
        return 0;
    
    mi = prepare_match (pattern);
    ri = xmalloc ((dict_strlen(pattern)+range+2)*(range+1)*sizeof(*ri));
    for (i=0; i<=range; i++)
        ri[i] = (2<<i)-1;
    
    i = dict_look_ec (dict, mi, ri, 0, userfunc, range);
    xfree (ri);
    return i;
}




