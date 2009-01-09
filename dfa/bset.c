/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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


#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <idzebra/util.h>
#include <bset.h>
#include "imalloc.h"

#define GET_BIT(s,m) (s[(m)/(sizeof(BSetWord)*8)]&(1<<(m&(sizeof(BSetWord)*8-1)))) 
#define SET_BIT(s,m) (s[(m)/(sizeof(BSetWord)*8)]|=(1<<(m&(sizeof(BSetWord)*8-1))))

BSetHandle *mk_BSetHandle (int size, int chunk)
{
    int wsize = 1+size/(sizeof(BSetWord)*8);    
    BSetHandle *sh;

    if (chunk <= 1)
        chunk = 32;
    sh = (BSetHandle *) imalloc (sizeof(BSetHandle) + 
                                chunk*sizeof(BSetWord)*wsize);

    sh->size = size;
    sh->wsize = wsize;
    sh->chunk = chunk * wsize;
    sh->offset = 0;
    sh->setchain = NULL;
    return sh;
}

void rm_BSetHandle (BSetHandle **shp)
{
    BSetHandle *sh, *sh1;

    assert (shp);
    sh = *shp;
    assert (sh);
    while (sh)
    {
        sh1 = sh->setchain;
        ifree (sh);
        sh = sh1;
    }
}

int inf_BSetHandle (BSetHandle *sh, long *used, long *allocated)
{
    int wsize;

    assert (sh);
    *used = 0;
    *allocated = 0;
    wsize = sh->wsize;
    do
    {
        *used += sh->offset;
        *allocated += sh->chunk;
    } while ((sh = sh->setchain));
    return wsize;
}

BSet mk_BSet (BSetHandle **shp)
{
    BSetHandle *sh, *sh1;
    unsigned off;
    assert (shp);
    sh = *shp;
    assert (sh);

    off = sh->offset;
    if ((off + sh->wsize) > sh->chunk)
    {
        sh1 = (BSetHandle *) imalloc (sizeof(BSetHandle) + 
                                     sh->chunk*sizeof(BSetWord));
        sh1->size = sh->size;
        sh1->wsize = sh->wsize;
        sh1->chunk = sh->chunk;
        off = sh1->offset = 0;
        sh1->setchain = sh;
        sh = *shp = sh1;
    }
    sh->offset = off + sh->wsize;
    return sh->setarray + off;
}

void add_BSet (BSetHandle *sh, BSet dst, unsigned member)
{
    assert (dst);
    assert (sh);
    assert (member <= sh->size);
    SET_BIT(dst, member);
}

unsigned test_BSet (BSetHandle *sh, BSet src, unsigned member)
{
    assert (src);
    assert (sh);
    assert (member <= sh->size);
    return GET_BIT (src , member) != 0;
}

BSet cp_BSet (BSetHandle *sh, BSet dst, BSet src)
{
    assert (sh);
    assert (dst);
    assert (src);
    memcpy (dst, src, sh->wsize * sizeof(BSetWord));
    return dst;
}

void res_BSet (BSetHandle *sh, BSet dst)
{
    assert (dst);
    memset (dst, 0, sh->wsize * sizeof(BSetWord));
}

void union_BSet (BSetHandle *sh, BSet dst, BSet src)
{
    int i;
    assert (sh);
    assert (dst);
    assert (src);
    for (i=sh->wsize; --i >= 0;)
        *dst++ |= *src++;
}

unsigned hash_BSet (BSetHandle *sh, BSet src)
{
    int i;
    unsigned s = 0;
    assert (sh);
    assert (src);
    for (i=sh->wsize; --i >= 0;)
        s += *src++;
    return s;
}

void com_BSet (BSetHandle *sh, BSet dst)
{
    int i;
    assert (sh);
    assert (dst);
    for (i=sh->wsize; --i >= 0; dst++)
        *dst = ~*dst;
}

int eq_BSet (BSetHandle *sh, BSet dst, BSet src)
{
    int i;
    assert (sh);
    assert (dst);
    assert (src);
    for (i=sh->wsize; --i >= 0;)
        if (*dst++ != *src++)
            return 0;
    return 1;
}

int trav_BSet (BSetHandle *sh, BSet src, unsigned member)
{
    int i = sh->size - member;
    BSetWord *sw = src+member/(sizeof(BSetWord)*8);
    unsigned b = member & (sizeof(BSetWord)*8-1);
    while(i >= 0)
        if (b == 0 && *sw == 0)
        {
            member += sizeof(BSetWord)*8;
            ++sw;
            i -= sizeof(BSetWord)*8;
        }
        else if (*sw & (1<<b))
            return member;
        else
        {
            ++member;
            --i;
            if (++b == sizeof(BSetWord)*8)
            {
                b = 0;
                ++sw;
            }
        }
    return -1;
}

int travi_BSet (BSetHandle *sh, BSet src, unsigned member)
{
    int i = sh->size - member;
    BSetWord *sw = src+member/(sizeof(BSetWord)*8);
    unsigned b = member & (sizeof(BSetWord)*8-1);
    while(i >= 0)
        if (b == 0 && *sw == (BSetWord) ~ 0)
        {
            member += sizeof(BSetWord)*8;
            ++sw;
            i -= sizeof(BSetWord)*8;
        }
        else if ((*sw & (1<<b)) == 0)
            return member;
        else
        {
            ++member;
            --i;
            if (++b == sizeof(BSetWord)*8)
            {
                b = 0;
                ++sw;
            }
        }
    return -1;
}


void pr_BSet (BSetHandle *sh, BSet src)
{
    int i;
    assert (sh);
    assert (src);
    for (i=0; (i=trav_BSet(sh,src,i)) != -1; i++)
        printf (" %d", i);
    putchar ('\n');
}

void pr_charBSet (BSetHandle *sh, BSet src, void (*f) (int))
{
    int i0, i1, i;

    assert (sh);
    assert (src);
    i = trav_BSet (sh, src, 0);
    while (i != -1)
    {
        i0 = i;
        if (i == '-')
            f ('\\');
        f(i);
        i1 = trav_BSet (sh, src, ++i);
        if (i1 == i)
        {
            while ((i1=trav_BSet (sh, src, ++i)) == i)
                ;
            if (i != i0+2)
                f ('-');
            if (i-1 == '-')
                f ('\\');
            f(i-1);
        }
        i = i1;
    }
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

