/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: insert.c,v $
 * Revision 1.1  1994-08-16 16:26:48  adam
 * Added dict.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

static Dict_ptr new_page (Dict dict, Dict_ptr back_ptr, void **pp)
{
    void *p;
    Dict_ptr ptr = dict->head.free_list;
    if (dict->head.free_list == dict->head.last)
    {
        dict->head.free_list++;
        dict->head.last = dict->head.free_list;
        bf_newp (dict->bf, ptr, &p);
    }
    else
    {
        bf_readp (dict->bf, dict->head.free_list, &p);
        dict->head.free_list = DICT_nextptr(p);
        if (dict->head.free_list == 0)
            dict->head.free_list = dict->head.last;
    }
    assert (p);
    DICT_type(p) = 1;
    DICT_backptr(p) = back_ptr;
    DICT_nextptr(p) = 0;
    DICT_nodir(p) = 0;
    DICT_size(p) = 0;
    *pp = p;
    return ptr;
}

static int dict_ins (Dict dict, const Dict_char *str, Dict_ptr back_ptr,
                     void *p, void *userinfo)
{
    Dict_ptr ptr = back_ptr, subptr;
    short *indxp, *indxp1, *indxp2;
    short newsize;
    if (ptr == 0)
        ptr = new_page (dict, back_ptr, &p);
    assert (p);
    assert (ptr);

    indxp = (short*) ((char*) p+DICT_PAGESIZE);
    while (*str != DICT_EOS)
    {
        char *info;
        if (*--indxp > 0) /* tail string here! */
        {
            int cmp;
            info = DICT_info(p) + *indxp;
            cmp = dict_strcmp ((Dict_char*)
                              (info+sizeof(Dict_info)+sizeof(Dict_ptr)),
                               str);
            if (!cmp)
            {
                if (memcmp (info+sizeof(Dict_ptr), userinfo, sizeof(userinfo)))
                {
                    memcpy (info+sizeof(Dict_ptr), userinfo, sizeof(userinfo));
                    bf_touch (dict->bf, ptr);
                }
                return 0;
            }
            else if(cmp < 0)
                break;
            
        }
        else if(*indxp < 0)  /* tail of string in sub page */
        {
            int cmp;
            info = DICT_info(p) - *indxp;
            cmp = memcmp (info+sizeof(Dict_info)+sizeof(Dict_ptr), str, 
                         sizeof(Dict_char));
            if (!cmp)
            {
                Dict_ptr subptr;
                void *pp;
                memcpy (&subptr, info, sizeof(subptr));
                if (subptr == 0)
                {
                    subptr = new_page (dict, ptr, &pp);
                    memcpy (info, &subptr, sizeof(subptr));
                    bf_touch (dict->bf, ptr);
                }
                return dict_ins (dict, str+1, ptr, pp, userinfo);
            }
            else if(cmp < 0)
                break;
        }
        else
            break;
    }
    newsize = DICT_size(p);
    subptr = 0;
    memcpy (DICT_info(p) + newsize, &subptr, sizeof(subptr));
    memcpy (DICT_info(p) + newsize + sizeof(Dict_ptr), userinfo,
            sizeof(Dict_info));
    memcpy (DICT_info(p) + newsize + sizeof(Dict_ptr)+sizeof(Dict_info),
            str, dict_strlen (str));
    newsize = DICT_size(p) +
        sizeof(Dict_info) + sizeof(Dict_ptr) + dict_strlen (str);
    DICT_size (p) = newsize;

    DICT_nodir(p) = DICT_nodir(p)+1;
    indxp2 = (short*)((char*) p + DICT_PAGESIZE - DICT_nodir(p)*sizeof(short));
    for (indxp1 = indxp2; indxp1 != indxp; indxp1++)
        indxp[0] = indxp[1];
    *indxp = -newsize;
    return 0;
}

int dict_insert (Dict dict, const Dict_char *str, void *userinfo)
{
    dict_ins (dict, str, 0, NULL, userinfo);
    return 0;
}













