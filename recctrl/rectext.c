/* $Id: rectext.c,v 1.15 2002-08-02 19:26:56 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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


#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <zebrautl.h>
#include "rectext.h"

static void *text_init (RecType recType)
{
    return 0;
}

static void text_destroy (void *clientData)
{
}

struct buf_info {
    struct recExtractCtrl *p;
    char *buf;
    int offset;
    int max;
};

struct buf_info *buf_open (struct recExtractCtrl *p)
{
    struct buf_info *fi = (struct buf_info *) xmalloc (sizeof(*fi));

    fi->p = p;
    fi->buf = (char *) xmalloc (4096);
    fi->offset = 1;
    fi->max = 1;
    return fi;
}

int buf_read (struct buf_info *fi, char *dst)
{
    if (fi->offset >= fi->max)
    {
        if (fi->max <= 0)
            return 0;
        fi->max = (*fi->p->readf)(fi->p->fh, fi->buf, 4096);
        fi->offset = 0;
        if (fi->max <= 0)
            return 0;
    }
    *dst = fi->buf[(fi->offset)++];
    return 1;
}

void buf_close (struct buf_info *fi)
{
    xfree (fi->buf);
    xfree (fi);
}

static int text_extract (void *clientData, struct recExtractCtrl *p)
{
    char w[512];
    RecWord recWord;
    int r;
    struct buf_info *fi = buf_open (p);

    (*p->init)(p, &recWord);
    recWord.reg_type = 'w';
    do
    {
        int i = 0;
            
        r = buf_read (fi, w);
        while (r > 0 && i < 511 && w[i] != '\n' && w[i] != '\r')
        {
            i++;
            r = buf_read (fi, w + i);
        }
        if (i)
        {
            recWord.string = w;
	    recWord.length = i;
            (*p->tokenAdd)(&recWord);
        }
    } while (r > 0);
    buf_close (fi);
    return RECCTRL_EXTRACT_OK;
}

static int text_retrieve (void *clientData, struct recRetrieveCtrl *p)
{
    int r, text_ptr = 0;
    static char *text_buf = NULL;
    static int text_size = 0;
    int start_flag = 1;
    const char *elementSetName = NULL;
    int no_lines = 0;

    if (p->comp && p->comp->which == Z_RecordComp_simple &&
        p->comp->u.simple->which == Z_ElementSetNames_generic)
        elementSetName = p->comp->u.simple->u.generic;

    /* don't make header for the R(aw) element set name */
    if (elementSetName && !strcmp(elementSetName, "R"))
        start_flag = 0;
    while (1)
    {
        if (text_ptr + 4096 >= text_size)
        {
            char *nb;

            text_size = 2*text_size + 8192;
            nb = (char *) xmalloc (text_size);
            if (text_buf)
            {
                memcpy (nb, text_buf, text_ptr);
                xfree (text_buf);
            }
            text_buf = nb;
        }
        if (start_flag)
        {
            start_flag = 0;
            if (p->score >= 0)
            {
                sprintf (text_buf, "Rank: %d\n", p->score);
                text_ptr = strlen(text_buf);
            }
            sprintf (text_buf + text_ptr, "Local Number: %d\n", p->localno);
            text_ptr = strlen(text_buf);
        }
        r = (*p->readf)(p->fh, text_buf + text_ptr, 4096);
        if (r <= 0)
            break;
        text_ptr += r;
    }
    text_buf[text_ptr] = '\0';
    if (elementSetName)
    {
        if (!strcmp (elementSetName, "B"))
            no_lines = 4;
        if (!strcmp (elementSetName, "M"))
            no_lines = 20;
    }
    if (no_lines)
    {
        char *p = text_buf;
        int i = 0;

        while (++i <= no_lines && (p = strchr (p, '\n')))
            p++;
        if (p)
        {
            p[1] = '\0';
            text_ptr = p-text_buf;
        }
    }
    p->output_format = VAL_SUTRS;
    p->rec_buf = text_buf;
    p->rec_len = text_ptr; 
    return 0;
}

static struct recType text_type = {
    "text",
    text_init,
    text_destroy,
    text_extract,
    text_retrieve
};

RecType recTypeText = &text_type;
