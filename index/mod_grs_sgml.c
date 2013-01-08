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
#include <assert.h>
#include <yaz/log.h>

#include <idzebra/recgrs.h>

struct sgml_getc_info {
    char *buf;
    int buf_size;
    int size;
    int off;
    struct ZebraRecStream *stream;
    off_t moffset;
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
    res = p->stream->readf(p->stream, p->buf, p->buf_size);
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

    sgi->moffset = p->stream->tellf(p->stream);
    sgi->stream = p->stream;
    sgi->off = 0;
    sgi->size = 0;
    res = sgi->stream->readf(sgi->stream, sgi->buf, sgi->buf_size);
    if (res > 0)
	sgi->size += res;
    else
	return 0;
    node = data1_read_nodex(p->dh, p->mem, sgml_getc, sgi, sgi->wrbuf);
    if (node && p->stream->endf)
    {
        off_t end_offset = sgi->moffset + sgi->off;
	p->stream->endf(sgi->stream, &end_offset);
    }
    return node;
}

static void *grs_init_sgml(Res res, RecType recType)
{
    struct sgml_getc_info *p = (struct sgml_getc_info *) xmalloc (sizeof(*p));
    p->buf_size = 512;
    p->buf = xmalloc (p->buf_size);
    p->wrbuf = wrbuf_alloc();
    return p;
}

static ZEBRA_RES grs_config_sgml(void *clientData, Res res, const char *args)
{
    return ZEBRA_OK;
}

static void grs_destroy_sgml(void *clientData)
{
    struct sgml_getc_info *p = (struct sgml_getc_info *) clientData;

    wrbuf_destroy(p->wrbuf);
    xfree(p->buf);
    xfree(p);
}

static int grs_extract_sgml(void *clientData, struct recExtractCtrl *ctrl)
{
    return zebra_grs_extract(clientData, ctrl, grs_read_sgml);
}

static int grs_retrieve_sgml(void *clientData, struct recRetrieveCtrl *ctrl)
{
    return zebra_grs_retrieve(clientData, ctrl, grs_read_sgml);
}

static struct recType grs_type_sgml =
{
    0,
    "grs.sgml",
    grs_init_sgml,
    grs_config_sgml,
    grs_destroy_sgml,
    grs_extract_sgml,
    grs_retrieve_sgml
};

RecType
#if IDZEBRA_STATIC_GRS_SGML
idzebra_filter_grs_sgml
#else
idzebra_filter
#endif

[] = {
    &grs_type_sgml,
    0,
};
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

