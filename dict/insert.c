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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "dict-p.h"

#define CHECK 0

static int dict_ins(Dict dict, const Dict_char *str,
                    Dict_ptr back_ptr, int userlen, void *userinfo);
static void clean_page(Dict dict, Dict_ptr ptr, void *p, Dict_char *out,
                       Dict_ptr subptr, char *userinfo);


static Dict_ptr new_page(Dict dict, Dict_ptr back_ptr, void **pp)
{
    void *p;
    Dict_ptr ptr = dict->head.last;
    if (!dict->head.freelist)
    {
        dict_bf_newp(dict->dbf, dict->head.last, &p, dict->head.page_size);
	(dict->head.last)++;
    }
    else
    {
	ptr = dict->head.freelist;
        dict_bf_readp(dict->dbf, ptr, &p);
        dict->head.freelist = DICT_backptr(p);
    }
    assert(p);
    DICT_type(p) = 0;
    DICT_backptr(p) = back_ptr;
    DICT_nodir(p) = 0;
    DICT_size(p) = DICT_infoffset;
    DICT_bsize(p) = dict->head.page_size;
    if (pp)
        *pp = p;
    return ptr;
}

static int split_page(Dict dict, Dict_ptr ptr, void *p)
{
    void *subp;
    char *info_here;
    Dict_ptr subptr;
    int i, j;
    short *indxp, *best_indxp = NULL;
    Dict_char best_char = 0;
    Dict_char prev_char = 0;
    int best_no = -1, no_current = 1;

    dict->no_split++;
    /* determine splitting char... */
    indxp = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
    for (i = DICT_nodir(p); --i >= 0; --indxp)
    {
        if (*indxp > 0) /* tail string here! */
        {
            Dict_char dc;

            memcpy(&dc, (char*) p + *indxp, sizeof(dc));
            if (best_no < 0)
            {   /* first entry met */
                best_char = prev_char = dc;
                best_no = 1;
                best_indxp = indxp;
            }
            else if (prev_char == dc)
            {   /* same char prefix. update */
                if (++no_current > best_no)
                {   /* best entry so far */
                    best_no = no_current;
                    best_char = dc;
                    best_indxp = indxp;
                }
            }
            else
            {   /* new char prefix. restore */
                prev_char = dc;
                no_current = 1;
            }
        }
    }
    assert(best_no >= 0); /* we didn't find any tail string entry at all! */

    j = best_indxp - (short*) p;
    subptr = new_page(dict, ptr, &subp);
    /* scan entries to see if there is a string with */
    /* length 1. info_here indicates if such entry exist */
    info_here = NULL;
    for (i=0; i<best_no; i++, j++)
    {
        char *info, *info1;
        int slen;
        Dict_char dc;

        info = (char*) p + ((short*) p)[j];
        /* entry start */
        memcpy(&dc, info, sizeof(dc));
        assert(dc == best_char);
        slen = 1+dict_strlen((Dict_char*) info);

        assert(slen > 1);
        if (slen == 2)
        {
            assert(!info_here);
            info_here = info+slen*sizeof(Dict_char);
        }
        else
        {
            info1 = info+slen*sizeof(Dict_char);  /* info start */
            dict_ins(dict, (Dict_char*) (info+sizeof(Dict_char)),
                     subptr, *info1, info1+1);
            dict_bf_readp(dict->dbf, ptr, &p);
        }
    }
    /* now clean the page ... */
    clean_page(dict, ptr, p, &best_char, subptr, info_here);
    return 0;
}

