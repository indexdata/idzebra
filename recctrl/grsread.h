/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: grsread.h,v $
 * Revision 1.2  1997-04-30 08:56:08  quinn
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
    void (*endf)(void *, off_t);
    void *fh;
    off_t offset;
    char type[80];
    NMEM mem;
};

data1_node *grs_read_regx (struct grs_read_info *p);
data1_node *grs_read_sgml (struct grs_read_info *p);
#endif
