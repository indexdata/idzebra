/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: sgmlread.c,v $
 * Revision 1.9  1999-07-14 10:56:16  adam
 * Filter handles multiple records in one file.
 *
 * Revision 1.8  1999/06/25 13:47:25  adam
 * Minor change that prevents MSVC warning.
 *
 * Revision 1.7  1999/05/21 12:00:17  adam
 * Better diagnostics for extraction process.
 *
 * Revision 1.6  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.5  1999/02/02 14:51:31  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.4  1997/09/17 12:19:22  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.3  1997/09/04 13:54:41  adam
 * Added MARC filter - type grs.marc.<syntax> where syntax refers
 * to abstract syntax. New method tellf in retrieve/extract method.
 *
 * Revision 1.2  1997/04/30 08:56:08  quinn
 * null
 *
 * Revision 1.1  1996/10/11  10:57:32  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 */
#include <assert.h>
#include <log.h>

#include "grsread.h"

struct sgml_getc_info {
    char *buf;
    int buf_size;
    int size;
    int off;
    int moffset;
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

