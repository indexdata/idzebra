/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rootblk.h,v $
 * Revision 1.3  1999-05-26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.2  1999/02/02 14:51:25  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.1  1994/09/26 16:08:00  quinn
 * Most of the functionality in place.
 *
 */

#ifndef ROOTBLK_H
#define ROOTBLK_H

#include <isam.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct is_type_header
{
    int blocksize;                /* for sanity-checking */
    int keysize;                  /*   -do-  */
    int freelist;                 /* first free block */
    int top;                      /* first unused block */
} is_type_header;

int is_rb_write(isam_blocktype *ib, is_type_header *hd);
int is_rb_read(isam_blocktype *ib, is_type_header *hd);

#ifdef __cplusplus
}
#endif

#endif
