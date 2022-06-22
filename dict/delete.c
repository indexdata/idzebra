/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

static void dict_del_subtree(Dict dict, Dict_ptr ptr,
                             void *client,
                             int (*f)(const char *, void *))
{
    void *p = 0;
    short *indxp;
    int i, hi;

    if (!ptr)
        return;

    dict_bf_readp(dict->dbf, ptr, &p);
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
            memcpy(&subptr, info, sizeof(Dict_ptr));

            if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
            {
                if (f)
                    (*f)(info+sizeof(Dict_ptr)+sizeof(Dict_char), client);
            }
            if (subptr)
            {
                dict_del_subtree(dict, subptr, client, f);

                /* page may be gone. reread it .. */
                dict_bf_readp(dict->dbf, ptr, &p);
                indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
            }
        }
    }
    DICT_backptr(p) = dict->head.freelist;
    dict->head.freelist = ptr;
    dict_bf_touch(dict->dbf, ptr);
}

static int dict_del_string(Dict dict, const Dict_char *str, Dict_ptr ptr,
                           int sub_flag, void *client,
                           int (*f)(const char *, void *))
{
    int mid, lo, hi;
    int cmp;
    void *p;
    short *indxp;
    char *info;
    int r = 0;
    Dict_ptr subptr = 0;

    if (!ptr)
        return 0;
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
            if (sub_flag)
            {
                /* determine if prefix match */
                if (!dict_strncmp(str, (Dict_char*) info, dict_strlen(str)))
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
                    dict_bf_touch(dict->dbf, ptr);
                    --hi;
                    mid = lo = 0;
                    r = 1; /* signal deleted */
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
                    dict_bf_touch(dict->dbf, ptr);
                    r = 1;
                    break;
                }
            }
        }
        else
        {
            Dict_char dc;

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
                    {
                        /* entry does exist. Wipe it out */
                        info[sizeof(Dict_ptr)+sizeof(Dict_char)] = 0;
                        DICT_type(p) = 1;
                        dict_bf_touch(dict->dbf, ptr);

                        if (f)
                            (*f)(info+sizeof(Dict_ptr)+sizeof(Dict_char),
                                 client);
                        r = 1;
                    }
                    if (sub_flag)
                    {
                        /* must delete all suffixes (subtrees) as well */
                        hi = DICT_nodir(p)-1;
                        while (mid < hi)
                        {
                            indxp[-mid] = indxp[-mid-1];
                            mid++;
                        }
                        (DICT_nodir(p))--;
                        DICT_type(p) = 1;
                        dict_bf_touch(dict->dbf, ptr);
                    }
                }
                else
                {
                    /* subptr may be 0 */
                    r = dict_del_string(dict, str, subptr, sub_flag, client, f);

                    /* recover */
                    dict_bf_readp(dict->dbf, ptr, &p);
                    indxp = (short*)
                        ((char*) p+DICT_bsize(p)-sizeof(short));
                    info = (char*)p - indxp[-mid];

                    subptr = 0; /* avoid dict_del_subtree (end of function)*/
                    if (r == 2)
                    {   /* subptr page became empty and is removed */

                        /* see if this entry is a real one or if it just
                           serves as pointer to subptr */
                        if (info[sizeof(Dict_ptr)+sizeof(Dict_char)])
                        {
                            /* this entry do exist, set subptr to 0 */
                            memcpy(info, &subptr, sizeof(subptr));
                        }
                        else
                        {
                            /* this entry ONLY points to subptr. remove it */
                            hi = DICT_nodir(p)-1;
                            while (mid < hi)
                            {
                                indxp[-mid] = indxp[-mid-1];
                                mid++;
                            }
                            (DICT_nodir(p))--;
                        }
                        dict_bf_touch(dict->dbf, ptr);
                        r = 1;
                    }
                }
                break;
            }
        }
        if (cmp < 0)
            lo = mid+1;
        else
            hi = mid-1;
    }
    if (DICT_nodir(p) == 0 && ptr != dict->head.root)
    {
        DICT_backptr(p) = dict->head.freelist;
        dict->head.freelist = ptr;
        dict_bf_touch(dict->dbf, ptr);
        r = 2;
    }
    if (subptr && sub_flag)
        dict_del_subtree(dict, subptr, client, f);

    return r;
}

int dict_delete(Dict dict, const char *p)
{
    return dict_del_string(dict, (const Dict_char*) p, dict->head.root, 0,
                           0, 0);
}

int dict_delete_subtree(Dict dict, const char *p, void *client,
                        int (*f)(const char *info, void *client))
{
    return dict_del_string(dict, (const Dict_char*) p, dict->head.root, 1,
                           client, f);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

