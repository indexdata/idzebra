/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.h,v $
 * Revision 1.2  1994-08-17 13:32:33  adam
 * Use cache in dict - not in bfile.
 *
 * Revision 1.1  1994/08/16  16:16:02  adam
 * bfile header created.
 *
 */

#ifndef BFILE_H
#define BFILE_H
#include <util.h>

typedef struct BFile_struct
{
    int fd;
    int block_size;
} *BFile;

int bf_close (BFile);
BFile bf_open (const char *name, int block_size, int rw);
int bf_read (BFile bf, int no, void *buf);
int bf_write (BFile bf, int no, const void *buf);

#endif
