/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rootblk.h,v $
 * Revision 1.1  1994-09-26 16:08:00  quinn
 * Most of the functionality in place.
 *
 */

#ifndef ROOTBLK_H
#define ROOTBLK_H

#include <isam.h>

typedef struct is_type_header
{
    int blocksize;                /* for sanity-checking */
    int keysize;                  /*   -do-  */
    int freelist;                 /* first free block */
    int top;                      /* first unused block */
} is_type_header;



int is_rb_write(isam_blocktype *ib, is_type_header *hd);
int is_rb_read(isam_blocktype *ib, is_type_header *hd);

#endif
