/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include "dict-p.h"

typedef unsigned MatchWord;

typedef struct {
    MatchWord *s;
    int m;
} MatchInfo;

#define SH(x) (((x)<<1)+1)

static int lookup_ec(Dict dict, Dict_ptr ptr,
                     MatchInfo *mi, MatchWord *ri_base,
                     int pos, int (*userfunc)(char *), int range,
                     Dict_char *prefix)
{
    int lo, hi;
    void *p;
    short *indxp;
    char *info;
    MatchWord match_mask = 1<<(mi->m-1);

    dict_bf_readp(dict->dbf, ptr, &p);
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

                memcpy(&ch, info+j*sizeof(Dict_char), sizeof(Dict_char));
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
            memcpy(&ch, info+sizeof(Dict_ptr), sizeof(Dict_char));
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
                memcpy(&subptr, info, sizeof(Dict_ptr));
                if (subptr)
                {
                    lookup_ec(dict, subptr, mi, ri, pos+1,
                              userfunc, range, prefix);
                    dict_bf_readp(dict->dbf, ptr, &p);
                    indxp = (short*) ((char*) p + 
                                      DICT_bsize(p)-sizeof(short));
                }
            }
        }
        lo++;
    }
    return 0;
}

static MatchInfo *prepare_match(Dict_char *pattern)
{
    int i;
    MatchWord *s;
    MatchInfo *mi;

    mi = (MatchInfo *) xmalloc(sizeof(*mi));
    mi->m = dict_strlen(pattern);
    mi->s = s = (MatchWord *) xmalloc(sizeof(*s)*256);  /* 256 !!! */
    for (i = 0; i < 256; i++)
        s[i] = 0;
    for (i = 0; pattern[i]; i++)
        s[pattern[i]&255] += 1<<i;
    return mi;
}

int dict_lookup_ec(Dict dict, char *pattern, int range,
                   int (*userfunc)(char *name))
{
    MatchInfo *mi;
    MatchWord *ri;
    int ret, i;
    Dict_char prefix[2048];

    if (!dict->head.root)
        return 0;
    
    mi = prepare_match((Dict_char*) pattern);
    
    ri = (MatchWord *) xmalloc((dict_strlen((Dict_char*) pattern)+range+2)
                               * (range+1)*sizeof(*ri));
    for (i = 0; i <= range; i++)
        ri[i] = (2<<i)-1;
    
    ret = lookup_ec(dict, dict->head.root, mi, ri, 0, userfunc,
                    range, prefix);
    xfree(ri);
    return ret;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

