/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lookup.c,v $
 * Revision 1.2  1994-09-16 15:39:14  adam
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

static char *dict_look (Dict dict, Dict_char *str)
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
    indxp = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));    
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
                return info+(dict_strlen (info)+1)*sizeof(Dict_char);
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
        if (cmp < 0)
            lo = mid+1;
        else
            hi = mid-1;
    }
    return NULL;
}

char *dict_lookup (Dict dict, Dict_char *p)
{
    if (dict->head.last == 1)
        return NULL;
    return dict_look (dict, p);
}


