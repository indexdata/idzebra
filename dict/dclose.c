/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dclose.c,v $
 * Revision 1.5  1999-02-02 14:50:16  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.4  1997/09/09 13:38:01  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.3  1994/09/01 17:49:36  adam
 * Removed stupid line. Work on insertion in dictionary. Not finished yet.
 *
 */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <dict.h>

int dict_bf_close (Dict_BFile dbf)
{
    int i;
    dict_bf_flush_blocks (dbf, -1);
    
    xfree (dbf->all_blocks);
    xfree (dbf->all_data);
    xfree (dbf->hash_array);
    i = bf_close (dbf->bf);
    xfree (dbf);
    return i;
}
