/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lookup.c,v $
 * Revision 1.7  1996-02-02 13:43:51  adam
 * The public functions simply use char instead of Dict_char to represent
 * search strings. Dict_char is used internally only.
 *
 * Revision 1.6  1995/12/11  09:04:50  adam
 * Bug fix: the lookup/scan/lookgrep didn't handle empty dictionary.
 *
 * Revision 1.5  1995/09/04  09:09:15  adam
 * String arg in lookup is const.
 *
 * Revision 1.4  1994/10/05  12:16:51  adam
 * Pagesize is a resource now.
 *
 * Revision 1.3  1994/09/26  10:17:25  adam
 * Minor changes.
 *
 * Revision 1.2  1994/09/16  15:39:14  adam
 * Initial code of lookup - not tested yet.
 *
 * Revision 1.1  1994/08/16  16:26:48  adam
 * Added dict.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

static char *dict_look (Dict dict, const Dict_char *str)
{
    Dict_ptr ptr = 1;
    int mid, lo, hi;
    int cmp;
    void *p;
    short *indxp;
    char *info;

    dict_bf_readp (dict->dbf, ptr, &p);
    mid = lo = 0;
    hi = DICT_nodir(p)-1;
    indxp = (short*) ((char*) p+DICT_pagesize(dict)-sizeof(short));    
    while (lo <= hi)
    {
        mid = (lo+hi)/2;
        if (indxp[-mid] > 0)
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */
            info = (char*)p + indxp[-mid];
            cmp = dict_strcmp((Dict_char*) info, str);
            if (!cmp)
                return info+(dict_strlen ((Dict_char*) info)+1)
                    *sizeof(Dict_char);
        }
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
                {
                    if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
                        return info+sizeof(Dict_ptr)+sizeof(Dict_char);
                    return NULL;
                }
                else
                {
                    if (subptr == 0)
                        return NULL;
                    ptr = subptr;
                    dict_bf_readp (dict->dbf, ptr, &p);
                    mid = lo = 0;
                    hi = DICT_nodir(p)-1;
                    indxp = (short*) ((char*) p+DICT_pagesize(dict)
                                      -sizeof(short));
                    continue;
                }
            }
        }
        if (cmp < 0)
            lo = mid+1;
        else
            hi = mid-1;
    }
    return NULL;
}

char *dict_lookup (Dict dict, const char *p)
{
    if (dict->head.last <= 1)
        return NULL;
    return dict_look (dict, (const Dict_char *) p);
}


