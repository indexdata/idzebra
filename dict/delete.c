/* $Id: delete.c,v 1.10 2004-12-08 12:23:08 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "dict-p.h"

static void dict_del_subtree (Dict dict, Dict_ptr ptr,
			      void *client, 
			      int (*f)(const char *, void *))
{
    void *p = 0;
    short *indxp;
    int i, hi;
    
    if (!ptr)
	return;
	
    dict_bf_readp (dict->dbf, ptr, &p);
    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
    hi = DICT_nodir(p)-1;
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
	
		/* page may be gone. reread it .. */
		dict_bf_readp (dict->dbf, ptr, &p);
		indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
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
