/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: scan.c,v $
 * Revision 1.1  1995-10-06 09:04:18  adam
 * First version of scan.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

void dict_scan_trav (Dict dict, Dict_ptr ptr, int pos, Dict_char *str, 
		    int start, int *count,
                    int (*userfunc)(Dict_char *, const char *, int pos),
		    int dir)
{
    int lo, hi, j;
    void *p;
    short *indxp;
    char *info;

    dict_bf_readp (dict->dbf, ptr, &p);
    hi = DICT_nodir(p)-1;
    if (start == 0 && dir == -1)
        lo = hi;
    else
        lo = start;
    indxp = (short*) ((char*) p+DICT_pagesize(dict)-sizeof(short)); 

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
            (*userfunc)(str, info+j*sizeof(Dict_char), *count * dir);
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
                 (*userfunc)(str, info+sizeof(Dict_ptr)+sizeof(Dict_char),
			     *count * dir);
                 --(*count);
            }
            if (*count > 0 && subptr)
	         dict_scan_trav (dict, subptr, pos+1, str, 0, count, 
                                 userfunc, dir);
        }
        lo += dir;
    }
}
    
int dict_scan_r (Dict dict, Dict_ptr ptr, int pos, Dict_char *str, 
		 int *before, int *after,
                 int (*userfunc)(Dict_char *, const char *, int))
{
    int cmp = 0, mid, lo, hi, j;
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
	    cmp = dict_strcmp ((Dict_char*) info, str + pos);
	    if (!cmp)
            {
		for (j = 0; info[j++] != DICT_EOS; )
		    ;
                (*userfunc)(str, info+j*sizeof(Dict_char), *after);
                --(*after);
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
                        (*userfunc)(str, 
                                    info+sizeof(Dict_ptr)+sizeof(Dict_char),
				    *after);
	                --(*after);
                    }
                    if (*after > 0 && subptr)
		        dict_scan_trav (dict, subptr, pos+1, str, 0, 
                                        after, userfunc, 1);
                }
		else if (*after > 0 && subptr)
                    dict_scan_r (dict, subptr, pos+1, str, before, after,
                                 userfunc);
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
        dict_scan_trav (dict, ptr, pos, str, cmp ? mid : mid+1, after,
                        userfunc, 1);
    if (*before && mid > 1)
        dict_scan_trav (dict, ptr, pos, str, mid-1, before, 
                        userfunc, -1);
    return 0;
}

int dict_scan (Dict dict, Dict_char *str, int *before, int *after,
               int (*f)(Dict_char *name, const char *info, int pos))
{
    int i;
    i = dict_scan_r (dict, 1, 0, str, before, after, f);
    return i;
}

