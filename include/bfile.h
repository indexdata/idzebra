/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 $Log: bfile.h,v $
 Revision 1.6  1994-09-14 13:10:35  quinn
 Small changes

 * Revision 1.5  1994/08/24  08:45:52  quinn
 * Using mfile.
 *
 * Revision 1.4  1994/08/17  15:38:28  adam
 * Include of util.h.
 *
 * Revision 1.3  1994/08/17  14:09:47  quinn
 * Small changes
 *
 */

#ifndef BFILE_H
#define BFILE_H

#include <util.h>
#include <mfile.h>

#define bf_blocksize(bf) mf_blocksize(bf->mf)

typedef struct BFile_struct
{
    MFile mf;
} *BFile, BFile_struct;

int bf_close (BFile);
BFile bf_open (const char *name, int block_size, int wflag);
int bf_read (BFile bf, int no, int offset, int num, void *buf);
int bf_write (BFile bf, int no, int offset, int num, const void *buf);

#endif
