/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: scan.c,v 1.14 2002-01-14 01:52:22 adam Exp $
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

int dict_scan_trav (Dict dict, Dict_ptr ptr, int pos, Dict_char *str, 
		    int start, int *count, void *client,
                    int (*userfunc)(char *, const char *, int, void *),
		    int dir)
{
    int lo, hi, j;
    void *p;
    short *indxp;
    char *info;

    dict_bf_readp (dict->dbf, ptr, &p);
    hi = DICT_nodir(p)-1;
    if (start != -1)
        lo = start;
    else
    {
        if (dir == -1)
            lo = hi;
        else
            lo = 0;
    }
    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short)); 

    while (lo <= hi && lo >= 0 && *count > 0)
    {
        if (indxp[-lo] > 0)
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */

	    info = (char*)p + indxp[-lo];
            for (j = 0; info[j] != DICT_EOS; j++)
		str[pos+j] = info[j];
            str[pos+j] = DICT_EOS;
            (*userfunc)((char*) str, info+(j+1)*sizeof(Dict_char),
                            *count * dir, client);
            --(*count);
        }
        else
        {
            Dict_char dc;
	    Dict_ptr subptr;

            /* Dict_ptr             subptr */
            /* Dict_char            sub char */
            /* unsigned char        length of information */
            /* char *               information */

            info = (char*)p - indxp[-lo];
            memcpy (&dc, info+sizeof(Dict_ptr), sizeof(Dict_char));
            str[pos] = dc;
	    memcpy (&subptr, info, sizeof(Dict_ptr));
	    if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
            {
                 str[pos+1] = DICT_EOS;
                 if ((*userfunc)((char*) str,
                                 info+sizeof(Dict_ptr)+sizeof(Dict_char),
                                 *count * dir, client))
                     return 1;
                 --(*count);
            }
            if (*count > 0 && subptr)
            {
	        dict_scan_trav (dict, subptr, pos+1, str, -1, count, 
                                client, userfunc, dir);
                dict_bf_readp (dict->dbf, ptr, &p);
                indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short)); 
	    }
        }
        lo += dir;
    }
    return 0;
}

int dict_scan_r (Dict dict, Dict_ptr ptr, int pos, Dict_char *str, 
		 int *before, int *after, void *client,
                 int (*userfunc)(char *, const char *, int, void *))
{
    int cmp = 0, mid, lo, hi;
    void *p;
    short *indxp;
    char *info;

    dict_bf_readp (dict->dbf, ptr, &p);
    if (!p)
        return 0;
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
	    cmp = dict_strcmp ((Dict_char*) info, str + pos);
	    if (!cmp)
            {
                if (*after)
                {
                    (*userfunc)((char *) str, info+
                                (dict_strlen((Dict_char*) info)+1)
                                *sizeof(Dict_char), 
                                *after, client);
                    --(*after);
                }
                break;
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
	    cmp = dc - str[pos];
	    if (!cmp)
            {
		memcpy (&subptr, info, sizeof(Dict_ptr));
                if (str[pos+1] == DICT_EOS)
                {
		    if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
                    {
                        if (*after)
                        {
                            (*userfunc)((char*) str,
                                        info+sizeof(Dict_ptr)+
                                        sizeof(Dict_char),
                                        *after, client);
                            --(*after);
                        }
                    }
                    if (*after && subptr)
		        if (dict_scan_trav (dict, subptr, pos+1, str, -1, 
                                            after, client, userfunc, 1))
                            return 1;
                }
		else if (subptr)
                {
                    if (dict_scan_r (dict, subptr, pos+1, str, before, after,
                                     client, userfunc))
                        return 1;
                }
                break;
            }
        }
	if (cmp < 0)
	    lo = mid+1;
	else
	    hi = mid-1;
    }
    if (lo>hi && cmp < 0)
        ++mid;
    if (*after)
        if (dict_scan_trav (dict, ptr, pos, str, cmp ? mid : mid+1, after,
                            client, userfunc, 1))
            return 1;
    if (*before && mid > 0)
        if (dict_scan_trav (dict, ptr, pos, str, mid-1, before, 
                            client, userfunc, -1))
            return 1;
    return 0;
}

int dict_scan (Dict dict, char *str, int *before, int *after, void *client,
               int (*f)(char *name, const char *info, int pos, void *client))
{
    int i;

    logf (LOG_DEBUG, "dict_scan");
    for (i = 0; str[i]; i++)
    {
	logf (LOG_DEBUG, " %3d  %c", str[i],
	      (str[i] > ' ' && str[i] < 127) ? str[i] : '?');
    }
    if (!dict->head.root)
        return 0;
    return dict_scan_r (dict, dict->head.root, 0, (Dict_char *) str,
			before, after, client, f);
}
