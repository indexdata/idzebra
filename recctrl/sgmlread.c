/* $Id: sgmlread.c,v 1.11.2.2 2006-08-14 10:39:17 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include <assert.h>
#include <yaz/log.h>

#include "grsread.h"

struct sgml_getc_info {
    char *buf;
    int buf_size;
    int size;
    int off;
    off_t moffset;
    void *fh;
    int (*readf)(void *, char *, size_t);
    WRBUF wrbuf;
};

int sgml_getc (void *clientData)
{
    struct sgml_getc_info *p = (struct sgml_getc_info *) clientData;
    int res;
    
    if (p->off < p->size)
	return p->buf[(p->off)++];
    if (p->size < p->buf_size)
	return 0;
    p->moffset += p->off;
    p->off = 0;
    p->size = 0;
    res = (*p->readf)(p->fh, p->buf, p->buf_size);
    if (res > 0)
    {
	p->size += res;
	return p->buf[(p->off)++];
    }
    return 0;
}

static data1_node *grs_read_sgml (struct grs_read_info *p)
{
    struct sgml_getc_info *sgi = (struct sgml_getc_info *) p->clientData;
    data1_node *node;
    int res;
    
    sgi->moffset = p->offset;
    sgi->fh = p->fh;
    sgi->readf = p->readf;
    sgi->off = 0;
    sgi->size = 0;
    res = (*sgi->readf)(sgi->fh, sgi->buf, sgi->buf_size);
    if (res > 0)
	sgi->size += res;
    else
	return 0;
    node = data1_read_nodex (p->dh, p->mem, sgml_getc, sgi, sgi->wrbuf);
    if (node && p->endf)
	(*p->endf)(sgi->fh, sgi->moffset + sgi->off);
    return node;
}

static void *grs_init_sgml(void)
{
    struct sgml_getc_info *p = (struct sgml_getc_info *) xmalloc (sizeof(*p));
    p->buf_size = 512;
    p->buf = xmalloc (p->buf_size);
    p->wrbuf = wrbuf_alloc();
    return p;
}

static void grs_destroy_sgml(void *clientData)
{
    struct sgml_getc_info *p = (struct sgml_getc_info *) clientData;

    wrbuf_free(p->wrbuf, 1);
    xfree (p->buf);
    xfree (p);
}

static struct recTypeGrs sgml_type = {
    "sgml",
    grs_init_sgml,
    grs_destroy_sgml,
    grs_read_sgml
};

RecTypeGrs recTypeGrs_sgml = &sgml_type;

