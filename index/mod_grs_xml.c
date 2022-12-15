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
#if HAVE_EXPAT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#if HAVE_ICONV_H
#include <errno.h>
#include <iconv.h>
#endif

#include <yaz/log.h>
#include <yaz/snprintf.h>
#include <yaz/log.h>
#include <yaz/xmalloc.h>
#include <idzebra/recgrs.h>

#include <expat.h>

#define XML_CHUNK 1024

struct user_info {
    data1_node *d1_stack[256];
    int level;
    data1_handle dh;
    NMEM nmem;
    int loglevel;
};

static void report_xml_error(XML_Parser parser)
{
    zint line = XML_GetCurrentLineNumber(parser);
    zint col = XML_GetCurrentColumnNumber(parser);
    yaz_log(YLOG_WARN, ZINT_FORMAT ":" ZINT_FORMAT ":XML error: %s",
            line, col, XML_ErrorString(XML_GetErrorCode(parser)));
}

static void cb_start(void *user, const char *el, const char **attr)
{
    struct user_info *ui = (struct user_info*) user;
    if (ui->level == 1)
        data1_set_root (ui->dh, ui->d1_stack[0], ui->nmem, el);
    ui->d1_stack[ui->level] = data1_mk_tag(ui->dh, ui->nmem, el, attr,
                                           ui->d1_stack[ui->level-1]);
    ui->level++;
    yaz_log (ui->loglevel, "cb_start %s", el);
}

static void cb_end (void *user, const char *el)
{
    struct user_info *ui = (struct user_info*) user;

    ui->level--;
    yaz_log(ui->loglevel, "cb_end %s", el);
}

static void cb_chardata(void *user, const char *s, int len)
{
    struct user_info *ui = (struct user_info*) user;
#if 0
    yaz_log (ui->loglevel, "cb_chardata %.*s", len, s);
#endif
    ui->d1_stack[ui->level] = data1_mk_text_n(ui->dh, ui->nmem, s, len,
                                              ui->d1_stack[ui->level -1]);
}

static void cb_decl(void *user, const char *version, const char *encoding,
                    int standalone)
{
    struct user_info *ui = (struct user_info*) user;
    const char *attr_list[7];

    attr_list[0] = "version";
    attr_list[1] = version;

    attr_list[2] = "encoding";
    attr_list[3] = "UTF-8"; /* internally it's always UTF-8 */

    attr_list[4] = "standalone";
    attr_list[5] = standalone  ? "yes" : "no";

    attr_list[6] = 0;

    data1_mk_preprocess(ui->dh, ui->nmem, "xml", attr_list,
                        ui->d1_stack[ui->level-1]);
#if 0
    yaz_log (YLOG_LOG, "decl version=%s encoding=%s",
             version ? version : "null",
             encoding ? encoding : "null");
#endif
}

static void cb_processing(void *user, const char *target,
                          const char *data)
{
    struct user_info *ui = (struct user_info*) user;
    data1_node *res =
        data1_mk_preprocess(ui->dh, ui->nmem, target, 0,
                            ui->d1_stack[ui->level-1]);
    data1_mk_text_nf(ui->dh, ui->nmem, data, strlen(data), res);

    yaz_log(ui->loglevel, "decl processing target=%s data=%s",
            target ? target : "null",
            data ? data : "null");
}

static void cb_comment(void *user, const char *data)
{
    struct user_info *ui = (struct user_info*) user;
    yaz_log(ui->loglevel, "decl comment data=%s", data ? data : "null");
    data1_mk_comment(ui->dh, ui->nmem, data, ui->d1_stack[ui->level-1]);
}

static void cb_doctype_start(void *userData, const char *doctypeName,
                              const char *sysid, const char *pubid,
                              int has_internal_subset)
{
    struct user_info *ui = (struct user_info*) userData;
    yaz_log(ui->loglevel, "doctype start doctype=%s sysid=%s pubid=%s",
            doctypeName, sysid, pubid);
}

static void cb_doctype_end(void *userData)
{
    struct user_info *ui = (struct user_info*) userData;
    yaz_log(ui->loglevel, "doctype end");
}


static void cb_entity_decl(void *userData, const char *entityName,
                           int is_parameter_entity,
                           const char *value, int value_length,
                           const char *base, const char *systemId,
                           const char *publicId, const char *notationName)
{
    struct user_info *ui = (struct user_info*) userData;
    yaz_log(ui->loglevel,
            "entity decl %s is_para_entry=%d value=%.*s base=%s systemId=%s"
            " publicId=%s notationName=%s",
            entityName, is_parameter_entity, value_length, value,
            base, systemId, publicId, notationName);

}

