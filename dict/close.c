/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: close.c,v $
 * Revision 1.3  1994-08-18 12:40:52  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.2  1994/08/17  13:32:19  adam
 * Use cache in dict - not in bfile.
 *
 * Revision 1.1  1994/08/16  16:26:47  adam
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
    
    dict_bf_close (dict->dbf);
    xfree (dict);
    return 0;
}

