/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: grsread.h,v $
 * Revision 1.5  1999-02-02 14:51:26  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.4  1997/09/17 12:19:21  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.3  1997/09/04 13:54:40  adam
 * Added MARC filter - type grs.marc.<syntax> where syntax refers
 * to abstract syntax. New method tellf in retrieve/extract method.
 *
 * Revision 1.2  1997/04/30 08:56:08  quinn
 * null
 *
 * Revision 1.1  1996/10/11  10:57:23  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 */

#ifndef GRSREAD_H
#define GRSREAD_H

#include <data1.h>
struct grs_read_info {
    int (*readf)(void *, char *, size_t);
    off_t (*seekf)(void *, off_t);
    off_t (*tellf)(void *);
    void (*endf)(void *, off_t);
    void *fh;
    off_t offset;
    char type[80];
    NMEM mem;
    data1_handle dh;
};

data1_node *grs_read_regx (struct grs_read_info *p);
data1_node *grs_read_sgml (struct grs_read_info *p);
data1_node *grs_read_marc (struct grs_read_info *p);
#endif
