/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: insert.c,v $
 * Revision 1.7  1994-09-12 08:06:42  adam
 * Futher development of insert.c
 *
 * Revision 1.6  1994/09/06  13:05:15  adam
 * Further development of insertion. Some special cases are
 * not properly handled yet! assert(0) are put here. The
 * binary search in each page definitely reduce usr CPU.
 *
 * Revision 1.5  1994/09/01  17:49:39  adam
 * Removed stupid line. Work on insertion in dictionary. Not finished yet.
 *
 * Revision 1.4  1994/09/01  17:44:09  adam
 * depend include change.
 *
 * Revision 1.3  1994/08/18  12:40:56  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.2  1994/08/17  13:32:19  adam
 * Use cache in dict - not in bfile.
 *
 * Revision 1.1  1994/08/16  16:26:48  adam
 * Added dict.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

#define CHECK 0

static int dict_ins (Dict dict, const Dict_char *str,
                     Dict_ptr back_ptr, int userlen, void *userinfo);


static Dict_ptr new_page (Dict dict, Dict_ptr back_ptr, void **pp)
{
    void *p;
    Dict_ptr ptr = dict->head.free_list;
    if (dict->head.free_list == dict->head.last)
    {
        dict->head.free_list++;
        dict->head.last = dict->head.free_list;
        dict_bf_newp (dict->dbf, ptr, &p);
    }
    else
    {
        dict_bf_readp (dict->dbf, dict->head.free_list, &p);
        dict->head.free_list = DICT_nextptr(p);
        if (dict->head.free_list == 0)
            dict->head.free_list = dict->head.last;
    }
    assert (p);
    DICT_type(p) = 0;
    DICT_backptr(p) = back_ptr;
    DICT_nextptr(p) = 0;
    DICT_nodir(p) = 0;
    DICT_size(p) = DICT_infoffset;
    if (pp)
        *pp = p;
    return ptr;
}

static int split_page (Dict dict, Dict_ptr ptr, void *p)
{
    void *subp;
    char *info_here;
    Dict_ptr subptr;
    int i, need;
    short *indxp, *best_indxp;
    Dict_char best_char = 0;
    Dict_char prev_char = 0;
    int best_no = -1, no_current = 1;

    indxp = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));
    for (i = DICT_nodir (p); --i >= 0; --indxp)
    {
        if (*indxp > 0) /* tail string here! */
        {
            Dict_char dc;

            memcpy (&dc, (char*) p + *indxp, sizeof(dc));
            if (best_no < 0)
            {   /* first entry met */
                best_char = prev_char = dc;
                best_no = 1;
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
    if (best_no < 0) /* we didn't find any tail string entry at all! */
        return -1;

    subptr = new_page (dict, ptr, &subp);
    /* scan entries to see if there is a string with */
    /* length 1. info_here indicates if such entry exist */
    info_here = NULL;
    for (indxp=best_indxp, i=0; i<best_no; i++, indxp++)
    {
        char *info;
        int slen;

        assert (*indxp > 0);
        
        info = (char*) p + *indxp;                    /* entry start */
        assert (*info == best_char);
        slen = dict_strlen(info);

        assert (slen > 0);
        if (slen == 1)
        {
            assert (!info_here);
            info_here = info+(slen+1)*sizeof(Dict_char);
        }
    }
    /* calculate the amount of bytes needed for this entry when */
    /* transformed to a sub entry */
    need = sizeof(Dict_char)+sizeof(Dict_ptr)+1;
    if (info_here)
        need += *info_here;

    indxp = best_indxp;
    /* now loop on all entries with string length > 1 i.e. all */
    /* those entries which contribute to a sub page */
    best_indxp = NULL;                  
    for (i=0; i<best_no; i++, indxp++)
    {
        char *info, *info1;
        int slen;

        assert (*indxp > 0);
        
        info = (char*) p + *indxp;                    /* entry start */
        assert (*info == best_char);
        slen = dict_strlen(info);
            
        if (slen > 1)
        {
            info1 = info+(1+slen)*sizeof(Dict_char);  /* info start */

            if (need <= (1+slen)*sizeof(Dict_char) + 1 + *info1)
                best_indxp = indxp;                   /* space for entry */
            dict_ins (dict, info+sizeof(Dict_char), subptr, *info1, info1+1);
            dict_bf_readp (dict->dbf, ptr, &p);
        }
    }
    if (best_indxp)
    {   /* there was a hole big enough for a sub entry */
        char *info = (char*) p + *best_indxp;
        short *indxp1;

        *--indxp = - *best_indxp;
        DICT_type(p) = 1;
        DICT_nodir (p) -= (best_no-1);
        indxp1 = (short*)((char*)p+DICT_PAGESIZE-DICT_nodir(p)*sizeof(short));
        while (indxp != indxp1)
        {
            --indxp;
            *indxp = indxp[1-best_no];
        }
        memcpy (info, &subptr, sizeof(Dict_ptr));     /* store subptr */
        info += sizeof(Dict_ptr);
        memcpy (info, &best_char, sizeof(Dict_char)); /* store sub char */
        info += sizeof(Dict_char);
        if (info_here)
            memcpy (info, info_here, *info_here+1);   /* with information */
        else
            *info = 0;                                /* without info */
#if CHECK
        best_indxp = NULL;
        prev_char = 0;
        indxp = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));
        for (i = DICT_nodir (p); --i >= 0; --indxp)
        {
            if (*indxp > 0) /* tail string here! */
            {
                Dict_char dc;
                
                memcpy (&dc, (char*) p + *indxp, sizeof(dc));
                assert (dc != best_char);
                assert (dc >= prev_char);
                prev_char = dc;
            }
            else
            {
                Dict_char dc;
                memcpy (&dc, (char*)p - *indxp+sizeof(Dict_ptr),
                        sizeof(dc));
                assert (dc > prev_char);
                if (dc == best_char)
                {
                    assert (best_indxp == NULL);
                    best_indxp = indxp;
                }
                prev_char = dc;
            }
        }
        assert (best_indxp);
#endif    
    }
    else
    {
        short *indxp1, *indxp2;
        assert (0);
        DICT_type(p) = 1;
        DICT_nodir(p) -= best_no;
        indxp2 = indxp;
        indxp1 = (short*)((char*) p+DICT_PAGESIZE-DICT_nodir(p)*sizeof(short));
        do
        {
            --indxp2;
            indxp2[0] = indxp2[-best_no];
        } while (indxp2 != indxp1);
    }
    return 0;
}

