/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: close.c,v $
 * Revision 1.1  1994-08-16 16:26:47  adam
 * Added dict.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

int dict_close (Dict dict)
{
    assert (dict);
    
    bf_close (dict->bf);
    free (dict);
    return 0;
}

