/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: sgmlread.c,v $
 * Revision 1.5  1999-02-02 14:51:31  adam
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

data1_node *grs_read_sgml (struct grs_read_info *p)
{
    return data1_read_record (p->dh, p->readf, p->fh, p->mem);
}
