/* $Id: xmlread.c,v 1.2 2002-08-02 19:26:56 adam Exp $
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