static void clean_page (Dict dict, Dict_ptr ptr, void *p, Dict_char *out)
{
    char *np = xmalloc (dict->head.page_size);
    int i, slen, no = 0;
    short *indxp1, *indxp2;
    char *info1, *info2;

    indxp1 = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));
    indxp2 = (short*) ((char*) np+DICT_PAGESIZE);
    info2 = (char*) np + DICT_infoffset;
    for (i = DICT_nodir (p); --i >= 0; --indxp1)
    {
        if (*indxp1 > 0) /* tail string here! */
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */

            info1 = (char*) p + *indxp1;
            if (out && memcmp (out, info1, sizeof(Dict_char)) == 0)
                continue;
            *--indxp2 = info2 - np;
            slen = (dict_strlen(info1)+1)*sizeof(Dict_char);
            memcpy (info2, info1, slen);
            info2 += slen;
            info1 += slen;
        }
        else
        {
            /* Dict_ptr             subptr */
            /* Dict_char            sub char */
            /* unsigned char        length of information */
            /* char *               information */

            assert (*indxp1 < 0);
            *--indxp2 = -(info2 - np);
            info1 = (char*) p - *indxp1;
            memcpy (info2, info1, sizeof(Dict_ptr)+sizeof(Dict_char));
            info2 += sizeof(Dict_ptr)+sizeof(Dict_char);
            info1 += sizeof(Dict_ptr)+sizeof(Dict_char);
        }
        slen = *info1+1;
        memcpy (info2, info1, slen);
        info2 += slen;
        ++no;
    }
    memcpy ((char*)p+DICT_infoffset, (char*)np+DICT_infoffset,
            DICT_PAGESIZE-DICT_infoffset);
    DICT_size(p) = info2 - np;
    DICT_type(p) = 0;
    DICT_nodir(p) = no;
    xfree (np);
}



/* return 0 if new */
/* return 1 if before but change of info */
/* return 2 if same as before */

