/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: delete.c,v $
 * Revision 1.4  1996-02-02 13:43:50  adam
 * The public functions simply use char instead of Dict_char to represent
 * search strings. Dict_char is used internally only.
 *
 * Revision 1.3  1995/12/07  11:48:55  adam
 * Insert operation obeys DICT_type = 1 (slack in page).
 * Function dict_open exists if page size or magic aren't right.
 *
 * Revision 1.2  1995/12/06  17:48:30  adam
 * Bug fix: delete didn't work.
 *
 * Revision 1.1  1995/12/06  14:52:21  adam
 * New function: dict_delete.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

static int dict_del (Dict dict, const Dict_char *str)
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
            {
                hi = DICT_nodir(p)-1;
                while (mid < hi)
                {
                    indxp[-mid] = indxp[-mid-1];
                    mid++;
                }
                DICT_type(p) = 1;
                (DICT_nodir(p))--;
                dict_bf_touch (dict->dbf, ptr);
                return 1;
            }
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
                    {
                        info[sizeof(Dict_ptr)+sizeof(Dict_char)] = 0;
                        DICT_type(p) = 1;
                        dict_bf_touch (dict->dbf, ptr);
                        return 1;
                    }
                    return 0;
                }
                else
                {
                    if (subptr == 0)
                        return 0;
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
    return 0;
}

int dict_delete (Dict dict, const char *p)
{
    if (dict->head.last == 1)
        return 0;
    return dict_del (dict, (const Dict_char*) p);
}
