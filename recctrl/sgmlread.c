/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: sgmlread.c,v $
 * Revision 1.7  1999-05-21 12:00:17  adam
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
#include <log.h>

#include "grsread.h"

static data1_node *grs_read_sgml (struct grs_read_info *p)
{
    return data1_read_record (p->dh, p->readf, p->fh, p->mem);
}

static void *grs_init_sgml()
{
    return 0;
}

static void grs_destroy_sgml(void *clientData)
{
}

static struct recTypeGrs sgml_type = {
    "sgml",
    grs_init_sgml,
    grs_destroy_sgml,
    grs_read_sgml
};

RecTypeGrs recTypeGrs_sgml = &sgml_type;

