/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.h,v $
 * Revision 1.10  1999-02-02 14:50:36  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.9  1997/11/18 10:04:03  adam
 * Function res_trav returns number of 'hits'.
 *
 * Revision 1.8  1997/09/17 12:19:10  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.7  1997/09/05 15:30:02  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.6  1996/10/29 13:44:12  adam
 * Added res_get_match.
 *
 * Revision 1.5  1994/09/16 14:37:46  quinn
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

#ifdef __cplusplus
extern "C" {
#endif

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
int res_get_match (Res r, const char *name, const char *value, const char *s);
void res_put (Res r, const char *name, const char *value);
int res_trav (Res r, const char *prefix, void *p,
	      void (*f)(void *p, const char *name, const char *value));
int res_write (Res r);

#ifdef __cplusplus
}
#endif

#endif
