/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: grsread.h,v 1.9 2002-05-13 14:13:43 adam Exp $
 */

#ifndef GRSREAD_H
#define GRSREAD_H

#include <yaz/data1.h>

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
extern RecTypeGrs recTypeGrs_xml;

#ifdef __cplusplus
}
#endif

#endif
