/* $Id: alvis.c,v 1.3 2005-06-23 06:45:47 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/


#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <idzebra/util.h>
#include <idzebra/recctrl.h>

struct filter_info {
    char *sep;
};

static void *filter_init (Res res, RecType recType)
{
    struct filter_info *tinfo = (struct filter_info *) xmalloc(sizeof(*tinfo));
    tinfo->sep = 0;
    return tinfo;
}

static void filter_config(void *clientData, Res res, const char *args)
{

}

static void filter_destroy (void *clientData)
{
    struct filter_info *tinfo = clientData;
    xfree (tinfo->sep);
    xfree (tinfo);
}

struct buf_info {
    struct recExtractCtrl *p;
    char *buf;
    int offset;
    int max;
};

static struct buf_info *buf_open (struct recExtractCtrl *p)
{
    struct buf_info *fi = (struct buf_info *) xmalloc (sizeof(*fi));

    fi->p = p;
    fi->buf = (char *) xmalloc (4096);
    fi->offset = 1;
    fi->max = 1;
    return fi;
}

static int buf_read (struct filter_info *tinfo, struct buf_info *fi, char *dst)
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
    if (tinfo->sep && *dst == *tinfo->sep)
    {
	off_t off = (*fi->p->tellf)(fi->p->fh);
	(*fi->p->endf)(fi->p->fh, off - (fi->max - fi->offset));
	return 0;
    }
    return 1;
}

static void buf_close (struct buf_info *fi)
{
    xfree (fi->buf);
    xfree (fi);
}

static int filter_extract (void *clientData, struct recExtractCtrl *p)
{
    struct filter_info *tinfo = clientData;
    char w[512];
    RecWord recWord;
    int r;
    struct buf_info *fi = buf_open (p);

#if 0
    yaz_log(YLOG_LOG, "filter_extract off=%ld",
	    (long) (*fi->p->tellf)(fi->p->fh));
#endif
    xfree(tinfo->sep);
    tinfo->sep = 0;
    (*p->init)(p, &recWord);
    do
    {
        int i = 0;
            
        r = buf_read (tinfo, fi, w);
        while (r > 0 && i < 511 && w[i] != '\n' && w[i] != '\r')
        {
            i++;
            r = buf_read (tinfo, fi, w + i); 
	}
        if (i)
        {
            recWord.term_buf = w;
	    recWord.term_len = i;
            (*p->tokenAdd)(&recWord);
        }
    } while (r > 0);
    buf_close (fi);
    return RECCTRL_EXTRACT_OK;
}

static int filter_retrieve (void *clientData, struct recRetrieveCtrl *p)
{
    int r, filter_ptr = 0;
    static char *filter_buf = NULL;
    static int filter_size = 0;
    int make_header = 1;
    int make_body = 1;
    const char *elementSetName = NULL;
    int no_lines = 0;

    if (p->comp && p->comp->which == Z_RecordComp_simple &&
        p->comp->u.simple->which == Z_ElementSetNames_generic)
        elementSetName = p->comp->u.simple->u.generic;

    if (elementSetName)
    {
	/* don't make header for the R(aw) element set name */
	if (!strcmp(elementSetName, "R"))
	{
	    make_header = 0;
	    make_body = 1;
	}
	/* only make header for the H(eader) element set name */
	else if (!strcmp(elementSetName, "H"))
	{
	    make_header = 1;
	    make_body = 0;
	}
    }
    while (1)
    {
        if (filter_ptr + 4096 >= filter_size)
        {
            char *nb;

            filter_size = 2*filter_size + 8192;
            nb = (char *) xmalloc (filter_size);
            if (filter_buf)
            {
                memcpy (nb, filter_buf, filter_ptr);
                xfree (filter_buf);
            }
            filter_buf = nb;
        }
        if (make_header && filter_ptr == 0)
        {
            if (p->score >= 0)
            {
                sprintf (filter_buf, "Rank: %d\n", p->score);
                filter_ptr = strlen(filter_buf);
            }
            sprintf (filter_buf + filter_ptr, "Local Number: " ZINT_FORMAT "\n",
		     p->localno);
            filter_ptr = strlen(filter_buf);
	    if (p->fname)
	    {
		sprintf (filter_buf + filter_ptr, "Filename: %s\n", p->fname);
		filter_ptr = strlen(filter_buf);
	    }
	    strcpy(filter_buf+filter_ptr++, "\n");
        }
	if (!make_body)
	    break;
        r = (*p->readf)(p->fh, filter_buf + filter_ptr, 4096);
        if (r <= 0)
            break;
        filter_ptr += r;
    }
    filter_buf[filter_ptr] = '\0';
    if (elementSetName)
    {
        if (!strcmp (elementSetName, "B"))
            no_lines = 4;
        if (!strcmp (elementSetName, "M"))
            no_lines = 20;
    }
    if (no_lines)
    {
        char *p = filter_buf;
        int i = 0;

        while (++i <= no_lines && (p = strchr (p, '\n')))
            p++;
        if (p)
        {
            p[1] = '\0';
            filter_ptr = p-filter_buf;
        }
    }
    p->output_format = VAL_SUTRS;
    p->rec_buf = filter_buf;
    p->rec_len = filter_ptr; 
    return 0;
}

static struct recType filter_type = {
    0,
    "alvis",
    filter_init,
    filter_config,
    filter_destroy,
    filter_extract,
    filter_retrieve
};

RecType
#ifdef IDZEBRA_STATIC_ALVIS
idzebra_filter_alvis
#else
idzebra_filter
#endif

[] = {
    &filter_type,
    0,
};