static int dict_ins (Dict dict, const Dict_char *str,
                     Dict_ptr back_ptr, int userlen, void *userinfo)
{
    int hi, lo, mid, i, slen, cmp = 1;
    Dict_ptr ptr = back_ptr;
    short *indxp;
    char *info;
    void *p;

    if (ptr == 0)
        ptr = new_page (dict, back_ptr, &p);
    else
        dict_bf_readp (dict->dbf, ptr, &p);
        
    assert (p);
    assert (ptr);

    mid = lo = 0;
    hi = DICT_nodir(p)-1;
    indxp = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));
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
                info += (dict_strlen(info)+1)*sizeof(Dict_char);
                /* consider change of userinfo length... */
                if (*info == userlen)
                {
                    if (memcmp (info+1, userinfo, userlen))
                    {
                        dict_bf_touch (dict->dbf, ptr);
                        memcpy (info+1, userinfo, userlen);
                        return 1;
                    }
                    return 2;
                }
                else if (*info > userlen)
                {
                    DICT_type(p) = 1;
                    *info = userlen;
                    dict_bf_touch (dict->dbf, ptr);
                    memcpy (info+1, userinfo, userlen);
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
            memcpy (&dc, info+sizeof(Dict_ptr), sizeof(Dict_char));
            cmp = dc- *str;
            if (!cmp)
            {
                memcpy (&subptr, info, sizeof(Dict_ptr));
                if (*++str == DICT_EOS)
                {
                    int xlen;
                    
                    xlen = info[sizeof(Dict_ptr)+sizeof(Dict_char)];
                    if (xlen == userlen)
                    {
                        if (memcmp (info+sizeof(Dict_ptr)+sizeof(Dict_char)+1,
                                    userinfo, userlen))
                        {
                            dict_bf_touch (dict->dbf, ptr);
                            memcpy (info+sizeof(Dict_ptr)+sizeof(Dict_char)+1,
                                    userinfo, userlen);
                            return 1;
                        }
                        return 2;
                    }
                    else if (xlen > userlen)
                    {
                        DICT_type(p) = 1;
                        info[sizeof(Dict_ptr)+sizeof(Dict_char)] = userlen;
                        memcpy (info+sizeof(Dict_ptr)+sizeof(Dict_char)+1,
                                userinfo, userlen);
                        dict_bf_touch (dict->dbf, ptr);
                        return 1;
                    }
                    if (DICT_size(p)+sizeof(Dict_char)+sizeof(Dict_ptr)+
                        userlen >=
                        DICT_PAGESIZE - (1+DICT_nodir(p))*sizeof(short))
                    {
                        if (DICT_type(p) == 1)
                        {
                            clean_page (dict, ptr, p, NULL);
                            dict_bf_touch (dict->dbf, ptr);
                            return dict_ins (dict, str-1, ptr,
                                             userlen, userinfo);
                        }
                        if (split_page (dict, ptr, p)) 
                        {
                            log (LOG_FATAL, "Unable to split page %d\n", ptr);
                            abort ();
                        }
                        return dict_ins (dict, str-1, ptr, userlen, userinfo);
                    }
                    else
                    {
                        info = (char*)p + DICT_size(p);
                        memcpy (info, &subptr, sizeof(subptr));
                        memcpy (info+sizeof(Dict_ptr), &dc, sizeof(Dict_char));
                        info[sizeof(Dict_char)+sizeof(Dict_ptr)] = userlen;
                        memcpy (info+sizeof(Dict_char)+sizeof(Dict_ptr)+1,
                                userinfo, userlen);
                        indxp[-mid] = -DICT_size(p);
                        DICT_size(p) += sizeof(Dict_char)+sizeof(Dict_ptr)
                            +1+userlen;
                        DICT_type(p) = 1;
                        dict_bf_touch (dict->dbf, ptr);
                    }
                    if (xlen)
                        return 1;
                    return 0;
                }
                else
                {
                    if (subptr == 0)
                    {
                        subptr = new_page (dict, ptr, NULL);
                        memcpy (info, &subptr, sizeof(subptr));
                        dict_bf_touch (dict->dbf, ptr);
                    }
                    return dict_ins (dict, str, subptr, userlen, userinfo);
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
        DICT_PAGESIZE - (1+DICT_nodir(p))*sizeof(short)) /* overflow? */
    {
        if (DICT_type(p) == 1)
        {
            clean_page (dict, ptr, p, NULL);
            dict_bf_touch (dict->dbf, ptr);
            return dict_ins (dict, str, ptr, userlen, userinfo);
        }
        i = 0;
        do 
        {
            assert (i <= 1);
            if (split_page (dict, ptr, p)) 
            {
                log (LOG_FATAL, "Unable to split page %d\n", ptr);
                abort ();
            }
            if (DICT_size(p)+slen+userlen <
                DICT_PAGESIZE - (1+DICT_nodir(p))*sizeof(short))
                break;
            i++;
            clean_page (dict, ptr, p, NULL);
        } while (DICT_size(p)+slen+userlen > DICT_PAGESIZE -
                 (1+DICT_nodir(p))*sizeof(short));
        return dict_ins (dict, str, ptr, userlen, userinfo);
    }
    if (cmp)
    {
        short *indxp1;
        (DICT_nodir(p))++;
        indxp1 = (short*)((char*) p + DICT_PAGESIZE
                          - DICT_nodir(p)*sizeof(short));
        for (; indxp1 != indxp; indxp1++)
            indxp1[0] = indxp1[1];
#if CHECK
        indxp1 = (short*) ((char*) p+DICT_PAGESIZE-sizeof(short));
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
    memcpy (info, str, slen);
    info += slen;
    *info++ = userlen;
    memcpy (info, userinfo, userlen);
    info += userlen;

    *indxp = DICT_size(p);
    DICT_size(p) = info- (char*) p;
    dict_bf_touch (dict->dbf, ptr);
    if (cmp)
        return 0;
    return 1;
}

int dict_insert (Dict dict, const Dict_char *str, int userlen, void *userinfo)
{
    assert (dict->head.last > 0);
    if (dict->head.last == 1)
        return dict_ins (dict, str, 0, userlen, userinfo);
    else
        return dict_ins (dict, str, 1, userlen, userinfo);
}

