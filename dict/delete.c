/*
 * Copyright (C) 1994-2000, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: delete.c,v $
 * Revision 1.7  2000-12-05 09:59:10  adam
 * Work on dict_delete_subtree.
 *
 * Revision 1.6  1999/05/15 14:36:37  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.5  1999/02/02 14:50:17  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.4  1996/02/02 13:43:50  adam
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

static void dict_del_subtree (Dict dict, Dict_ptr ptr,
			      void *client, 
			      int (*f)(const char *, void *))
{
    int more = 1;
    void *p = 0;
    
    if (!ptr)
	return;
    
    while (more)
    {
	short *indxp;
	int i, hi;
	
	dict_bf_readp (dict->dbf, ptr, &p);
	hi = DICT_nodir(p)-1;
	indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
	more = 0;
	for (i = 0; i <= hi; i++)
	{
	    if (indxp[-i] > 0)
	    {
		/* string (Dict_char *) DICT_EOS terminated */
		/* unsigned char        length of information */
		/* char *               information */
		char *info = (char*)p + indxp[-i];
		if (f)
		    (*f)(info + (dict_strlen((Dict_char*) info)+1)
			 *sizeof(Dict_char), client);
	    }
	    else
	    {
		
		Dict_ptr subptr;
	    
		/* Dict_ptr             subptr */
		/* Dict_char            sub char */
		/* unsigned char        length of information */
		/* char *               information */
		char *info = (char*)p - indxp[-i];
		memcpy (&subptr, info, sizeof(Dict_ptr));

		if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
		{
		    if (f)
			(*f)(info+sizeof(Dict_ptr)+sizeof(Dict_char), client);
		}
		if (subptr)
		{
		    dict_del_subtree (dict, subptr, client, f);
		    more = 1;
		    break;
		}
	    }
	}
    }
    DICT_backptr(p) = dict->head.freelist;
    dict->head.freelist = ptr;
    dict_bf_touch (dict->dbf, ptr);
}

static int dict_del_string (Dict dict, const Dict_char *str, Dict_ptr ptr,
			    int sub_flag, void *client, 
			    int (*f)(const char *, void *))
{
    int mid, lo, hi;
    int cmp;
    void *p;
    short *indxp;
    char *info;

    if (!ptr)
	return 0;
    dict_bf_readp (dict->dbf, ptr, &p);
    mid = lo = 0;
    hi = DICT_nodir(p)-1;
    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));    
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
	    if (sub_flag)
	    {
		/* determine if prefix match */
		if (!dict_strncmp (str, (Dict_char*) info, dict_strlen(str)))
		{
		    if (f)
			(*f)(info + (dict_strlen((Dict_char*) info)+1)
			     *sizeof(Dict_char), client);

		    hi = DICT_nodir(p)-1;
		    while (mid < hi)
		    {
			indxp[-mid] = indxp[-mid-1];
			mid++;
		    }
		    DICT_type(p) = 1;
		    (DICT_nodir(p))--;
		    dict_bf_touch (dict->dbf, ptr);
		    --hi;
		    mid = lo = 0;
                    /* start again (may not be the most efficient way to go)*/
		    continue; 
		}
	    }
	    else
	    {
		/* normal delete: delete if equal */
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
		    if (sub_flag && subptr)
		    {
			Dict null_ptr = 0;
			memcpy (info, &null_ptr, sizeof(Dict_ptr));
		    }
                    if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
                    {
                        info[sizeof(Dict_ptr)+sizeof(Dict_char)] = 0;
                        DICT_type(p) = 1;
                        dict_bf_touch (dict->dbf, ptr);

			if (f)
			    (*f)(info+sizeof(Dict_ptr)+sizeof(Dict_char),
				 client);
			if (sub_flag && subptr)
			    dict_del_subtree (dict, subptr, client, f);
                        return 1;
                    }
		    if (sub_flag && subptr)
		    {
                        DICT_type(p) = 1;
                        dict_bf_touch (dict->dbf, ptr);
			dict_del_subtree (dict, subptr, client, f);
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
                    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
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
    return dict_del_string (dict, (const Dict_char*) p, dict->head.root, 0,
			    0, 0);
}

int dict_delete_subtree (Dict dict, const char *p, void *client,
			 int (*f)(const char *info, void *client))
{
    return dict_del_string (dict, (const Dict_char*) p, dict->head.root, 1,
			    client, f);
}
