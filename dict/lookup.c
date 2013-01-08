/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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



#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "dict-p.h"

static char *dict_look(Dict dict, const Dict_char *str, Dict_ptr ptr)
{
    int mid, lo, hi;
    int cmp;
    void *p;
    short *indxp;
    char *info;

    dict_bf_readp(dict->dbf, ptr, &p);
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
            if (!cmp)
                return info+(dict_strlen ((Dict_char*) info)+1)
                    *sizeof(Dict_char);
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
            memcpy(&dc, info+sizeof(Dict_ptr), sizeof(Dict_char));
            cmp = dc- *str;
            if (!cmp)
            {
                memcpy(&subptr, info, sizeof(Dict_ptr));
                if (*++str == DICT_EOS)
                {
                    if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
                        return info+sizeof(Dict_ptr)+sizeof(Dict_char);
                    return NULL;
                }
                else
                {
                    if (subptr == 0)
                        return NULL;
                    ptr = subptr;
                    dict_bf_readp(dict->dbf, ptr, &p);
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
    return NULL;
}

char *dict_lookup(Dict dict, const char *p)
{
    dict->no_lookup++;
    if (!dict->head.root)
        return NULL;
    return dict_look(dict, (const Dict_char *) p, dict->head.root);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

