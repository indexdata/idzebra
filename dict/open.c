/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: open.c,v $
 * Revision 1.11  1996-10-29 14:00:05  adam
 * Page size given by DICT_DEFAULT_PAGESIZE in dict.h.
 *
 * Revision 1.10  1996/05/24 14:46:04  adam
 * Added dict_grep_cmap function to define user-mapping in grep lookups.
 *
 * Revision 1.9  1996/02/02  13:43:51  adam
 * The public functions simply use char instead of Dict_char to represent
 * search strings. Dict_char is used internally only.
 *
 * Revision 1.8  1995/12/07  11:48:56  adam
 * Insert operation obeys DICT_type = 1 (slack in page).
 * Function dict_open exists if page size or magic aren't right.
 *
 * Revision 1.7  1995/09/04  12:33:32  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.6  1994/10/05  12:16:52  adam
 * Pagesize is a resource now.
 *
 * Revision 1.5  1994/09/01  17:49:39  adam
 * Removed stupid line. Work on insertion in dictionary. Not finished yet.
 *
 * Revision 1.4  1994/09/01  17:44:10  adam
 * depend include change.
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
    char resource_str[80];
    int page_size;

    dict = xmalloc (sizeof(*dict));

    sprintf (resource_str, "dict.%s.pagesize", name);

    dict->grep_cmap = NULL;
    page_size = DICT_DEFAULT_PAGESIZE;
    if (page_size < 2048)
    {
        logf (LOG_WARN, "Resource %s was too small. Set to 2048",
              resource_str);
        page_size = 2048;
    }
    dict->dbf = dict_bf_open (name, page_size, cache, rw);
    dict->rw = rw;

    if(!dict->dbf)
    {
        logf (LOG_WARN, "Cannot open `%s'", name);
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
            dh->page_size = page_size;
            memcpy (&dict->head, dh, sizeof(*dh));
        }
        else
        {   /* no header present, i.e. no dictionary at all */
            dict->head.free_list = dict->head.last = 0;
            dict->head.page_size = page_size;
        }
    }
    else /* header was there, check magic and page size */
    {
        dh = (struct Dict_head *) head_buf;
        if (strcmp (dh->magic_str, DICT_MAGIC))
        {
            logf (LOG_WARN, "Bad magic of `%s'", name);
            exit (1);
        }
        if (dh->page_size != page_size)
        {
            logf (LOG_WARN, "Resource %s is %d and pagesize of `%s' is %d",
                  resource_str, page_size, name, dh->page_size);
            exit (1);
        }
        memcpy (&dict->head, dh, sizeof(*dh));
    }
    return dict;
}

int dict_strcmp (const Dict_char *s1, const Dict_char *s2)
{
    return strcmp ((const char *) s1, (const char *) s2);
}

int dict_strlen (const Dict_char *s)
{
    return strlen((const char *) s);
}
