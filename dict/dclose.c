/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dclose.c,v $
 * Revision 1.3  1994-09-01 17:49:36  adam
 * Removed stupid line. Work on insertion in dictionary. Not finished yet.
 *
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
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
