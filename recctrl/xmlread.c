/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 *
 * $Id: xmlread.c,v 1.1 2002-05-13 14:13:43 adam Exp $
 */

#if YAZ_HAVE_EXPAT

#include <assert.h>
#include <yaz/log.h>

#include "grsread.h"

struct xml_info {
    int dummy;
};

static void *grs_init_xml(void)
{
    struct xml_info *p = (struct xml_info *) xmalloc (sizeof(*p));
    return p;
}

static data1_node *grs_read_xml (struct grs_read_info *p)
{
    return data1_read_xml (p->dh, p->readf, p->fh, p->mem);
}

static void grs_destroy_xml(void *clientData)
{
    struct sgml_getc_info *p = (struct sgml_getc_info *) clientData;

    xfree (p);
}

static struct recTypeGrs xml_type = {
    "xml",
    grs_init_xml,
    grs_destroy_xml,
    grs_read_xml
};

RecTypeGrs recTypeGrs_xml = &xml_type;

#endif