static int cb_external_entity(XML_Parser pparser,
                              const char *context,
                              const char *base,
                              const char *systemId,
                              const char *publicId)
{
    struct user_info *ui = (struct user_info*) XML_GetUserData(pparser);
    FILE *inf;
    int done = 0;
    XML_Parser parser;

    yaz_log(ui->loglevel,
            "external entity context=%s base=%s systemid=%s publicid=%s",
            context, base, systemId, publicId);
    if (!systemId)
        return 1;

    if (!(inf = fopen(systemId, "rb")))
    {
        yaz_log (YLOG_WARN|YLOG_ERRNO, "fopen %s", systemId);
        return 0;
    }

    parser = XML_ExternalEntityParserCreate(pparser, "", 0);
    while (!done)
    {
        int r;
        void *buf = XML_GetBuffer(parser, XML_CHUNK);
        if (!buf)
        {
            yaz_log(YLOG_WARN, "XML_GetBuffer fail");
            break;
        }
        r = fread(buf, 1, XML_CHUNK, inf);
        if (r == 0)
        {
            if (ferror(inf))
            {
                yaz_log(YLOG_WARN|YLOG_ERRNO, "fread %s", systemId);
                break;
            }
            done = 1;
        }
        if (!XML_ParseBuffer(parser, r, done))
        {
            done = 1;
            report_xml_error(parser);
        }
    }
    fclose (inf);
    XML_ParserFree(parser);
    return done;
}


#if HAVE_ICONV_H
static int cb_encoding_convert(void *data, const char *s)
{
    iconv_t t = (iconv_t) data;
    size_t ret;
    size_t outleft = 2;
    char outbuf_[2], *outbuf = outbuf_;
    size_t inleft = 4;
    char *inbuf = (char *) s;
    unsigned short code;

#if 1
    yaz_log(YLOG_LOG, "------------------------- cb_encoding_convert --- ");
#endif
    ret = iconv(t, &inbuf, &inleft, &outbuf, &outleft);
    if (ret == (size_t) (-1) && errno != E2BIG)
    {
        iconv (t, 0, 0, 0, 0);
        return -1;
    }
    if (outleft != 0)
        return -1;
    memcpy (&code, outbuf_, sizeof(short));
    return code;
}

static void cb_encoding_release(void *data)
{
    iconv_t t = (iconv_t) data;
    iconv_close (t);
}

static int cb_encoding_handler(void *userData, const char *name,
                               XML_Encoding *info)
{
    int i = 0;
    int no_ok = 0;
    struct user_info *ui = (struct user_info*) userData;

    iconv_t t = iconv_open("UNICODE", name);
    if (t == (iconv_t) (-1))
        return 0;

    info->data = 0;  /* signal that multibyte is not in use */
    yaz_log(ui->loglevel, "Encoding handler of %s", name);
    for (i = 0; i<256; i++)
    {
        size_t ret;
        char outbuf_[5];
        char inbuf_[5];
        char *inbuf = inbuf_;
        char *outbuf = outbuf_;
        size_t inleft = 1;
        size_t outleft = 2;
        inbuf_[0] = i;

        iconv (t, 0, 0, 0, 0);  /* reset iconv */

        ret = iconv(t, &inbuf, &inleft, &outbuf, &outleft);
        if (ret == (size_t) (-1))
        {
            if (errno == EILSEQ)
            {
                yaz_log(ui->loglevel, "Encoding %d: invalid sequence", i);
                info->map[i] = -1;  /* invalid sequence */
            }
            if (errno == EINVAL)
            {                       /* multi byte input */
                int len = 2;
                int j = 0;
                info->map[i] = -1;

                while (len <= 4)
                {
                    inbuf = inbuf_;
                    inleft = len;
                    outbuf = outbuf_;
                    outleft = 2;

                    inbuf_[len-1] = j;
                    iconv (t, 0,0,0,0);

                    assert (i >= 0 && i<255);

                    ret = iconv(t, &inbuf, &inleft, &outbuf, &outleft);
                    if (ret == (size_t) (-1))
                    {
                        if (errno == EILSEQ || errno == E2BIG)
                        {
                            j++;
                            if (j > 255)
                                break;
                        }
                        else if (errno == EINVAL)
                        {
                            len++;
                            j = 7;
                        }
                    }
                    else if (outleft == 0)
                    {
                        info->map[i] = -len;
                        info->data = t;  /* signal that multibyte is in use */
                        break;
                    }
                    else
                    {
                        break;
                    }
                }
                if (info->map[i] < -1)
                    yaz_log(ui->loglevel, "Encoding %d: multibyte input %d",
                            i, -info->map[i]);
                else
                    yaz_log(ui->loglevel, "Encoding %d: multibyte input failed",
                            i);
            }
            if (errno == E2BIG)
            {
                info->map[i] = -1;  /* no room for output */
                if (i != 0)
                    yaz_log(YLOG_WARN, "Encoding %d: no room for output",
                            i);
            }
        }
        else if (outleft == 0)
        {
            unsigned short code;
            memcpy (&code, outbuf_, sizeof(short));
            info->map[i] = code;
            no_ok++;
        }
        else
        {   /* should never happen */
            info->map[i] = -1;
            yaz_log (YLOG_DEBUG, "Encoding %d: bad state", i);
        }
    }
    if (info->data)
    {   /* at least one multi byte */
        info->convert = cb_encoding_convert;
        info->release = cb_encoding_release;
    }
    else
    {
        /* no multi byte - we no longer need iconv handler */
        iconv_close(t);
        info->convert = 0;
        info->release = 0;
    }
    if (!no_ok)
        return 0;
    return 1;
}
/* HAVE_ICONV_H */
#endif

