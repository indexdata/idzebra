/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.h,v $
 * Revision 1.1  1994-08-17 15:34:14  adam
 * Initial version of resource manager.
 *
 */

#ifndef RES_H
#define RES_H

const char *res_get (const char *name);
const char *res_put (const char *name, const char *value);
int res_write (void);
#endif
