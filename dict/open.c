/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: open.c,v $
 * Revision 1.1  1994-08-16 16:26:49  adam
 * Added dict.
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <dict.h>

Dict dict_open (const char *name, int cache, int rw)
{
    Dict dict;
    void *head_buf;
    struct Dict_head *dh;

    dict = xmalloc (sizeof(*dict));

    if (rw)
        dict->bf = bf_open_w (name, DICT_PAGESIZE, cache);
    else
        dict->bf = bf_open (name, DICT_PAGESIZE, cache);

    if(!dict->bf)
    {
        free (dict);
        return NULL;
    }
    if (bf_read (dict->bf, 0, &head_buf) <= 0)
    {
        if (rw) 
        {   /* create header with information (page 0) */
            bf_newp (dict->bf, 0, &head_buf);
            dh = (struct Dict_head *) head_buf;
            strcpy(dh->magic_str, DICT_MAGIC);
            dh->free_list = dh->last = 1;
            dh->page_size = DICT_PAGESIZE;
            memcpy (&dict->head, dh, sizeof(*dh));
        }
        else
        {   /* no header present, i.e. no dictionary at all */
            dict->head.free_list = dict->head.last = 0;
            dict->head.page_size = DICT_PAGESIZE;
        }
    }
    else /* header was there, check magic and page size */
    {
        dh = (struct Dict_head *) head_buf;
        if (!strcmp (dh->magic_str, DICT_MAGIC))
        {
            bf_close (dict->bf);
            free (dict);
            return NULL;
        }
        if (dh->page_size != DICT_PAGESIZE)
        {
            bf_close (dict->bf);
            free (dict);
            return NULL;
        }
        memcpy (&dict->head, dh, sizeof(*dh));
    }
    return dict;
}

int dict_strcmp (const Dict_char *s1, const Dict_char *s2)
{
    return strcmp (s1, s2);
}

int dict_strlen (const Dict_char *s)
{
    return strlen(s)+1;
}
