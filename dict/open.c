/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: open.c,v $
 * Revision 1.4  1994-09-01 17:44:10  adam
 * depend include change.
 * CVS ----------------------------------------------------------------------
 *
 * Revision 1.3  1994/08/18  12:40:58  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.2  1994/08/17  13:32:20  adam
 * Use cache in dict - not in bfile.
 *
 * Revision 1.1  1994/08/16  16:26:49  adam
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

    dict->dbf = dict_bf_open (name, DICT_PAGESIZE, cache, rw);
    dict->rw = rw;

    if(!dict->dbf)
    {
        log (LOG_LOG, "cannot open `%s'", name);
        xfree (dict);
        return NULL;
    }
    if (dict_bf_readp (dict->dbf, 0, &head_buf) <= 0)
    {
        if (rw) 
        {   /* create header with information (page 0) */
            dict_bf_newp (dict->dbf, 0, &head_buf);
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
        if (strcmp (dh->magic_str, DICT_MAGIC))
        {
            log (LOG_LOG, "bad magic of `%s'", name);
            dict_bf_close (dict->dbf);
            xfree (dict);
            return NULL;
        }
        if (dh->page_size != DICT_PAGESIZE)
        {
            log (LOG_LOG, "page size mismatch of `%s'", name);
            dict_bf_close (dict->dbf);
            xfree (dict);
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
    return strlen(s);
}