static void cb_ns_start(void *userData, const char *prefix, const char *uri)
{
    struct user_info *ui = (struct user_info*) userData;
    if (prefix && uri)
        yaz_log(ui->loglevel, "cb_ns_start %s %s", prefix, uri);
}

static void cb_ns_end(void *userData, const char *prefix)
{
    struct user_info *ui = (struct user_info*) userData;
    if (prefix)
        yaz_log(ui->loglevel, "cb_ns_end %s", prefix);
}

data1_node *zebra_read_xml(data1_handle dh,
                           struct ZebraRecStream *stream,
                           NMEM m)
{
    XML_Parser parser;
    struct user_info uinfo;
    int done = 0;
    data1_node *first_node;
    int no_read = 0;

    uinfo.loglevel = YLOG_DEBUG;
    uinfo.level = 1;
    uinfo.dh = dh;
    uinfo.nmem = m;
    uinfo.d1_stack[0] = data1_mk_node2 (dh, m, DATA1N_root, 0);
    uinfo.d1_stack[1] = 0; /* indicate no children (see end of routine) */

    parser = XML_ParserCreate (0 /* encoding */);

    XML_SetElementHandler(parser, cb_start, cb_end);
    XML_SetCharacterDataHandler(parser, cb_chardata);
    XML_SetXmlDeclHandler(parser, cb_decl);
    XML_SetProcessingInstructionHandler(parser, cb_processing);
    XML_SetUserData(parser, &uinfo);
    XML_SetCommentHandler(parser, cb_comment);
    XML_SetDoctypeDeclHandler(parser, cb_doctype_start, cb_doctype_end);
    XML_SetEntityDeclHandler(parser, cb_entity_decl);
    XML_SetExternalEntityRefHandler(parser, cb_external_entity);
    XML_SetNamespaceDeclHandler(parser, cb_ns_start, cb_ns_end);
#if HAVE_ICONV_H
    XML_SetUnknownEncodingHandler(parser, cb_encoding_handler, &uinfo);
#endif
    while (!done)
    {
        int r;
        void *buf = XML_GetBuffer(parser, XML_CHUNK);
        if (!buf)
        {
            /* error */
            yaz_log(YLOG_WARN, "XML_GetBuffer fail");
            break;
        }
        r = stream->readf(stream, buf, XML_CHUNK);
        if (r < 0)
        {
            /* error */
            yaz_log(YLOG_WARN, "XML read fail");
            break;
        }
        else if (r == 0)
            done = 1;
        else
            no_read += r;
        if (no_read && !XML_ParseBuffer(parser, r, done))
        {
            done = 1;
            report_xml_error(parser);
        }
    }
    XML_ParserFree(parser);
    if (no_read == 0)
        return 0;
    if (!uinfo.d1_stack[1] || !done)
        return 0;
    /* insert XML header if not present .. */
    first_node = uinfo.d1_stack[0]->child;
    if (first_node->which != DATA1N_preprocess ||
        strcmp(first_node->u.preprocess.target, "xml"))
    {
        const char *attr_list[5];

        attr_list[0] = "version";
        attr_list[1] = "1.0";

        attr_list[2] = "encoding";
        attr_list[3] = "UTF-8"; /* encoding */

        attr_list[4] = 0;

        data1_insert_preprocess(uinfo.dh, uinfo.nmem, "xml", attr_list,
                                uinfo.d1_stack[0]);
    }
    return uinfo.d1_stack[0];
}

struct xml_info {
    XML_Expat_Version expat_version;
};

static data1_node *grs_read_xml(struct grs_read_info *p)
{
    return zebra_read_xml(p->dh, p->stream, p->mem);
}

static void *filter_init(Res res, RecType recType)
{
    struct xml_info *p = (struct xml_info *) xmalloc (sizeof(*p));

    p->expat_version = XML_ExpatVersionInfo();

    return p;
}

static void filter_destroy(void *clientData)
{
    struct xml_info *p = (struct xml_info *) clientData;

    xfree (p);
}

static int filter_extract(void *clientData, struct recExtractCtrl *ctrl)
{
    return zebra_grs_extract(clientData, ctrl, grs_read_xml);
}

static int filter_retrieve(void *clientData, struct recRetrieveCtrl *ctrl)
{
    return zebra_grs_retrieve(clientData, ctrl, grs_read_xml);
}

static struct recType filter_type = {
    0,
    "grs.xml",
    filter_init,
    0,
    filter_destroy,
    filter_extract,
    filter_retrieve,
};

RecType
#if IDZEBRA_STATIC_GRS_XML
idzebra_filter_grs_xml
#else
idzebra_filter
#endif

[] = {
    &filter_type,
    0,
};

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