static void clean_page(Dict dict, Dict_ptr ptr, void *p, Dict_char *out,
                       Dict_ptr subptr, char *userinfo)
{
    char *np = (char *) xmalloc(dict->head.page_size);
    int i, slen, no = 0;
    short *indxp1, *indxp2;
    char *info1, *info2;

    DICT_bsize(np) = dict->head.page_size;
    indxp1 = (short*) ((char*) p+DICT_bsize(p)-sizeof(short));
    indxp2 = (short*) ((char*) np+DICT_bsize(np));
    info2 = (char*) np + DICT_infoffset;
    for (i = DICT_nodir(p); --i >= 0; --indxp1)
    {
        if (*indxp1 > 0) /* tail string here! */
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */

            info1 = (char*) p + *indxp1;
            if (out && memcmp(out, info1, sizeof(Dict_char)) == 0)
	    {
		if (subptr == 0)
	            continue;
		*--indxp2 = -(info2 - np);
		memcpy(info2, &subptr, sizeof(Dict_ptr));
		info2 += sizeof(Dict_ptr);
		memcpy(info2, out, sizeof(Dict_char));
		info2 += sizeof(Dict_char);
		if (userinfo)
		{
                    memcpy(info2, userinfo, *userinfo+1);
		    info2 += *userinfo + 1;
		}
		else
		    *info2++ = 0;
                subptr = 0;
	        ++no;
                continue;
	    }
            *--indxp2 = info2 - np;
            slen = (dict_strlen((Dict_char*) info1)+1)*sizeof(Dict_char);
            memcpy(info2, info1, slen);
	    info1 += slen;
            info2 += slen;
        }
        else
        {
            /* Dict_ptr             subptr */
            /* Dict_char            sub char */
            /* unsigned char        length of information */
            /* char *               information */

            assert(*indxp1 < 0);
            *--indxp2 = -(info2 - np);
            info1 = (char*) p - *indxp1;
            memcpy(info2, info1, sizeof(Dict_ptr)+sizeof(Dict_char));
	    info1 += sizeof(Dict_ptr)+sizeof(Dict_char);
            info2 += sizeof(Dict_ptr)+sizeof(Dict_char);
        }
        slen = *info1+1;
        memcpy(info2, info1, slen);
        info2 += slen;
        ++no;
    }
#if 1
    memcpy((char*)p+DICT_infoffset,
           (char*)np+DICT_infoffset,
           info2 - ((char*)np+DICT_infoffset));
    memcpy((char*)p + ((char*)indxp2 - (char*)np),
           indxp2,
           ((char*) np+DICT_bsize(p)) - (char*)indxp2);
#else
    memcpy((char*)p+DICT_infoffset, (char*)np+DICT_infoffset,
           DICT_pagesize(dict)-DICT_infoffset);
#endif
    DICT_size(p) = info2 - np;
    DICT_type(p) = 0;
    DICT_nodir(p) = no;
    xfree(np);
    dict_bf_touch(dict->dbf, ptr);
}



/* return 0 if new */
/* return 1 if before but change of info */
/* return 2 if same as before */

