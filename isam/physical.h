/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: physical.h,v $
 * Revision 1.4  1997-09-05 15:30:10  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.3  1994/09/26 17:11:32  quinn
 * Trivial
 *
 * Revision 1.2  1994/09/26  17:06:37  quinn
 * Back again...
 *
 * Revision 1.1  1994/09/26  16:07:59  quinn
 * Most of the functionality in place.
 *
 */

#ifndef PHYSICAL_H
#define PHYSICAL_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void is_p_sync(is_mtable *tab);
void is_p_unmap(is_mtable *tab);
void is_p_align(is_mtable *tab);
void is_p_remap(is_mtable *tab);
int is_p_read_partial(is_mtable *tab, is_mblock *block);
int is_p_read_full(is_mtable *tab, is_mblock *block);

#ifdef __cplusplus
}
#endif


#endif
