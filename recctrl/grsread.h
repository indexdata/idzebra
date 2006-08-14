/* $Id: grsread.h,v 1.14.2.2 2006-08-14 10:39:16 adam Exp $
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



#ifndef GRSREAD_H
#define GRSREAD_H

#include <sys/types.h>
#include <stdlib.h>
#include <data1.h>

#ifdef __cplusplus
extern "C" {
#endif

struct grs_read_info {
    void *clientData;
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

typedef struct recTypeGrs {
    char *type;
    void *(*init)(void);
    void (*destroy)(void *clientData);
    data1_node *(*read)(struct grs_read_info *p);
} *RecTypeGrs;

extern RecTypeGrs recTypeGrs_sgml;
extern RecTypeGrs recTypeGrs_regx;
extern RecTypeGrs recTypeGrs_tcl;
extern RecTypeGrs recTypeGrs_marc;
extern RecTypeGrs recTypeGrs_marcxml;
extern RecTypeGrs recTypeGrs_xml;
extern RecTypeGrs recTypeGrs_perl;
extern RecTypeGrs recTypeGrs_danbib;

#ifdef __cplusplus
}
#endif

#endif