static int dict_ins(Dict dict, const Dict_char *str,
                    Dict_ptr ptr, int userlen, void *userinfo)
{
    int hi, lo, mid, slen, cmp = 1;
    short *indxp;
    char *info;
    void *p;

    dict_bf_readp(dict->dbf, ptr, &p);

    assert(p);
    assert(ptr);

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
            {
                info += (dict_strlen((Dict_char*) info)+1)*sizeof(Dict_char);
                /* consider change of userinfo length... */
                if (*info == userlen)
                {
		    /* change of userinfo ? */
                    if (memcmp(info+1, userinfo, userlen))
                    {
                        dict_bf_touch(dict->dbf, ptr);
                        memcpy(info+1, userinfo, userlen);
                        return 1;
                    }
		    /* same userinfo */
                    return 2;
                }
                else if (*info > userlen)
                {
		    /* room for new userinfo */
                    DICT_type(p) = 1;
                    *info = userlen;
                    dict_bf_touch(dict->dbf, ptr);
                    memcpy(info+1, userinfo, userlen);
                    return 1;
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
            memcpy(&dc, info+sizeof(Dict_ptr), sizeof(Dict_char));
            cmp = dc- *str;
            if (!cmp)
            {
                memcpy(&subptr, info, sizeof(Dict_ptr));
                if (*++str == DICT_EOS)
                {
		    /* finish of string. Store userinfo here... */

                    int xlen = info[sizeof(Dict_ptr)+sizeof(Dict_char)];
                    if (xlen == userlen)
                    {
                        if (memcmp(info+sizeof(Dict_ptr)+sizeof(Dict_char)+1,
                                   userinfo, userlen))
                        {
                            dict_bf_touch(dict->dbf, ptr);
                            memcpy(info+sizeof(Dict_ptr)+sizeof(Dict_char)+1,
                                   userinfo, userlen);
                            return 1;
                        }
                        return 2;
                    }
                    else if (xlen > userlen)
                    {
                        DICT_type(p) = 1;
                        info[sizeof(Dict_ptr)+sizeof(Dict_char)] = userlen;
                        memcpy(info+sizeof(Dict_ptr)+sizeof(Dict_char)+1,
                               userinfo, userlen);
                        dict_bf_touch(dict->dbf, ptr);
                        return 1;
                    }
		    /* xlen < userlen, expanding needed ... */
                    if (DICT_size(p)+sizeof(Dict_char)+sizeof(Dict_ptr)+
                        userlen >=
                        DICT_bsize(p) - (1+DICT_nodir(p))*sizeof(short))
                    {
			/* not enough room - split needed ... */
                        if (DICT_type(p) == 1)
                        {
                            clean_page(dict, ptr, p, NULL, 0, NULL);
                            return dict_ins(dict, str-1, ptr,
                                            userlen, userinfo);
                        }
                        if (split_page(dict, ptr, p))
                        {
                            yaz_log(YLOG_FATAL, "Unable to split page %d\n", ptr);
                            assert(0);
                        }
                        return dict_ins(dict, str-1, ptr, userlen, userinfo);
                    }
                    else
                    {   /* enough room - no split needed ... */
                        info = (char*)p + DICT_size(p);
                        memcpy(info, &subptr, sizeof(subptr));
                        memcpy(info+sizeof(Dict_ptr), &dc, sizeof(Dict_char));
                        info[sizeof(Dict_char)+sizeof(Dict_ptr)] = userlen;
                        memcpy(info+sizeof(Dict_char)+sizeof(Dict_ptr)+1,
                               userinfo, userlen);
                        indxp[-mid] = -DICT_size(p);
                        DICT_size(p) += sizeof(Dict_char)+sizeof(Dict_ptr)
                            +1+userlen;
                        DICT_type(p) = 1;
                        dict_bf_touch(dict->dbf, ptr);
                    }
                    if (xlen)
                        return 1;
                    return 0;
                }
                else
                {
                    if (subptr == 0)
                    {
                        subptr = new_page(dict, ptr, NULL);
                        memcpy(info, &subptr, sizeof(subptr));
                        dict_bf_touch(dict->dbf, ptr);
                    }
                    return dict_ins(dict, str, subptr, userlen, userinfo);
                }
            }
        }
        if (cmp < 0)
            lo = mid+1;
        else
            hi = mid-1;
    }
    indxp = indxp-mid;
    if (lo>hi && cmp < 0)
        --indxp;
    slen = (dict_strlen(str)+1)*sizeof(Dict_char);
    if (DICT_size(p)+slen+userlen >=
        (int)(DICT_bsize(p) - (1+DICT_nodir(p))*sizeof(short)))/* overflow? */
    {
        if (DICT_type(p))
        {
            clean_page(dict, ptr, p, NULL, 0, NULL);
            return dict_ins(dict, str, ptr, userlen, userinfo);
        }
        split_page(dict, ptr, p);
        return dict_ins(dict, str, ptr, userlen, userinfo);
    }
    if (cmp)
    {
        short *indxp1;
        (DICT_nodir(p))++;
        indxp1 = (short*)((char*) p + DICT_bsize(p)
                          - DICT_nodir(p)*sizeof(short));
        for (; indxp1 != indxp; indxp1++)
            indxp1[0] = indxp1[1];
#if CHECK
        indxp1 = (short*) ((char*) p+DICT_pagesize(dict)-sizeof(short));
        for (i = DICT_nodir (p); --i >= 0; --indxp1)
        {
            if (*indxp1 < 0)
            {
                info = (char*)p - *indxp1;
                assert (info[sizeof(Dict_ptr)] > ' ');
            }
        }
#endif
    }
    else
        DICT_type(p) = 1;
    info = (char*)p + DICT_size(p);
    memcpy(info, str, slen);
    info += slen;
    *info++ = userlen;
    memcpy(info, userinfo, userlen);
    info += userlen;

    *indxp = DICT_size(p);
    DICT_size(p) = info- (char*) p;
    dict_bf_touch(dict->dbf, ptr);
    if (cmp)
        return 0;
    return 1;
}

int dict_insert(Dict dict, const char *str, int userlen, void *userinfo)
{
    if (!dict->rw)
        return -1;
    dict->no_insert++;
    if (!dict->head.root)
    {
	void *p;
        dict->head.root = new_page(dict, 0, &p);
	if (!dict->head.root)
	    return -1;
    }
    return dict_ins(dict, (const Dict_char *) str, dict->head.root,
                    userlen, userinfo);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

