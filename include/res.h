/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.h,v $
 * Revision 1.2  1994-08-18 08:22:26  adam
 * Res.h modified. xmalloc now declares xstrdup.
 *
 */

#ifndef RES_H
#define RES_H

struct res_entry {
    char *name;
    char *value;
    struct res_entry *next;
};

typedef struct res_struct {
    struct res_entry *first, *last;
    char *name;
    int  init;
} *Res;

#endif
