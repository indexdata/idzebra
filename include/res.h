/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: res.h,v 1.11 2002-04-04 14:14:13 adam Exp $
 */

#ifndef RES_H
#define RES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct res_struct *Res;

Res res_open (const char *name, Res res_def);
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
