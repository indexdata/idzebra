/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: insert.c,v $
 * Revision 1.3  1994-08-18 12:40:56  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.2  1994/08/17  13:32:19  adam
 * Use cache in dict - not in bfile.
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

static Dict_ptr new_page (Dict dict, Dict_ptr back_ptr, void **pp)
{
    void *p;
    Dict_ptr ptr = dict->head.free_list;
    if (dict->head.free_list == dict->head.last)
    {
        dict->head.free_list++;
        dict->head.last = dict->head.free_list;
        dict_bf_newp (dict->dbf, ptr, &p);
    }
    else
    {
        dict_bf_readp (dict->dbf, dict->head.free_list, &p);
        dict->head.free_list = DICT_nextptr(p);
        if (dict->head.free_list == 0)
            dict->head.free_list = dict->head.last;
    }
    assert (p);
    DICT_type(p) = 1;
    DICT_backptr(p) = back_ptr;
    DICT_nextptr(p) = 0;
    DICT_nodir(p) = 0;
    DICT_size(p) = DICT_infoffset;
    *pp = p;
    return ptr;
}

static int dict_ins (Dict dict, const Dict_char *str, Dict_ptr back_ptr,
                     void *p, void *userinfo)
{
    int i;
    Dict_ptr ptr = back_ptr, subptr;
    short *indxp, *indxp1;
    short newsize;

    if (ptr == 0)
        ptr = new_page (dict, back_ptr, &p);
    assert (p);
    assert (ptr);

    indxp = (short*) ((char*) p+DICT_PAGESIZE);
    for (i = DICT_nodir (p); --i >= 0; )
    {
        char *info;
        int cmp;
        if (*--indxp > 0) /* tail string here! */
        {
            info = p + *indxp;
            cmp = dict_strcmp ((Dict_char*)
                              (info+sizeof(Dict_info)+sizeof(Dict_ptr)),
                               str);
            if (!cmp)
            {
                if (memcmp (info+sizeof(Dict_ptr), userinfo, sizeof(userinfo)))
                {
                    memcpy (info+sizeof(Dict_ptr), userinfo, sizeof(userinfo));
                    dict_bf_touch (dict->dbf, ptr);
                }
                return 0;
            }
            else if(cmp < 0)
                break;
        }
        else  /* tail of string in sub page */
        {
            assert (*indxp < 0);
            info = p - *indxp;
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
                    dict_bf_touch (dict->dbf, ptr);
                }
                return dict_ins (dict, str+1, ptr, pp, userinfo);
            }
            else if(cmp < 0)
                break;
        }
    }
    newsize = DICT_size(p);
    subptr = 0;
    memcpy (p+newsize, &subptr, sizeof(subptr));
    memcpy (p+newsize + sizeof(Dict_ptr), userinfo, sizeof(Dict_info));
    memcpy (p+newsize + sizeof(Dict_ptr)+sizeof(Dict_info), str,
            dict_strlen (str)+1);
    (DICT_nodir(p))++;
    indxp1 = (short*)((char*) p + DICT_PAGESIZE - DICT_nodir(p)*sizeof(short));
    for (; indxp1 != indxp; indxp1++)
        indxp1[0] = indxp1[1];
    *indxp = -newsize;
    
    DICT_size(p) = newsize + sizeof(Dict_info)+sizeof(Dict_ptr)
        +dict_strlen (str)+1;
    return 0;
}

int dict_insert (Dict dict, const Dict_char *str, void *userinfo)
{
    void *p;
    if (dict->head.last == 1)
        dict_ins (dict, str, 0, NULL, userinfo);
    else
    {
        dict_bf_readp (dict->dbf, 1, &p);
        dict_ins (dict, str, 1, p, userinfo);
    }
    return 0;
}
