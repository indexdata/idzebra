/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 $Log: bfile.h,v $
 Revision 1.4  1994-08-17 15:38:28  adam
 Include of util.h.

 * Revision 1.3  1994/08/17  14:09:47  quinn
 * Small changes
 *
 */

#ifndef BFILE_H
#define BFILE_H

#include <util.h>

typedef struct BFile_struct
{
    int fd;
    int block_size;
} *BFile, BFile_struct;

int bf_close (BFile);
BFile bf_open (const char *name, int block_size, int wflag);
int bf_read (BFile bf, int no, int offset, int num, void *buf);
int bf_write (BFile bf, int no, int offset, int num, const void *buf);

#endif
