/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.h,v $
 * Revision 1.5  1994-09-16 14:37:46  quinn
 * added res_get_def
 *
 * Revision 1.4  1994/09/06  13:02:29  quinn
 * Removed const from res_get
 *
 * Revision 1.3  1994/08/18  09:43:04  adam
 * Added res_trav. Major changes of prototypes.
 *
 * Revision 1.2  1994/08/18  08:22:26  adam
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

Res res_open (const char *name);
void res_close (Res r);
char *res_get (Res r, const char *name);
char *res_get_def (Res r, const char *name, char *def);
void res_put (Res r, const char *name, const char *value);
void res_trav (Res r, const char *prefix, 
               void (*f)(const char *name, const char *value));
int res_write (Res r);
#endif
