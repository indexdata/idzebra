/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: open.c,v $
 * Revision 1.18  2002-05-03 13:47:15  adam
 * make checkergcc happy
 *
 * Revision 1.17  2000/12/05 09:59:10  adam
 * Work on dict_delete_subtree.
 *
 * Revision 1.16  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.15  1999/05/15 14:36:37  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.14  1999/03/09 13:07:06  adam
 * Work on dict_compact routine.
 *
 * Revision 1.13  1999/02/02 14:50:27  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.12  1997/09/17 12:19:07  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.11  1996/10/29 14:00:05  adam
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

Dict dict_open (BFiles bfs, const char *name, int cache, int rw,
		int compact_flag)
{
    Dict dict;
    void *head_buf;
    char resource_str[80];
    int page_size;

    dict = (Dict) xmalloc (sizeof(*dict));

    if (cache < 5)
	cache = 5;
    sprintf (resource_str, "dict.%s.pagesize", name);

    dict->grep_cmap = NULL;
    page_size = DICT_DEFAULT_PAGESIZE;
    if (page_size < 2048)
    {
        logf (LOG_WARN, "Resource %s was too small. Set to 2048",
              resource_str);
        page_size = 2048;
    }
    dict->dbf = dict_bf_open (bfs, name, page_size, cache, rw);
    dict->rw = rw;

    if(!dict->dbf)
    {
        logf (LOG_WARN, "Cannot open `%s'", name);
        xfree (dict);
        return NULL;
    }
    if (dict_bf_readp (dict->dbf, 0, &head_buf) <= 0)
    {
	memset (dict->head.magic_str, 0, sizeof(dict->head.magic_str));
	strcpy (dict->head.magic_str, DICT_MAGIC);
	dict->head.last = 1;
	dict->head.root = 0;
	dict->head.freelist = 0;
	dict->head.page_size = page_size;
	dict->head.compact_flag = compact_flag;
	
	/* create header with information (page 0) */
        if (rw) 
            dict_bf_newp (dict->dbf, 0, &head_buf, page_size);
    }
    else /* header was there, check magic and page size */
    {
	memcpy (&dict->head, head_buf, sizeof(dict->head));
        if (strcmp (dict->head.magic_str, DICT_MAGIC))
        {
            logf (LOG_WARN, "Bad magic of `%s'", name);
            exit (1);
        }
        if (dict->head.page_size != page_size)
        {
            logf (LOG_WARN, "Resource %s is %d and pagesize of `%s' is %d",
                  resource_str, page_size, name, dict->head.page_size);
	    return 0;
        }
    }
    if (dict->head.compact_flag)
	dict_bf_compact(dict->dbf);
    return dict;
}

int dict_strcmp (const Dict_char *s1, const Dict_char *s2)
{
    return strcmp ((const char *) s1, (const char *) s2);
}

int dict_strncmp (const Dict_char *s1, const Dict_char *s2, size_t n)
{
    return strncmp ((const char *) s1, (const char *) s2, n);
}

int dict_strlen (const Dict_char *s)
{
    return strlen((const char *) s);
}
