/* $Id: safari.c,v 1.6 2005-03-30 09:25:24 adam Exp $
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

struct safari_info {
    char *sep;
};

static void *safari_init (Res res, RecType recType)
{
    struct safari_info *tinfo = (struct safari_info *) xmalloc(sizeof(*tinfo));
    tinfo->sep = 0;
    return tinfo;
}

static void safari_config(void *clientData, Res res, const char *args)
{

}

static void safari_destroy(void *clientData)
{
    struct safari_info *tinfo = clientData;
    xfree (tinfo->sep);
    xfree (tinfo);
}

struct fi_info {
    struct recExtractCtrl *p;
    char *buf;
    int offset;
    int max;
};

struct fi_info *fi_open(struct recExtractCtrl *p)
{
    struct fi_info *fi = (struct fi_info *) xmalloc (sizeof(*fi));

    fi->p = p;
    fi->buf = (char *) xmalloc (4096);
    fi->offset = 1;
    fi->max = 1;
    return fi;
}

int fi_getchar(struct fi_info *fi, char *dst)
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

int fi_gets(struct fi_info *fi, char *dst, int max)
{
    int l;
    for (l = 0; l < max; l++)
    {
	if (!fi_getchar(fi, dst+l))
	    return 0;
	if (dst[l] == '\n')
	    break;
    }
    dst[l] = '\0';
    return 1;
}

void fi_close (struct fi_info *fi)
{
    xfree (fi->buf);
    xfree (fi);
}

static int safari_extract(void *clientData, struct recExtractCtrl *p)
{
    struct safari_info *tinfo = clientData;
    char line[512];
    RecWord recWord;
    struct fi_info *fi = fi_open(p);

#if 0
    yaz_log(YLOG_LOG, "safari_extract off=%ld",
	    (long) (*fi->p->tellf)(fi->p->fh));
#endif
    xfree(tinfo->sep);
    tinfo->sep = 0;
    (*p->init)(p, &recWord);

    if (!fi_gets(fi, line, sizeof(line)-1))
	return RECCTRL_EXTRACT_ERROR_GENERIC;
    sscanf(line, "%255s", p->match_criteria);
    
    recWord.reg_type = 'w';
    while (fi_gets(fi, line, sizeof(line)-1))
    {
	int nor = 0;
	char field[40];
	char *cp;
#if 0
	yaz_log(YLOG_LOG, "safari line: %s", line);
#endif
	if (sscanf(line, ZINT_FORMAT " " ZINT_FORMAT " " ZINT_FORMAT " %39s %n",
		   &recWord.record_id, &recWord.section_id, &recWord.seqno,
		   field, &nor) < 4)
	{
	    yaz_log(YLOG_WARN, "Bad safari record line: %s", line);
	    return RECCTRL_EXTRACT_ERROR_GENERIC;
	}
	for (cp = line + nor; *cp == ' '; cp++)
	    ;
	recWord.attrStr = field;
	recWord.term_buf = cp;
	recWord.term_len = strlen(cp);
	(*p->tokenAdd)(&recWord);
    }
    fi_close(fi);
    return RECCTRL_EXTRACT_OK;
}

static int safari_retrieve (void *clientData, struct recRetrieveCtrl *p)
{
    int r, safari_ptr = 0;
    static char *safari_buf = NULL;
    static int safari_size = 0;
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
        if (safari_ptr + 4096 >= safari_size)
        {
            char *nb;

            safari_size = 2*safari_size + 8192;
            nb = (char *) xmalloc (safari_size);
            if (safari_buf)
            {
                memcpy (nb, safari_buf, safari_ptr);
                xfree (safari_buf);
            }
            safari_buf = nb;
        }
        if (make_header && safari_ptr == 0)
        {
            if (p->score >= 0)
            {
                sprintf (safari_buf, "Rank: %d\n", p->score);
                safari_ptr = strlen(safari_buf);
            }
            sprintf (safari_buf + safari_ptr, "Local Number: " ZINT_FORMAT "\n",
		     p->localno);
            safari_ptr = strlen(safari_buf);
	    if (p->fname)
	    {
		sprintf (safari_buf + safari_ptr, "Filename: %s\n", p->fname);
		safari_ptr = strlen(safari_buf);
	    }
	    strcpy(safari_buf+safari_ptr++, "\n");
        }
	if (!make_body)
	    break;
        r = (*p->readf)(p->fh, safari_buf + safari_ptr, 4096);
        if (r <= 0)
            break;
        safari_ptr += r;
    }
    safari_buf[safari_ptr] = '\0';
    if (elementSetName)
    {
        if (!strcmp (elementSetName, "B"))
            no_lines = 4;
        if (!strcmp (elementSetName, "M"))
            no_lines = 20;
    }
    if (no_lines)
    {
        char *p = safari_buf;
        int i = 0;

        while (++i <= no_lines && (p = strchr (p, '\n')))
            p++;
        if (p)
        {
            p[1] = '\0';
            safari_ptr = p-safari_buf;
        }
    }
    p->output_format = VAL_SUTRS;
    p->rec_buf = safari_buf;
    p->rec_len = safari_ptr; 
    return 0;
}

static struct recType safari_type = {
    "safari",
    safari_init,
    safari_config,
    safari_destroy,
    safari_extract,
    safari_retrieve
};

RecType
#ifdef IDZEBRA_STATIC_SAFARI
idzebra_filter_safari
#else
idzebra_filter
#endif

[] = {
    &safari_type,
    0,
};
