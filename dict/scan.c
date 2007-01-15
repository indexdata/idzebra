/* $Id: scan.c,v 1.25 2007-01-15 15:10:15 adam Exp $
   Copyright (C) 1995-2007
   Index Data ApS

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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "dict-p.h"

static void scan_direction(Dict dict, Dict_ptr ptr, int pos, Dict_char *str, 
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
            if ((*userfunc)((char*) str, info+(j+1)*sizeof(Dict_char),
                            *count * dir, client))
            {
                *count = 0;
            }
            else
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
	    if (dir>0 && info[sizeof(Dict_ptr)+sizeof(Dict_char)])
            {
                 str[pos+1] = DICT_EOS;
                 if ((*userfunc)((char*) str,
                                 info+sizeof(Dict_ptr)+sizeof(Dict_char),
                                 *count * dir, client))
                 {
                     *count = 0;
                 }
                 else
                     --(*count);
            }
            if (*count>0 && subptr)
            {
	        scan_direction (dict, subptr, pos+1, str, -1, count, 
                                client, userfunc, dir);
                dict_bf_readp (dict->dbf, ptr, &p);
                indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short)); 
	    }
	    if (*count>0 && dir<0 && info[sizeof(Dict_ptr)+sizeof(Dict_char)])
            {
                 str[pos+1] = DICT_EOS;
                 if ((*userfunc)((char*) str,
                                 info+sizeof(Dict_ptr)+sizeof(Dict_char),
                                 *count * dir, client))
                 {
                     *count = 0;
                 }
                 else
                     --(*count);
            }
        }
        lo += dir;
    }
}

void dict_scan_r(Dict dict, Dict_ptr ptr, int pos, Dict_char *str, 
		 int *before, int *after, void *client,
                 int (*userfunc)(char *, const char *, int, void *))
{
    int cmp = 0, mid, lo, hi;
    void *p;
    short *indxp;
    char *info;

    dict_bf_readp (dict->dbf, ptr, &p);
    if (!p)
        return;
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
                    if ((*userfunc)((char *) str, info+
                                    (dict_strlen((Dict_char*) info)+1)
                                    *sizeof(Dict_char), 
                                    *after, client))
                    {
                        *after = 0;
                    }
                    else
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
                            if ((*userfunc)((char*) str,
                                            info+sizeof(Dict_ptr)+
                                            sizeof(Dict_char),
                                            *after, client))
                            {
                                *after = 0;
                            }
                            else
                                --(*after);
                        }
                    }
                    if (*after && subptr)
		        scan_direction(dict, subptr, pos+1, str, -1, 
                                       after, client, userfunc, 1);
                }
		else if (subptr)
                {
                    dict_scan_r(dict, subptr, pos+1, str, before, after,
                                client, userfunc);
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
        scan_direction(dict, ptr, pos, str, cmp ? mid : mid+1, after,
                       client, userfunc, 1);
    if (*before && mid > 0)
        scan_direction(dict, ptr, pos, str, mid-1, before, 
                       client, userfunc, -1);
}

int dict_scan(Dict dict, char *str, int *before, int *after, void *client,
              int (*f)(char *name, const char *info, int pos, void *client))
{
    int i;

    yaz_log(YLOG_DEBUG, "dict_scan");
    for (i = 0; str[i]; i++)
    {
	yaz_log(YLOG_DEBUG, "start_term pos %d %3d  %c", i, str[i],
                (str[i] > ' ' && str[i] < 127) ? str[i] : '?');
    }
    if (!dict->head.root)
        return 0;
    dict_scan_r(dict, dict->head.root, 0, (Dict_char *) str,
                before, after, client, f);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

